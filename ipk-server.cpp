#include <iostream>
#include <getopt.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <strings.h>
#include <cstring>
#include <unistd.h>
#include <csignal>
#include "msg.h"

/**
 * Ziskani cisla portu z argumentu
 * @param argc  Pocet argumentu
 * @param argv  Argumenty
 * @return  Cislo portu
 */
int getArgs(int argc, char **argv){
    int portno = -1;
    int c;
    while ((c = getopt (argc, argv, "p:")) != -1) {
        switch (c) {
            case 'p': {
                char *ptr = nullptr;
                portno = (int) strtol(optarg, &ptr, 10);
                if (*ptr) {
                    return -1;
                }
                break;
            }
            default:
                return -1;

        }
    }
    return portno;
}

/**
 * Zjisteni prenosoveho modu
 * @param socket Socket
 * @return  prenosovy mod
 */
__int8_t getMode(int socket) {
    __int8_t mode;
    ssize_t n = recv(socket, &mode, 1, MSG_WAITALL);
    if (n < 0) {
        fprintf(stderr, "ERROR reading from socket\n");
        return -1;
    }
    return mode;
}

int childs[2048];
int child_counter;
/**
 * Ridici prommena fungovani serveru
 */
static volatile int keepRunning = 1;
void intHandler(int i){
    keepRunning = 0;
    for (int j = 0; j < child_counter ; ++j) {
        kill(childs[i],SIGTERM);
    }
}

int main(int argc, char **argv ){
    signal(SIGINT, intHandler);
    signal(SIGTERM, intHandler);
    child_counter = 0;

    int portno = getArgs(argc, argv);                       //cislo portu
    if (portno == -1){
        std::cerr << "ERROR: Invalid parameters\n";
    }

    /* Vytvoreni file deskriptoru socketu */
    int server_socket_fd;
    if ((server_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) <= 0){
        std::cerr << "ERROR: Failed to create socket descriptor\n";
        exit(EXIT_FAILURE);
    }


    /* Vyplneni adresy serveru */
    struct sockaddr_in server_address;
    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons((uint16_t)portno);

    /* Bindovani socketu k adrese serveru */
    if (bind(server_socket_fd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        std::cerr << "ERROR: Error on bind\n";
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in client_address;
    socklen_t clilen = sizeof(client_address);

    while (keepRunning) {
        /* Naslouchani spojeni */
        listen(server_socket_fd,5);

        fd_set set;
        FD_ZERO(&set);
        FD_SET(server_socket_fd, &set);

        int rv = select(server_socket_fd + 1, &set, NULL, NULL, NULL);
        if(rv == -1) {                                     //Preruseni signalem z klavesnice CTRL+C
           std::cout << "\nQuitting server\n";
        }
        else if(rv == 0) {
            std::cout << "timeout occurred (20 second)\n"; /* a timeout occured */
            return EXIT_FAILURE;
        }
        else {

            /*Prijeti spojeni*/
            int new_socket_fd = accept(server_socket_fd, (struct sockaddr *) &client_address, &clilen);
            if (new_socket_fd < 0) {
                std::cerr << "ERROR: Error on accept\n";
                break;
            } else{
                /* Zjisteni IP adresy klienta */
                struct sockaddr_in* pV4Addr = &client_address;
                struct in_addr ipAddr = pV4Addr->sin_addr;
                char addr[INET_ADDRSTRLEN];
                inet_ntop( AF_INET, &ipAddr,addr, INET_ADDRSTRLEN );
                std::cout << "Client connected, IP: " << addr << std::endl;
            }

            /* Vytvoreni noveho procesu, ktery bude provadet prenos */
            pid_t pid = fork();
            if (pid == 0) {
                if (new_socket_fd < 0) {
                    exit(EXIT_FAILURE);
                }

                /* Ziskani modu prenosu */
                int8_t mode = getMode(new_socket_fd);
                if (mode == -1){
                    exit(EXIT_FAILURE);
                }

                /* Ziskani jmena souboru */
                char filename[BUF_SIZE];
                bzero(filename, BUF_SIZE);
                ssize_t n = recv(new_socket_fd, filename, BUF_SIZE - 1,MSG_WAITALL);
                if (n < 0) {
                    std::cerr << "ERROR: Error reading from socket\n";
                    exit(EXIT_FAILURE);
                }

                std::cout << "Mode: " << ((mode == msg::READ_MODE) ? "read" : "write") << std::endl;
                std::cout << "Filename: " << filename << std::endl;

                int err = 0;
                if (mode == msg::READ_MODE)         //cteni
                    err = msg::sendFile(filename, new_socket_fd, false);
                else                                //zapis
                    err = msg::receiveFile(filename, new_socket_fd, false);

                if (!err)
                    std::cout << "File transfer successul\n";
                else
                    std::cout << "File transfer not successul\n";

                close(new_socket_fd);
                exit(0);

            } else if (pid == -1){
                std::cerr << "ERROR: Error on fork() call\n";
                exit(EXIT_FAILURE);
            } else{
                childs[child_counter] = pid;
            }
        }
    }

    close(server_socket_fd);

    return 0;
}

