#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> // For struct hostent

#define PORT 3490
#define BUFFER_SIZE 1024

void error(const char *msg) {
    perror(msg);
    exit(1); // Use exit(1) for errors.
}

// Placeholder for actual Base64 encode/decode functions
char* base64_encode(const unsigned char *input, size_t input_length, size_t *output_length) {
    // Implement base64 encoding or use a library
    *output_length = input_length;
    char* output = (char*)malloc(input_length + 1); // Ensure allocation is cast to char*
    strncpy(output, (const char*)input, input_length);
    output[input_length] = '\0'; // Null-terminate the string
    return output;
}

ssize_t readFile(const char* filename, char** buffer) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    *buffer = (char*)malloc(fileSize + 1); // Ensure allocation is cast to char*
    if (!*buffer) {
        fclose(file);
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }

    size_t bytesRead = fread(*buffer, 1, fileSize, file);
    if (bytesRead < fileSize) {
        perror("Failed to read file");
        free(*buffer);
        fclose(file);
        return -1;
    }

    (*buffer)[bytesRead] = '\0'; // Null-terminate the string
    fclose(file);

    return bytesRead;
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[BUFFER_SIZE];

    if (argc < 4) {
       fprintf(stderr,"usage %s hostname operation[GET/POST] remote_path [local_path_for_POST]\n", argv[0]);
       exit(1);
    }

    char* operation = argv[2];
    char* remotePath = argv[3];
    char* localPath = argc == 5 ? argv[4] : NULL;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    serv_addr.sin_port = htons(PORT);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    if (strcmp(operation, "GET") == 0) {
        printf("Sending GET request for: %s\n", remotePath);
        snprintf(buffer, BUFFER_SIZE, "GET %s \r\n\r\n", remotePath);
        if (write(sockfd, buffer, strlen(buffer)) < 0) {
            perror("Error writing to socket");
            exit(1);
        }
        printf("GET request sent.\n");
    }
    else if (strcmp(operation, "POST") == 0 && localPath != NULL) {
        char* fileContent;
        ssize_t bytesRead = readFile(localPath, &fileContent);
        if (bytesRead < 0) {
            error("ERROR reading file to post");
        }

        size_t encodedSize;
        char* encodedContent = base64_encode((const unsigned char*)fileContent, bytesRead, &encodedSize);

        printf("Sending POST request for: %s with data from %s\n", remotePath, localPath);
        snprintf(buffer, BUFFER_SIZE, "POST %s \r\n%s\r\n\r\n", remotePath, encodedContent);
        if (write(sockfd, buffer, strlen(buffer)) < 0) {
            perror("Error writing to socket");
            exit(1);
        }
        printf("POST request sent.\n");

        free(fileContent);
        free(encodedContent);
    }
    else {
        fprintf(stderr, "Invalid operation or missing local path for POST\n");
        close(sockfd);
        exit(1);
    }

    // Common part for reading the response
    memset(buffer, 0, BUFFER_SIZE);
    if (read(sockfd, buffer, BUFFER_SIZE - 1) < 0) {
        perror("Error reading from socket");
        close(sockfd);
        exit(1);
    }
    printf("Server response: %s\n", buffer);

    close(sockfd);
    return 0;
}
