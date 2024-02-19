#ifndef TFTP_CONSTANT_H
#define TFTP_CONSTANT_H

/*
 * TODO: Add constant variables in this file
 */
static const unsigned int MAX_PACKET_LEN = 516; // data 512 + opcode 2 + block num 2
static const unsigned int MAX_DATA_LEN = 512;
static const char * SERVER_FOLDER = "server-files/"; // DO NOT CHANGE
static const char * CLIENT_FOLDER = "client-files/"; // DO NOT CHANGE

#endif // TFTP_CONSTANT_H

