//
// Created by vojtech on 4.3.18.
//

#include <cstdint>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <strings.h>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <iostream>
#include "msg.h"

    const int8_t msg::READ_MODE = 0;
    const int8_t msg::WRITE_MODE = 1;


    int msg::send_filename(int socket, char* filename){
        ssize_t n = send(socket, filename, BUF_SIZE-1, 0);
        if (n < 0) {
            std::cerr << "ERROR writing to socket\n";
            return -1;
        }
        return 0;
    }
    long msg::send_filesize(int socket, char* filename){
        long size;
        if(fopen(filename,"r") == nullptr){
            size = -1;

        } else{
            struct stat st;
            stat(filename, &st);
            size = st.st_size;

        }

        char str[21] = {0};
        sprintf(str,"%ld",size);

        if (size < 0)
            std::cout << "Requested file does not exist\n";
        else
            std::cout << "Filesize: " << size << " bytes\n";

        ssize_t n = send(socket, str, 20,0);
        if (n < 0) {
            std::cerr << "ERROR writing to socket\n";
            return -1;
        }
        return size;
    }

    long msg::receive_filesize(int socket){
        char str[21] = {0};
        ssize_t n = recv(socket,str,20,MSG_WAITALL);
        if (n < 0) {
            std::cerr << "ERROR reading from socket\n";
            exit(EXIT_FAILURE);
        }
        long filesize = strtol(str, nullptr,10);
        if (filesize < 0)
            std::cout << "Requested file does not exist\n";
        else
            std::cout << "Filesize: " << filesize << " bytes\n";

        return filesize;
    }

    void msg::printProgress(long filesize, long done){
        double progress = (double)done/filesize*100;
        printf("\r[");
        for (int i = 0; i < 50 ; i++) {
            if (i < progress/2){
                printf("#");
            } else
                printf(" ");

        }
        printf("]");
        printf("%.2f%%",progress);
        fflush(stdout);
    }

    int msg::sendFile(char *filename, int socket,bool client){
        long filesize = msg::send_filesize(socket,filename);
        if (filesize == -1){
            return -1;
        }

        FILE *file = fopen(filename, "r");
        if (file == nullptr){
            std::cerr << "ERROR opening file \"" << filename << "\"" << " for reading\n";
            return -1;
        }

        char buffer[BUF_SIZE];
        bzero(buffer,BUF_SIZE);


        size_t nread;
        size_t sent = 0;
        while ((nread = fread(buffer, sizeof(char) ,BUF_SIZE-1, file))  > 0)
        {
            if((send(socket,buffer,nread,0)) < 0){
                std::cerr << "ERROR writing on socket\n";
                return -1;
            }
            sent += nread;
            if (client)
                msg::printProgress(filesize,sent);

            bzero(buffer,BUF_SIZE);
        }

        fclose(file);
        return 0;
    }

    int msg::receiveFile(char *filename, int socket, bool client){
        long filesize = msg::receive_filesize(socket);
        if (filesize == -1){
            return -1;
        }

        FILE *file = fopen(filename,"w");
        if (file == nullptr){
            std::cerr << "ERROR opening file \"" << filename << "\"" << " for writing\n";
            return -1;
        }

        char buffer[BUF_SIZE];
        bzero(buffer,BUF_SIZE);
        ssize_t block_size = 0;
        long remaining = filesize;

        while (1){
            block_size = recv(socket, buffer, BUF_SIZE, 0);
            if (block_size < 0){
                std::cerr << "ERROR: Error reading from socket\n";
                return -1;
            }

            for (int i = 0; i < block_size ; ++i) {
                fputc(buffer[i],file);
            }

            remaining -= block_size;
            bzero(buffer,BUF_SIZE);

            if (client)
                msg::printProgress(filesize,filesize-remaining);

            if (remaining == 0){
                break;
            }
        }

        fclose(file);
        return 0;
    }