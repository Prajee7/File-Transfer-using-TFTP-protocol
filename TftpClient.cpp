#include <cstdio>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include "TftpError.h"
#include "TftpOpcode.h"
#include "TftpCommon.h" // Assuming TftpCommon.h is the header for TftpCommon.cpp

#define SERV_UDP_PORT 61125
#define SERV_HOST_ADDR "127.0.0.1"

char *program;

void processReadRequest(int sockfd, FILE *file)
{
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    int recv_len;
    unsigned short block_num = 1;
    bool last_packet = false;

    while (!last_packet) {
        char buffer[MAX_PACKET_LEN];
        recv_len = recvfrom(sockfd, buffer, MAX_PACKET_LEN, 0, (struct sockaddr *)&from_addr, &from_len);
        
        if (recv_len < 0) {
            perror("Error receiving data");
            exit(EXIT_FAILURE);
        }

        // Extract opcode and block number
        unsigned short opcode = ntohs(*((unsigned short*)buffer));
        unsigned short received_block_num = ntohs(*((unsigned short*)(buffer + 2)));

        if (opcode == TFTP_DATA && received_block_num == block_num) {
            // Write data to file
            int data_len = recv_len - 4; // Subtract 4 bytes for opcode and block number
            fwrite(buffer + 4, 1, data_len, file); // Data starts at buffer + 4

            // Check if it is the last packet
            if (data_len < MAX_DATA_LEN) {
                last_packet = true;
            }

            // Send ACK
            *((unsigned short *)buffer) = htons(TFTP_ACK);
            *((unsigned short *)(buffer + 2)) = htons(block_num); // ACK the received block number

            if (sendto(sockfd, buffer, 4, 0, (struct sockaddr *)&from_addr, from_len) < 0) {
                perror("Error sending ACK");
                exit(EXIT_FAILURE);
            }

            block_num++; // Prepare for the next block
        }
    }
}

void processWriteRequest(int sockfd, struct sockaddr_in *serv_addr, FILE *file) {
    struct sockaddr_in fromAddr;
    socklen_t fromAddrLen = sizeof(fromAddr);
    char ackBuffer[MAX_PACKET_LEN];
    unsigned short blockNum = 0; // Start block numbers from 0 for WRQ's initial ACK
    bool isLastPacket = false;

    // Await initial ACK for WRQ
    if (recvfrom(sockfd, ackBuffer, MAX_PACKET_LEN, 0, (struct sockaddr*)&fromAddr, &fromAddrLen) < 0) {
        perror("Error receiving initial ACK");
        exit(EXIT_FAILURE);
    }

    // Validate initial ACK
    if (ntohs(*(unsigned short*)ackBuffer) != TFTP_ACK || ntohs(*(unsigned short*)(ackBuffer + 2)) != blockNum) {
        fprintf(stderr, "Initial ACK for WRQ failed\n");
        exit(EXIT_FAILURE);
    }

    blockNum++; // Increment after receiving initial ACK for WRQ

    while (!isLastPacket) {
        char dataBuffer[MAX_DATA_LEN];
        size_t bytesRead = fread(dataBuffer, 1, MAX_DATA_LEN, file);
        isLastPacket = bytesRead < MAX_DATA_LEN;

        char packetBuffer[MAX_PACKET_LEN];
        unsigned short* packetOpcode = (unsigned short*)packetBuffer;
        *packetOpcode = htons(TFTP_DATA);
        unsigned short* packetBlockNum = (unsigned short*)(packetBuffer + 2);
        *packetBlockNum = htons(blockNum);

        memcpy(packetBuffer + 4, dataBuffer, bytesRead);
        int packetSize = 4 + bytesRead;

        if (sendto(sockfd, packetBuffer, packetSize, 0, (struct sockaddr*)serv_addr, sizeof(*serv_addr)) < 0) {
            perror("Error sending data packet");
            exit(EXIT_FAILURE);
        }

        if (recvfrom(sockfd, ackBuffer, MAX_PACKET_LEN, 0, (struct sockaddr*)&fromAddr, &fromAddrLen) < 0) {
            perror("Error receiving ACK");
            exit(EXIT_FAILURE);
        }

        if (ntohs(*(unsigned short*)ackBuffer) != TFTP_ACK || ntohs(*(unsigned short*)(ackBuffer + 2)) != blockNum) {
            fprintf(stderr, "Incorrect ACK received\n");
            exit(EXIT_FAILURE);
        }

        blockNum++;
    }
}

int main(int argc, char *argv[])
{
    program = argv[0];

    int sockfd;
    struct sockaddr_in serv_addr;

    // Initialize server address
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_UDP_PORT);
    serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);

    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("Cannot create socket");
        exit(EXIT_FAILURE);
    }

    if (argc != 3)
    {
        std::cerr << "Usage: " << program << " <r|w> <filename>\n";
        exit(EXIT_FAILURE);
    }

    char *mode = argv[1];
    char *filename = argv[2];
    FILE *file;

    // Open file based on mode
    std::string filePath = std::string(CLIENT_FOLDER) + filename;
    if (strcmp(mode, "r") == 0)
    {
        file = fopen(filePath.c_str(), "wb");
    }
    else if (strcmp(mode, "w") == 0)
    {
        file = fopen(filePath.c_str(), "rb");
    }

    if (!file)
    {
        perror("Failed to open file");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Create and send the initial request packet
    int opcode = (strcmp(mode, "r") == 0) ? TFTP_RRQ : TFTP_WRQ;
    char buffer[MAX_PACKET_LEN];
    int len = 0;

    // Opcode
    *((unsigned short *)buffer) = htons(opcode);
    len += 2;

    // Filename
    strcpy(buffer + len, filename);
    len += strlen(filename) + 1; // +1 for the null terminator

    // Mode
    strcpy(buffer + len, mode);
    len += strlen(mode) + 1; // +1 for the null terminator

    // Send the packet
    if (sendto(sockfd, buffer, len, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("sendto failed");
        close(sockfd); // Close socket if send fails
        exit(EXIT_FAILURE);
    }
    else
    {
        std::cout << "Initial TFTP request packet sent successfully." << std::endl;
    }

    // Process the file transfer
    if (strcmp(mode, "r") == 0)
    { // RRQ
        processReadRequest(sockfd, file);
    }
    else
    { // WRQ
        processWriteRequest(sockfd, &serv_addr, file);
    }

    // Cleanup
    fclose(file);
    close(sockfd);

    return 0;
}
