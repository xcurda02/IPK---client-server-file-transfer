/**
 * Soubor: ipk-client.cpp
 * Autor: Vojtech Curda (xcurda02)
 *
 * Klientska cast 1. projektu IPK
 */
#include <iostream>
#include <getopt.h>
#include <sys/socket.h>
#include <netdb.h>
#include <strings.h>
#include <cstring>
#include <unistd.h>
#include <algorithm>
#include "msg.h"


typedef struct args{
    char *server;
    int port;
    __int8_t mode;
    char *file;
}args;

/**
 * Ziskani argumentu z prikazove radky
 * @param argc  pocet argumentu
 * @param argv  argumenty
 * @return    nullptr - chyba | struktura vyplnena argumenty - uspech
 */
args *getArgs(int argc, char **argv){
    int c;

    bool RWopt = false;

    args *arguments = new args();
    arguments->server = nullptr;
    arguments->port = -1;
    arguments->mode = -1;
    arguments->file = nullptr;
    while ((c = getopt (argc, argv, "h:p:r:w:")) != -1)
        switch (c)
        {
            case 'h': {
                char *server = (char*) malloc(strlen(optarg)+1);
                strcpy(server, optarg);
                arguments->server = server;
                break;
            }

            case 'p': {
                char *ptr = nullptr;
                arguments->port = (int) strtol(optarg, &ptr, 10);
                if (*ptr) {
                    std::cerr << "ERROR: invalid port number\n";
                    free(arguments);
                    return nullptr;
                }
                break;
            }
            case 'w':
            case 'r':
                if(!RWopt){
                    RWopt = true;
                    arguments->mode = c == 'r' ? msg::READ_MODE : msg::WRITE_MODE;
                    char *file = (char*) malloc(strlen(optarg)+1);
                    strcpy(file,optarg);
                    arguments->file = file;
                } else{
                    std::cerr << "ERROR: Cannot combine -r and -w parameters\n";
                    free(arguments);
                    return nullptr;
                }
                break;

            default:
                std::cerr << "ERROR: Invalid parameter\n";
                free(arguments);
                return nullptr;
        }

    if (arguments->server == nullptr || arguments->port == -1 || arguments->mode == -1 || arguments->file == nullptr) {
       std::cerr << "ERROR: Not enough parameters\n";
        free(arguments);
        return nullptr;
    }

    return arguments;
}

int getConnection(args *cli_args){
    int client_socket;
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
        std::cerr << "ERROR: Failed to create socket descriptor\n";
        return -1;
    }

    struct hostent *server = gethostbyname(cli_args->server);

    if (server == NULL) {
        std::cerr << "ERROR: Invalid server address\n";
        return -1;
    }

    struct sockaddr_in server_address;

    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;

    bcopy(server->h_addr, (char *) &server_address.sin_addr.s_addr, (size_t) server->h_length);

    server_address.sin_port = htons((uint16_t) cli_args->port);

    if (connect(client_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        std::cerr << "ERROR connecting to server\n";
        return -1;
    }
    return client_socket;
}

int main(int argc, char **argv) {
    args *args = getArgs(argc, argv);
    if (args == nullptr)
        exit(EXIT_FAILURE);

    int client_socket = getConnection(args);
    if (client_socket < 0){
        exit(EXIT_FAILURE);
    }

    // Odesilani informace o zadanem modu (R/W)
    ssize_t n = send(client_socket,&(args->mode),1,0);
    if (n < 0) {
        std::cerr << "ERROR writing on socket\n";
        exit(EXIT_FAILURE);
    }

    char filename[BUF_SIZE];
    bzero(filename,BUF_SIZE);

    strcpy(filename,args->file);

    std::cout << "Mode: " << ((args->mode == msg::READ_MODE) ? "read" : "write") << std::endl;
    std::cout << "Filename: " << filename << std::endl;

    if(msg::send_filename(client_socket, filename) == -1){
        exit(EXIT_FAILURE);
    }

    int err = 0;
    if (args->mode == msg::READ_MODE)
        err = msg::receiveFile(args->file,client_socket,true);
    else
        err = msg::sendFile(args->file,client_socket,true);


    if (!err)
        std::cout << "\nFile transfer successul\n";
    else
        std::cout << "\nFile transfer not successul\n";

    close(client_socket);
    free(args->server);
    free(args->file);
    delete(args);

    return 0;
}