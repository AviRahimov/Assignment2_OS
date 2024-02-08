# Operating Systems Course: Assignment 2 - Multi-Process Synchronization and Async I/O

## Overview

This assignment is part of the Operating Systems course, designed to deepen the understanding of multi-process synchronization and asynchronous I/O operations. The task involves implementing a simplified HTTP server and client (MYSIMPLEHTTP) to explore these concepts through practical application. This project focuses on network programming, file handling, and synchronization mechanisms in a Unix/Linux environment.

## Objective

To implement a server-client architecture that effectively demonstrates multi-process synchronization and asynchronous I/O using a custom, simplified HTTP protocol (MYSIMPLEHTTP). The protocol supports basic file transfer operations with a focus on synchronization and security considerations.

## Getting Started

Before attempting this assignment, ensure completion of all exercises in Beej's Guide to Network Programming. This foundational knowledge is critical for understanding the networking concepts applied in this assignment.
- https://beej.us/guide/bgnet/examples/server.c
- https://beej.us/guide/bgnet/examples/client.c
- https://beej.us/guide/bgnet/examples/listener.c
- https://beej.us/guide/bgnet/examples/talker.c


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


