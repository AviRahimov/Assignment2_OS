#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/bio.h> 
#include <openssl/evp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <poll.h>
#include <fcntl.h>
#include <math.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <stdbool.h>
#include <sys/stat.h>

#define PORT "8080"  // the port users will be connecting to
#define BUFFER_SIZE 1024

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
int Base64Decode(char* b64message, char** buffer, size_t* length) {
    BIO *bio, *b64;
    // Calculate the expected decode length based on the actual length of the Base64 input
    int decodeLen = calcDecodeLength(b64message, * length);
    *buffer = (char*)malloc(decodeLen + 1); // Allocate enough memory for the decoded data
    if (*buffer == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }

    FILE* stream = fmemopen(b64message, * length, "r"); // Use actual length of b64message
    if (!stream) {
        free(*buffer);
        fprintf(stderr, "Failed to open stream\n");
        return -2;
    }

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_fp(stream, BIO_NOCLOSE);
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // Do not use newlines to flush buffer

    // Perform the decoding
    int len = BIO_read(bio, *buffer, decodeLen); // Attempt to read up to decodeLen bytes
    if (len <= 0) {
        free(*buffer);
        BIO_free_all(bio);
        fclose(stream);
        fprintf(stderr, "Decoding failed\n");
        return -3;
    }

    (*buffer)[len] = '\0'; // Null-terminate the decoded output
    *length = len; // Update the actual length of the decoded data

    BIO_free_all(bio);
    fclose(stream);

    return 0; // Indicate success
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
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

bool handle_response(char *line) {
    if (strstr(line, "500 Internal Server Error") != NULL) {
        printf("File found.\n");
    } else
    if (strstr(line, "404 Not Found") != NULL) {
        printf("File not found.\n");
        return false;
    }
    return true;
}

// this function will handle the file download
// without using POLL
void file_handler (char * file_path, int sock_fd) {
    int numbytes;
    char buffer [BUFFER_SIZE];
    printf("Sending GET request for: %s\n", file_path);
    snprintf(buffer, BUFFER_SIZE, "GET %s\r\n\r\n", file_path);
    if (write(sock_fd, buffer, strlen(buffer)) < 0) {
        perror("Error writing to socket");
        exit(1);
    }
    printf("GET request sent.\n");
    // read the response from the server and write it to the file in chunks loop
    // open the file for writing, and enable creation if it doesn't exist
    // makedir
    char *dir = strdup(file_path);
    char *last_slash = strrchr(dir, '/');
    if (last_slash != NULL) {
        *last_slash = '\0';
        if (mkdir(dir, 0777) < 0) {
            if (errno != EEXIST) {
                perror("Error creating directory");
                exit(1);
            }
        }
    }
    int file_fd = open(file_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (file_fd < 0) {
        perror("Error opening file");
        exit(1);
    }
    while (1) {
        numbytes = recv(sock_fd, buffer, BUFFER_SIZE, 0);
        if (numbytes < 0) {
            perror("recv");
            exit(1);
        }
        if (numbytes == 0) {
            break;
        }
        if (!handle_response(buffer)) {
            return;
        }
        //buffer[numbytes] = '\0';
        char* decodedContent;
        size_t decodedLength = numbytes;
        if(Base64Decode((char*)buffer, &decodedContent, &decodedLength) != 0){
            perror("Error decoding the content of the file");
            exit(1);
        }
        if (write(file_fd, decodedContent, decodedLength) < 0) {
            perror("Error writing to file");
            exit(1);
        }
    }
    close(file_fd);
    printf("File downloaded.\n");
}

int count_lines(const char *file_path) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        perror("Error opening file for line count");
        return -1;
    }
    int lines = 0;
    int ch;
    while (EOF != (ch = getc(file))) {
        if (ch == '\n') {
            lines++;
        }
    }
    fclose(file);
    return (lines + 1);
}

// Helper function to establish a socket connection based on a parsed line
int create_socket_from_line(const char *line) {
    char host[1024]; // Adjust size as necessary
    struct hostent *server;
    struct sockaddr_in serv_addr;
    int sockfd;
    // convert PORT to int
    int portno = atoi(PORT);

    // Copy the line to a local buffer to avoid modifying the original line with strtok
    char buffer[1024];
    strncpy(buffer, line, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    // Extract the host part
    char *host_part = strtok(buffer, " ");
    if (!host_part) {
        perror("Invalid line format: Host missing");
        return -1;
    }

    // Creating a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Cannot create socket");
        return -1;
    }

    // Getting the host address
    server = gethostbyname(host_part);
    if (!server) {
        fprintf(stderr, "ERROR, no such host\n");
        close(sockfd);
        return -1;
    }

    // Setting up the server address structure
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    // Connecting to the server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        close(sockfd);
        return -1;
    }

    // At this point, the socket is created and connected to the server.
    // You can now use `sockfd` to send and receive data.
    return sockfd;
}

// list file handler
// this funciton will sends a get request to the server for each line in the file
// that represent a file path to be downloaded, and then download the file
// using POLL for async IO
// if the file is a regular file, it will be downloaded using the file_handler function
// if not, it will be recursively downloaded using the list_file_handler function
void list_file_handler(char *file_path) {
    int lines = count_lines(file_path);
    char buffer[BUFFER_SIZE];
    if (lines <= 0) {
        return;
    }
    struct pollfd pfds[lines];
    int file_fd = open(file_path, O_RDONLY);
    if (file_fd < 0) {
        perror("Error opening file");
        return;
    }
    FILE *file = fopen(file_path, "r");
    char line[BUFFER_SIZE];
    int files [BUFFER_SIZE];
    int i = 0;
    while (fgets(line, sizeof(line), file)) {
        int sockfd = create_socket_from_line(line);
        if (sockfd < 0) {
            // Error creating socket
            perror("Error creating socket");
            exit(1);
        }
        pfds[i].fd = sockfd;
        pfds[i].events = POLLIN;
        // send the GET request
        char *file_path = strstr(line, " ") + 1;
        // if the file path ends with a newline, remove it
        if (file_path[strlen(file_path) - 1] == '\n') {
            file_path[strlen(file_path) - 1] = '\0';
        }
        else
        {
            file_path[strlen(file_path)] = '\0';
        }
        

        printf("Sending GET request for: %s\n", file_path);
        snprintf(buffer, BUFFER_SIZE, "GET %s\r\n\r\n", file_path);
        if (write(sockfd, buffer, strlen(buffer)) < 0) {
            perror("Error writing to socket");
            exit(1);
        }
        // makedir
        char *dir = strdup(file_path);
        char *last_slash = strrchr(dir, '/');
        if (last_slash) {
            *last_slash = '\0';
            if (mkdir(dir, 0777) < 0 && errno != EEXIST) {
                perror("Error creating directory");
                exit(1);
            }
        }
        files[i] = open(file_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        // create a new file for writing
        // enable creating folders
        if (files[i] < 0) {
            perror("Error creating file");
            exit(1);
        }
        free(dir);
        i++;
    }
    fclose(file);
    while (poll(pfds, lines, 1000) > 0) {
        for (int i = 0; i < lines; i++) {
            if (pfds[i].revents & POLLIN) {
                int numbytes = recv(pfds[i].fd, buffer, BUFFER_SIZE, 0);
                if (numbytes > 0) {
                    char * decodedContent;
                    size_t decodedLength;
                    decodedLength = numbytes;
                    if(Base64Decode((char*)buffer, &decodedContent, &decodedLength) != 0){
                        perror("Error decoding the content of the file");
                        exit(1);
                    }
                    // write the response to the file
                    if (write(files[i], decodedContent, decodedLength) < 0) {
                        perror("Error writing to file");
                        exit(1);
                    }
                }
                else if (numbytes == 0) {
                    // close the socket
                    close(pfds[i].fd);
                    pfds[i].fd = -1;
                }
            }
        }
    }

    // Close all sockets
    for (int i = 0; i < lines; i++) {
        if (pfds[i].fd >= 0) {
            close(pfds[i].fd);
        }
        close(files[i]);
    }
}

// post request handler
// this function will send a post request to the server
void post_request_handler (char * local_file_path, char * remote_path, int sock_fd) {
    char buffer[BUFFER_SIZE];
     // Open the file for reading
    int file = open(local_file_path, O_RDONLY);
    if (file == -1) {
        perror("Error opening file");
        exit(1);
    }

    // Prepare and send the POST request line
    snprintf(buffer, BUFFER_SIZE, "POST %s\r\n", remote_path);
    if (write(sock_fd, buffer, strlen(buffer)) < 0) {
        perror("Error writing initial POST line to socket");
        close(file);
        exit(1);
    }

    // Initialize buffer for reading file content
    char fileBuffer[BUFFER_SIZE];
    size_t bytesRead;
    while ((bytesRead = read(file, fileBuffer, BUFFER_SIZE - 1)) > 0) {
        size_t encodedSize;
        char* encodedContent;
        fileBuffer[bytesRead] = '\0'; // Null-terminate the buffer
        Base64Encode((const char*)fileBuffer, &encodedContent, bytesRead);
        encodedSize = strlen(encodedContent);
        printf("Sending chunk of size %zu\n", encodedSize);

        // Directly write the encoded content to the socket
        if (write(sock_fd, encodedContent, encodedSize) < 0) {
            perror("Error writing encoded chunk to socket");
            close(file);
            free(encodedContent);
            exit(1);
        }

        // Free the encoded content after sending
        free(encodedContent);
    }

    // Send the final CRLF to indicate the end of the request
    if (write(sock_fd, "\r\n\r\n", 4) < 0) {
        perror("Error writing final CRLF to socket");
        close(file);
        exit(1);
    }

    close(file); // Close the file after sending all chunks
}

int main(int argc, char *argv[]) {
    int sockfd, numbytes;  
	char buffer[BUFFER_SIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
    char * file_path;
    int new_fd;

    if (argc < 4) {
       fprintf(stderr,"usage %s hostname operation[GET/POST] remote_path [local_path_for_POST]\n", argv[0]);
       exit(1);
    }

    char* operation = argv[2];
    char* remotePath = argv[3];
    char* localPath = argc == 5 ? argv[4] : NULL;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure

    if (strcmp(operation, "GET") == 0) {
        file_handler(remotePath, sockfd);
        // check if the file is a regular file
        if (ends_with(remotePath, ".list")) {
            // the file is a list file
            // recursively download the file
            list_file_handler(remotePath);
        }
    }
    else if (strcmp(operation, "POST") == 0) {
        post_request_handler(localPath, remotePath, sockfd);
    } 
    else {
        printf("Invalid operation\n");
    }
		
	close(sockfd);

	return 0;
}