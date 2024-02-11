#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> // For struct hostent
#include <openssl/bio.h> 
#include <openssl/evp.h>
#include <math.h>
#include <fcntl.h>
#include <sys/types.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void error(const char *msg) {
    perror(msg);
    exit(1); // Use exit(1) for errors.
}

// Function to calculate the length of a decoded Base64 string
int calcDecodeLength(const char* b64input, size_t len) {
    int padding = 0;

    // Check for trailing '=''s as padding
    if (b64input[len - 1] == '=' && b64input[len - 2] == '=') // Last two characters are =
        padding = 2;
    else if (b64input[len - 1] == '=') // Last character is =
        padding = 1;

    return (int)len * 0.75 - padding;
}

// Function to encode binary data to Base64
int Base64Encode(const char* message, char** buffer, size_t length) {
    BIO *bio, *b64;
    FILE* stream;
    int encodedSize = 4*ceil((double)length/3);
    *buffer = (char *)malloc(encodedSize+1);

    stream = fmemopen(*buffer, encodedSize+1, "w");
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_fp(stream, BIO_NOCLOSE);
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Ignore newlines - write everything in one line
    BIO_write(bio, message, length);
    BIO_flush(bio);
    BIO_free_all(bio);
    fclose(stream);

    return (0); //success
}

// Function to decode Base64 to binary data
int Base64Decode(char* b64message, unsigned char** buffer, size_t* length) {
    BIO *bio, *b64;

    int decodeLen = calcDecodeLength(b64message, *length);
    *buffer = (unsigned char*)malloc(decodeLen + 1);

    bio = BIO_new_mem_buf(b64message, -1);
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // Do not use newlines to flush buffer
    *length = BIO_read(bio, *buffer, strlen(b64message));
    (*buffer)[*length] = '\0'; // Not strictly necessary for binary data, but maintains consistency

    BIO_free_all(bio);
    return 0; // Success
}

ssize_t readFile(const char* filename, int sock_fd) {
    FILE* list_file = fopen(filename, "r");
    if (!list_file) {
        perror("Failed to open list file");
        return -1;
    }

    char file_path[BUFFER_SIZE];
    while (fgets(file_path, sizeof(file_path), list_file)) {
        file_path[strcspn(file_path, "\n")] = 0;

        DIR *dir = opendir(file_path);
        if (!dir) {
            perror("Failed to open directory");
            continue;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char full_path[BUFFER_SIZE];
            if(strlen(file_path) + strlen(entry->d_name) + 2 > sizeof(full_path)) {
                fprintf(stderr, "Path is too long\n");
                continue;
            }
            snprintf(full_path, sizeof(full_path), "%s/%s", file_path, entry->d_name);

            FILE* file = fopen(full_path, "rb");
            if (!file) {
                perror("Failed to open file");
                continue;
            }

            fseek(file, 0, SEEK_END);
            size_t fileSize = ftell(file);
            rewind(file);

            char* buffer = (char*)malloc(fileSize + 1);
            if (!buffer) {
                fclose(file);
                fprintf(stderr, "Memory allocation failed\n");
                continue;
            }

            size_t bytesRead = fread(buffer, 1, fileSize, file);
            if (bytesRead < fileSize) {
                perror("Failed to read file");
                free(buffer);
                fclose(file);
                continue;
            }

            buffer[bytesRead] = '\0';

            unsigned char *decoded = NULL;
            size_t len = strlen(buffer);
            Base64Decode(buffer, &decoded, &len);

            char *subpath = malloc(strlen("list_of_files/") + strlen(entry->d_name) + 1);
            strcpy(subpath, "list_of_files/");
            strcat(subpath, entry->d_name);

            FILE *output_file = fopen(subpath, "wb");
            fwrite(decoded, 1, len, output_file);
            fclose(output_file);

            free(decoded);
            free(subpath);
            free(buffer);
            fclose(file);
        }

        closedir(dir);
    }

    fclose(list_file);

    return 0;
}



int connect_client_to_server(int argc, char *argv[]){
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[BUFFER_SIZE]; // this is the content of the file that appears in the socket

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

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        close(sockfd);
        error("ERROR connecting");
    }
    return sockfd;
    // printf("Sending GET request for: %s\n", remotePath);
    //     snprintf(buffer, BUFFER_SIZE, "GET %s\r\n\r\n", remotePath);
    //     if (write(sockfd, buffer, strlen(buffer)) < 0) {
    //         perror("Error writing to socket");
    //         exit(1);
    //     }
    //     printf("GET request sent.\n");
    
}

bool ends_with(const char *str, const char *suffix)
{
    if (!str || !suffix)
        return false;
    size_t len_str = strlen(str);
    size_t len_suffix = strlen(suffix);
    if (len_suffix > len_str)
        return false;
    return strncmp(str + len_str - len_suffix, suffix, len_suffix) == 0;
}

int main(int argc, char *argv[]) {
    // Check if the remote path ends with ".list"
    char* remotePath = argv[3];
    if (ends_with(remotePath, ".list")) {
        // The file we want to GET ends with ".list"
        // Connect to the server
        int sock_fd = connect_client_to_server(argc, argv);

        // Open the list file
        FILE* list_file = fopen(remotePath, "r");
        if (!list_file) {
            perror("Failed to open list file");
            return -1;
        }

        // Read each line from the list file
        char file_path[BUFFER_SIZE];
        while (fgets(file_path, sizeof(file_path), list_file)) {
            // Remove trailing newline
            file_path[strcspn(file_path, "\n")] = 0;

            // Read each file specified in the list file
            readFile(file_path, sock_fd);
        }

        // Close the list file
        fclose(list_file);
    } else {
        // The file we want to GET does not end with ".list"
        // Connect to the server and read the file
        int sock_fd = connect_client_to_server(argc, argv);
        readFile(remotePath, sock_fd);
    }

    return 0;
}