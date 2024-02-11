# Operating Systems Course: Assignment 2 - Multi-Process Synchronization and Async I/O

## Overview

This assignment is part of the Operating Systems course, designed to deepen the understanding of multi-process synchronization and asynchronous I/O operations. The task involves implementing a simplified HTTP server and client (MYSIMPLEHTTP) to explore these concepts through practical application. This project focuses on network programming, file handling, and synchronization mechanisms in a Unix/Linux environment.

## Objective

To implement a server-client architecture that effectively demonstrates multi-process synchronization and asynchronous I/O using a custom, simplified HTTP protocol (MYSIMPLEHTTP). The protocol supports basic file transfer operations with a focus on synchronization and security considerations.

## Getting Started

Before attempting this assignment, ensure completion of all exercises in Beej's Guide to Network Programming. This foundational knowledge is critical for understanding the networking concepts applied in this assignment.

- [Beej's Guide to Network Programming - Server Example](https://beej.us/guide/bgnet/examples/server.c)
- [Beej's Guide to Network Programming - Client Example](https://beej.us/guide/bgnet/examples/client.c)
- [Beej's Guide to Network Programming - Listener Example](https://beej.us/guide/bgnet/examples/listener.c)
- [Beej's Guide to Network Programming - Talker Example](https://beej.us/guide/bgnet/examples/talker.c)

## MYSIMPLEHTTP Protocol

MYSIMPLEHTTP is a very simplified version of the HTTP protocol, supporting only two operations:

### Instructions

- **GET**: Requests a file from the server.
- **POST**: Uploads a file to the server.

### Server Responses

- **200 OK**: The request was successful, and the file's contents (Base64 encoded) are returned.
- **404 FILE NOT FOUND**: The requested file does not exist on the server.
- **500 INTERNAL ERROR**: An unspecified error occurred.
  
## Project Structure

### Server

- The server should support multiple simultaneous connections using multi-process synchronization.
- It must handle GET and POST requests according to the MYSIMPLEHTTP protocol.
- Synchronization is crucial to prevent concurrent access issues. Use `fcntl(2)` for file locking.

### Client

- Develop a simple client for testing the server functionality. This client is essential for demonstrating the server's ability to handle requests correctly but will not be submitted as part of this assignment.

## How to Compile and Run

### Compiling the Server and Client

Before running the server and client, you need to compile the source code. Make sure you have the GCC compiler and OpenSSL installed on your system. OpenSSL is required for encoding and decoding the data.

#### Compiling the Server

1. Open a terminal and navigate to the directory containing the `server.c` file.
2. Run the following command to compile the server application:
   ```bash
   gcc -o server server.c -lcrypto -lm

#### Compiling the Client
1. In the same or a different terminal, navigate to the directory containing the `client.c` file.
2. Compile the client application using the following command:
    ```bash
    gcc -o client client.c -lcrypto -lm

#### Running the Server
To start the server, run the following command in the terminal where you compiled the server, replacing <home_directory> with the path to the directory you want to use as the home directory for the server:
`./server <home_directory>`
Example:
    ```bash
    ./server ../Assignment2_OS/

#### Running the Client
To request a file from the server or to upload a file, run the client application in a new terminal. Replace hostname with the server's hostname (use localhost if running on the same machine), operation with GET or POST, and remote_path with the path of the file on the server. Optionally, if performing a POST operation, include local_path_for_POST with the path of the file on the client machine to upload.
For a GET request:
`./client localhost GET /path/to/requested/file`
For a POST request:
`./client localhost POST /path/to/save/file /path/to/local/file`
1. Example GET request:
    ```bash
    ./client localhost GET /read_data.txt

2. Example POST request:
    ```bash
    ./client localhost POST /save_data.txt /path/to/data.txt

## Notes and Best Practices
- Ensure your server is actively running before attempting to connect with the client application.
- When testing POST requests, verify the target directory on the server side is writable and accessible.
- Use relative or absolute paths appropriately when specifying file locations for GET and POST operations.









