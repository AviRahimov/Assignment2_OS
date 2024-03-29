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

- The server support multiple simultaneous connections using multi-process synchronization.
- It handle GET and POST requests according to the MYSIMPLEHTTP protocol.
- Synchronization is crucial to prevent concurrent access issues. We are using `fcntl(2)` function for file locking.

### Client

- Simple client for testing the server functionality. This client is essential for demonstrating the server's ability to handle requests correctly.

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
1. Example:
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


## Part B - ASYNC I/O Client
ASYNC I/O Client designed to facilitate downloading files, especially when dealing with .list files containing a list of files to download simultaneously. The client utilizes asynchronous I/O operations to handle multiple connections concurrently, ensuring efficient downloading.

## How to Compile and Run part B
#### Compiling and running the Server

1. Open a terminal and navigate to the directory containing the `server.c` file.
2. Run the following command to compile the server application:
   ```bash
   gcc -o server server.c
3. ```bash
   ./server files/ 


#### Compiling the asynchronic Client
1. In the same or a different terminal, navigate to the directory containing the `async_client.c` file.
2. Compile the client application using the following command:
    ```bash
    gcc -o async_client async_client.c -lcrypto -lm


#### Running the asynchronic Client
In the same terminal run the following command:
1. ```bash
   ./async_client localhost GET ubuntu_logo.jpg
note that ubuntu_logo.jpg is just an example to picture file and can be replaced with any that can be converted to Base64.

#### Run with files.list
In the same terminal run the following command:
1. ```bash
   ./async_client localhost GET list_of_files.list
Note that inside "list_of_files.list” there is a string encoded in base64 that stores the client IP and the txt files.

If you do GET to hello.txt (which is txt file in files, inside there is a string encoded in base64), and then POST with this command:
1. ```bash
   ./async_client localhost POST hello.txt hello.txt
   
You are supposed to see in the client a txt file with the same name with "Hello World!" inside.

#### Create a folder
If you want to create a folder you can do this with this next command:
1. ```bash
   ./async_client localhost POST <name_of_folder>/ubuntu_logo.jpg ubuntu_logo.jpg
This command will create a folder inside files and inside this folder you should see the jpg image.
Note that you should replace "name_of_folder" with the name of folder you want to.
Please be aware that the server is listen to files.

### Acknowledgments

Thanks to all the contributors who have invested their time in improving this assignment and make it all work well:
Jonatan boritsky, Avi rahimov, Avichay mazin.


