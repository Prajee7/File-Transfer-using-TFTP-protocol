#include <cstdio>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <cstring>
#include "TftpError.h"
#include "TftpOpcode.h"
#include "TftpCommon.h"


// Helper function to print the first len bytes of the buffer in Hex
 void printBuffer(const char * buffer, unsigned int len) {
    for(int i = 0; i < len; i++) {
        printf("%x,", buffer[i]);
    }
    printf("\n");
}

std::string extractFilename(const char* buffer) {
    // Assuming the filename starts at buffer index 2 (after the 2-byte opcode)
    // and ends before the first null byte following it.
    return std::string(buffer + 2);
}


/*
 * TODO: Add common code that is shared between your server and your client here. For example: helper functions for
 * sending bytes, receiving bytes, parse opcode from a tftp packet, parse data block/ack number from a tftp packet,
 * create a data block/ack packet, and the common "process the file transfer" logic.
 */
void sendAck(int sockfd, const sockaddr_in* addr, socklen_t addrLen, unsigned short blockNum) {
    char ackPacket[4];
    unsigned short opcode = htons(TFTP_ACK); // ACK opcode is 4
    unsigned short block = htons(blockNum);
    memcpy(ackPacket, &opcode, sizeof(opcode));
    memcpy(ackPacket + 2, &block, sizeof(block));
    
    // Print statement to show the ACK being sent
    printf("Sending ACK for block #%u\n", blockNum);

    sendto(sockfd, ackPacket, sizeof(ackPacket), 0, (struct sockaddr*)addr, addrLen);
}

void sendError(int sockfd, const sockaddr_in* addr, socklen_t addrLen, unsigned short errorCode, const char* errorMsg) {
    char errorPacket[512];
    unsigned short opcode = htons(TFTP_ERROR); // ERROR opcode is 5
    memcpy(errorPacket, &opcode, sizeof(opcode));
    memcpy(errorPacket + 2, &errorCode, sizeof(errorCode));
    strcpy(errorPacket + 4, errorMsg);
    size_t errMsgLength = strlen(errorMsg) + 1; // Include null terminator
    sendto(sockfd, errorPacket, 4 + errMsgLength, 0, (struct sockaddr*)addr, addrLen);
}


/**
 * Sends a file to the TFTP server. This function should manage the sending of data packets
 * and wait for acknowledgment packets from the server according to the TFTP protocol.
 */
bool sendFile(int sockfd, const char* filename, struct sockaddr_in* serv_addr) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Could not open file");
        return false;
    }

    char buffer[MAX_PACKET_LEN];
    char ackBuffer[4]; // For ACK packets
    int blockNum = 0;
    bool isLastPacket = false;

    while (!isLastPacket) {
        blockNum++; // Increment block number for each packet
        int bytesRead = fread(buffer + 4, 1, MAX_DATA_LEN, file);
        if (bytesRead < MAX_DATA_LEN) {
            isLastPacket = true; // This is the last packet if we read less than 512 bytes
        }

        // Prepare the data packet
        *(short*)buffer = htons(TFTP_DATA); // Opcode for data packet
        *(short*)(buffer + 2) = htons(blockNum); // Block number

        // Send the data packet
        if (sendto(sockfd, buffer, bytesRead + 4, 0, (struct sockaddr*)serv_addr, sizeof(*serv_addr)) < 0) {
            perror("sendto failed");
            fclose(file);
            return false;
        }

        // Wait for ACK
        socklen_t fromLen = sizeof(*serv_addr);
        if (recvfrom(sockfd, ackBuffer, sizeof(ackBuffer), 0, (struct sockaddr*)serv_addr, &fromLen) < 0) {
            perror("recvfrom failed");
            fclose(file);
            return false;
        }

        // Verify ACK packet
        if (ntohs(*(short*)ackBuffer) != TFTP_ACK || ntohs(*(short*)(ackBuffer + 2)) != blockNum) {
            fprintf(stderr, "Received incorrect ACK for block %d\n", blockNum);
            fclose(file);
            return false;
        }
    }

    fclose(file);
    return true;
}