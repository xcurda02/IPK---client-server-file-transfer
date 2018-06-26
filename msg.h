//
// Created by vojtech on 5.3.18.
//

#ifndef IPK_MSG_H
#define IPK_MSG_H


#include <cstdint>
#include <cstdio>

#define BUF_SIZE 8192           //Velikost bufferu , pomoci ktereho se prenaseji data


/**
 * Trida obsahuje funkce a pomocne konstanty pro prenos souboru
 */
class msg {
public:

    static const int8_t READ_MODE;
    static const int8_t WRITE_MODE;

    /**
     * Poslani nazvu souboru
     * @param socket    Socket, na ktery se bude posilat
     * @param filename  Nazev souboru
     * @return          -1 chyba | 0 bez chyb
     */
    static int send_filename(int socket, char* filename);

    /**
     * Poslani souboru, funkce deli soubor do bufferu delky BUF_SIZE a
     * posila na zadany socket
     * @param filename
     * @param socket
     * @return
     */
    static int sendFile(char *filename, int socket, bool client);

    /**
     * Prijeti souboru
     * @param filename
     * @param socket
     * @return
     */
    static int receiveFile(char *filename, int socket, bool client);

private:
    static void printProgress(long filesize, long done);

    /**
     * Poslani velikosti souboru
     * @param socket    Socket, na ktery se bude posilat
     * @param filename  Nazev souboru
     * @return          -1 chyba | 0 bez chyb
     */
    static long send_filesize(int socket, char* filename);

    /**
     * Ziskani velikosti prenaseneho souboru
     * @param socket    Socket, ze ktereho se bude prijimat
     * @param filename  Nazev souboru
     * @return          Velikost souboru
     */
    static long receive_filesize(int socket);


};
#endif //IPK_MSG_H
