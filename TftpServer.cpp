#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "TftpCommon.h"

#define SERV_UDP_PORT 61125 // Server's listening port as per your setup

// Implementation of read request handling (RRQ)
void handleReadRequest(int sockfd, char *buffer, int recv_len, struct sockaddr_in *client_addr, socklen_t client_len)
{
    // Extract the filename from the request
    std::string filename = extractFilename(buffer); // Utilize extractFilename if available, or directly use buffer + 2

    // Construct the full path of the file
    std::string fullPath = std::string(SERVER_FOLDER) + filename;

    // Log the request
    std::cout << "Handling RRQ for file: " << fullPath << std::endl;

    // Call sendFile to handle the file transfer
    bool success = sendFile(sockfd, fullPath.c_str(), client_addr);

    if (!success)
    {
        std::cerr << "Failed to send file: " << fullPath << std::endl;
        // send an error packet to the client.
        sendError(sockfd, client_addr, client_len, TFTP_ERROR_SEND_FAILED, "Failed to send file");
    }
}

// Handle a write request from a client.
void handleWriteRequest(int sockfd, char *buffer, int recv_len, struct sockaddr_in *client_addr, socklen_t client_len) {
    // Extract the filename where the client wants to write.
    std::string filename = extractFilename(buffer);

    // If the filename is empty, send an error message back to the client.
    if (filename.empty()) {
        sendError(sockfd, client_addr, client_len, TFTP_ERROR_FAILED, "Filename is missing.");
        return;
    }

    // Construct the full path where the file will be created or overwritten.
    std::string fullPath = std::string(SERVER_FOLDER) + filename;

    // Open or create the file for writing.
    FILE* file = fopen(fullPath.c_str(), "wb");
    if (!file) {
        sendError(sockfd, client_addr, client_len, TFTP_ERROR_FAILED, "Failed to open or create file");
        return;
    }

    sendAck(sockfd, client_addr, client_len, 0); // Send ACK for WRQ

    // Loop to receive data packets and write to the file.
    unsigned short expectedBlockNum = 1;
    bool isLastPacket = false;

    while (!isLastPacket) {
        memset(buffer, 0, MAX_PACKET_LEN);
        recv_len = recvfrom(sockfd, buffer, MAX_PACKET_LEN, 0, (struct sockaddr*)client_addr, &client_len);

        // Check if the received packet is smaller than the minimum expected size.
        if (recv_len < 4) { // Minimum length for data packet (opcode + block number)
            sendError(sockfd, client_addr, client_len, TFTP_ERROR_ILLEGAL_OPERATION, "Invalid packet size");
            fclose(file);
            return;
        }

        unsigned short opcode = ntohs(*(unsigned short*)buffer);
        unsigned short blockNum = ntohs(*(unsigned short*)(buffer + 2));

        // Write the data to the file if the opcode and block number are correct.
        if (opcode == TFTP_DATA && blockNum == expectedBlockNum) {
            fwrite(buffer + 4, 1, recv_len - 4, file);
            sendAck(sockfd, client_addr, client_len, blockNum);

            // If the received packet is smaller than the max packet size, it's the last packet.
            if (recv_len < MAX_PACKET_LEN) isLastPacket = true;
            expectedBlockNum++;
        } else {
            sendError(sockfd, client_addr, client_len, TFTP_ERROR_FAILED, "Unexpected block number");
            fclose(file);
            return;
        }
    }

    fclose(file);
}

// Listen for incoming TFTP requests and handle them accordingly.
int handleIncomingRequest(int sockfd)
{
    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);
    char buffer[MAX_PACKET_LEN];

    for (;;)
    {
        memset(buffer, 0, MAX_PACKET_LEN);
        ssize_t recv_len = recvfrom(sockfd, buffer, MAX_PACKET_LEN, 0, (struct sockaddr *)&cli_addr, &cli_len);
        if (recv_len < 0)
        {
            perror("Error receiving request");
            continue; // Continue in case of error
        }

        // Determine the type of request based on the opcode and handle it.
        unsigned short opcode = ntohs(*(unsigned short *)buffer);
        switch (opcode)
        {
        case TFTP_RRQ:
            handleReadRequest(sockfd, buffer, recv_len, &cli_addr, cli_len);
            break;
        case TFTP_WRQ:
            handleWriteRequest(sockfd, buffer, recv_len, &cli_addr, cli_len);
            break;
        default:
            sendError(sockfd, &cli_addr, cli_len, TFTP_ERROR_ILLEGAL_OPERATION, "Illegal TFTP operation.");
            break;
        }
    }
}

// Main function to set up the server socket and handle incoming requests.
int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in serv_addr;

    // Initialize server address structure
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_UDP_PORT);

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Cannot create socket");
        return 1;
    }

    // Bind the socket
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Bind failed");
        close(sockfd);
        return 1;
    }

    // Handle incoming requests
    handleIncomingRequest(sockfd);

    // Close the socket
    close(sockfd);
    return 0;
}
