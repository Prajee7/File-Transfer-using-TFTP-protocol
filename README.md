# File-Transfer-using-TFTP-protocol

This project involves developing a TFTP client capable of handling two primary operations: reading files from and writing files to a TFTP server. Implemented in C++, it operates over UDP, aligning with TFTP's specifications for minimalistic, unsecured file transfer between client and server in a network. The project focuses on the fundamental operations defined in the TFTP protocol, as outlined in RFC 1350, emphasizing simplicity and reliability.

----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

For this project I have programed a client/server network application in C/C++ on a UNIX platform. The server and client can transfer files between each other over TFTP protocol.

TFTP is used over local area networks with low error rates, low delays, and high speeds. It employs a simple stop-and-wait scheme over UDP.
In a TFTP file transfer there are two parties involved: a client and a server. The client may read a file from the server, or write a file to the server. You need to write and run two programs, a server and a client, that support file transfer in both directions.

The server runs continuously, listening on a predefined UDP port. If a received client request can be satisfied, a file transfer session begins and ends either after successful completion, or after a fatal error occurs. While the clients exit after each session terminates, the server must remain running to listen for the next client request, if it has not been terminated due to a fatal error.

To test the programs, you may start your server in one terminal and your clients in another terminal. The client takes two arguments. The first indicates whether it is a read or write request, the second is the filename to be read or written. Eg: tftpclient r filename or tftpclient w filename, to read or write files respectively. A read request from the client will download the file from the server, while a write request from the client will upload the file to the server.

During normal protocol operation, the programs send and receive messages according to protocol rules. The core logic of the program consists of a main receive/send loop plus some initialization and termination code. In the loop, you wait for a message from the peer. If it is the expected message, you send your next message or terminate the session successfully. You need to know the message number (data block #) that you are next expecting to see, so that you can distinguish between new and duplicate messages, whether DATA or ACK.
