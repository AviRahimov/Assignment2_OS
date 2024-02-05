#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 3490
#define BUFFER_SIZE 4096

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void handle_client(int sock) {
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE); // Clear the buffer

    // Read the request from the client
    ssize_t n = read(sock, buffer, BUFFER_SIZE - 1);
    if (n < 0) {
        perror("ERROR reading from socket");
        return;
    }

    printf("Here is the message: %s\n", buffer);

    // Parse the requested path from the GET request
    // This is a very simplified parser and should be improved for real applications
    char *method = strtok(buffer, " ");
    char *path = strtok(NULL, " ");
    
    if (method && strcmp(method, "GET") == 0 && path) {
        // Removing the leading "/" from the path
        if (path[0] == '/') path++;

        // Open and read the file
        FILE *file = fopen(path, "rb");
        if (file == NULL) {
            // File not found, send 404 response
            char *notFoundResponse = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nFile not found.";
            write(sock, notFoundResponse, strlen(notFoundResponse));
        } else {
            // File found, read its content and send it
            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file);
            rewind(file);
            char *fileContent = malloc(fileSize + 1);
            fread(fileContent, 1, fileSize, file);
            fileContent[fileSize] = '\0'; // Null-terminate the file content

            sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%s", fileContent);
            write(sock, response, strlen(response));
            free(fileContent); // Free the allocated memory
            fclose(file); // Close the file
        }
    } else {
        // Not a GET request or path is missing, send 400 Bad Request response
        char *badRequestResponse = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nBad Request.";
        write(sock, badRequestResponse, strlen(badRequestResponse));
    }

    close(sock); // Close the connection after handling the request
}

int main() {
    int sockfd, newsockfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");

        int pid = fork();
        if (pid < 0) {
            error("ERROR on fork");
        }
        if (pid == 0) {  // Child process
            close(sockfd);
            handle_client(newsockfd);
            exit(0);
        } else {  // Parent process
            close(newsockfd);
        }
    }

    close(sockfd);
    return 0;
}