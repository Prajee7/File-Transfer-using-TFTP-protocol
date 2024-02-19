#ifndef TFTPCOMMON_H
#define TFTPCOMMON_H

#include <netinet/in.h>
#include <cstdio>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <cstring>
#include "TftpError.h"
#include "TftpOpcode.h"
#include "TftpConstant.h"

// Helper function declaration
void printBuffer(const char *buffer, unsigned int len);

// Function to extract filename from the buffer
std::string extractFilename(const char* buffer);

// Send an acknowledgment packet
void sendAck(int sockfd, const sockaddr_in* addr, socklen_t addrLen, unsigned short blockNum);

// Send an error packet
void sendError(int sockfd, const sockaddr_in* addr, socklen_t addrLen, unsigned short errorCode, const char* errorMsg);

// Send a file to the TFTP server
bool sendFile(int sockfd, const char* filename, struct sockaddr_in* serv_addr);

#endif // TFTPCOMMON_H

