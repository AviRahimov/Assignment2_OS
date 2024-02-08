#include <openssl/bio.h> //BIO, BIO_new, BIO_free_all, BIO_push, BIO_set_flags, BIO_read, BIO_write, BIO_flush
#include <openssl/evp.h> //EVP_DecodeBlock
#include <string.h> //strlen
#include <stdio.h> //FILE, fmemopen, fclose
#include <math.h> //ceil
#include <stdlib.h> //malloc and free
#include <unistd.h> 
#include <netinet/in.h> // sockaddr_in
#include <sys/socket.h> // socket functions
#include <sys/types.h> // socket types
#include <sys/wait.h> // waitpid and WNOHANG
#include <signal.h> // signal
#include <errno.h> // errno 
#include <arpa/inet.h> // inet_ntop and inet_pton
#include <netdb.h> // getnameinfo and getaddrinfo
#include <fcntl.h> // fcntl


# define PORT "8080" // Default port for the server
# define BACKLOG 100 // Maximum number of pending connections for TCP socket

int Base64Encode(const char* message, char** buffer) { //Encodes a string to base64
  BIO *bio, *b64;
  FILE* stream;
  int encodedSize = 4*ceil((double)strlen(message)/3);
  *buffer = (char *)malloc(encodedSize+1);

  stream = fmemopen(*buffer, encodedSize+1, "w");
  b64 = BIO_new(BIO_f_base64());
  bio = BIO_new_fp(stream, BIO_NOCLOSE);
  bio = BIO_push(b64, bio);
  BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Ignore newlines - write everything in one line
  BIO_write(bio, message, strlen(message));
  BIO_flush(bio);
  BIO_free_all(bio);
  fclose(stream);

  return (0); //success
}

int calcDecodeLength(const char* b64input) { //Calculates the length of a decoded base64 string
  int len = strlen(b64input);
  int padding = 0;

  if (b64input[len-1] == '=' && b64input[len-2] == '=') //last two chars are =
    padding = 2;
  else if (b64input[len-1] == '=') //last char is =
    padding = 1;

  return (int)len*0.75 - padding;
}

int Base64Decode(char* b64message, char** buffer) { //Decodes a base64 encoded string
  BIO *bio, *b64;
  int decodeLen = calcDecodeLength(b64message),
      len = 0;
  *buffer = (char*)malloc(decodeLen+1);
  FILE* stream = fmemopen(b64message, strlen(b64message), "r");

  b64 = BIO_new(BIO_f_base64());
  bio = BIO_new_fp(stream, BIO_NOCLOSE);
  bio = BIO_push(b64, bio);
  BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Do not use newlines to flush buffer
  len = BIO_read(bio, *buffer, strlen(b64message));
    //Can test here if len == decodeLen - if not, then return an error
  (*buffer)[len] = '\0';

  BIO_free_all(bio);
  fclose(stream);

  return (0); //success
}
 
void handle_client(int socket_client, char * home_path) {
    char * encoded_str;
    char * decoded_str;
    char buffer[1024];
    char * data;
    char msg[1024];
    struct flock fl = {
        .l_type = F_WRLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0,
    };
    
    int len = recv(socket_client, buffer, sizeof(buffer) - 1, 0);
    buffer[len] = '\0';
    printf("Request: %s\n", buffer);
    // Check if the request starts with "POST"
    if (strncmp(buffer, "POST", 4) == 0) {
        // Read the file path until the first <CRLF>
        char *path = strtok(buffer + 5, "\r\n");
        char * file_path = (char *) malloc(strlen(home_path) + strlen(path) + 1);
        file_path[0] = '\0';
        strcat(file_path, home_path);
        strcat(file_path, path);
        printf("File path: %s\n", file_path);
        // Achieve a write lock on the file using fcntl
        int fd = open(file_path, O_WRONLY | O_CREAT, 0644);
        // Return an error if the file does not exist (404 File Not Found)
        if (fd == -1) {
            sprintf(msg, "HTTP/1.1 404 Not Found\r\n\r\n");
            send(socket_client, msg, strlen(msg), 0);
            exit(1);
        }
        if (fcntl(fd, F_SETLKW, &fl) == -1) {
            sprintf(msg, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
            send(socket_client, msg, strlen(msg), 0);
            exit(1);
        }

        // read the data from the buffer, start from the first <CRLF> after the first <CRLF>
        data = buffer + 5 + strlen(path) + 2; // 5 is the length of "POST ", 2 is the length of <CRLF>
        len = len - (data - buffer); // Adjust len based on the new start position
        
        while (1) {
            // Ensure null termination before decoding
            data[len] = '\0';
            // Check if the buffer ends with "\r\n\r\n" then break, not included in the file
            if ((len >=4) && strcmp(data + len - 4, "\r\n\r\n") == 0) {
                len -= 4;
                data[len] = '\0';
                Base64Decode(data, &decoded_str); // Assuming this function allocates memory for decoded_str
                write(fd, decoded_str, strlen(decoded_str)); // Use strlen(decoded_str) to get the correct length
                free(decoded_str); // Free decoded_str after use
                break;
            }
            else {
                Base64Decode(data, &decoded_str); // Assuming this function allocates memory for decoded_str
                write(fd, decoded_str, strlen(decoded_str)); // Use strlen(decoded_str) to get the correct length
                free(decoded_str); // Free decoded_str after use
                len = recv(socket_client, buffer, sizeof(buffer) - 1, 0);
                data = buffer; // Reset pointer to the start of the buffer for new data
            }
        }

        // Release the lock
        fl.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &fl);
        // free the memory
        free(file_path);
        // Close the file
        close(fd);
    }
    else if (strncmp(buffer, "GET", 3) == 0) {
        // Read the file path until the first <CRLF>
        char *path = strtok(buffer + 4, "\r\n\r\n");
        // concatenate the home path with the file path
        char * file_path = (char *) malloc(strlen(home_path) + strlen(path) + 1);
        file_path[0] = '\0';
        strcat(file_path, home_path);
        strcat(file_path, path);        
        // open the file in read-only mode, if it does not exist, return an error
        int fd = open(file_path, O_RDONLY);
        // Return an error if the file does not exist (404 File Not Found)
        if (fd == -1) {
            sprintf(msg, "HTTP/1.1 404 Not Found\r\n\r\n");
            perror("open");
            send(socket_client, msg, strlen(msg), 0);
            exit(1);
        }
        // put a read lock on the file
        fl.l_type = F_RDLCK;
        if (fcntl(fd, F_SETLKW, &fl) == -1) {
            sprintf(msg, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
            perror("fcntl");
            send(socket_client, msg, strlen(msg), 0);
            exit(1);
        }
        while (1) {
            len = read(fd, buffer, sizeof(buffer) - 1);
            if (len <= 0) break; // End of transmission
            Base64Encode(buffer, &encoded_str);
            len = strlen(encoded_str);
            send(socket_client, encoded_str, len, 0);
        }

        // Release the lock
        fl.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &fl);
        // free the memory
        free(file_path);
        // Close the file
        close(fd);
    }
    else {
        sprintf(msg, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
        printf("Invalid request\n");
        send(socket_client, msg, strlen(msg), 0);
        exit(1);
    }
    // send the response to the client
    sprintf(msg, "HTTP/1.1 200 OK\r\n\r\n");
    send(socket_client, msg, strlen(msg), 0);
  }

  // this function is used for extracting the client's IP address
  void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
      return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
  }

  // this function use for SIGCHLD signal to handle child process to prevent zombie process and
  // allow the child process to be terminated properly
  void sigchld_handler(int s) {
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
  }

  int main (int argc, char * argv []) {
    int socket_server, socket_client;
    struct addrinfo hints, *serv_info, *p;
    struct sockaddr_storage client_addr;
    socklen_t addr_size; // sizeof(client_addr);
    struct sigaction sa; // Signal handler defined for the child process to prevent zombie processes
    int yes = 1;
    char ipstr[INET6_ADDRSTRLEN]; // String to store the client's IP address
    int rv;

    // get from the command line argument the home directory
    if (argc != 2) {
      fprintf(stderr, "usage: ./server <home_directory>\n");
      exit(1);
    }

    char * home_path = argv[1];

    memset(&hints, 0, sizeof(hints)); // allocate memory for hints
    hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // for TCP
    hints.ai_flags = AI_PASSIVE; // use my IP

    // Search for the server's address information
    if ((rv = getaddrinfo(NULL, PORT, &hints, &serv_info)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // Loop through all the results and bind to the first we can
    for (p = serv_info; p != NULL; p = p->ai_next) {
        if ((socket_server = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(socket_server, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(socket_server, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_server);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(serv_info); // destroy the linked list

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(socket_server, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    // change the signal handler to sigchld_handler to prevent zombie processes
    if (signal(SIGCHLD, sigchld_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {
        addr_size = sizeof(client_addr);
        socket_client = accept(socket_server, (struct sockaddr *)&client_addr, &addr_size);
        if (socket_client == -1) {
            perror("accept");
            continue;
        }
        
        // extract the client's IP address and print it
        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), ipstr, sizeof(ipstr));
        printf("server: got connection from %s\n", ipstr);

        // if (!fork()) { // this is the child process
        //     close(socket_server); // child doesn't need the listener
        //     handle_client(socket_client, home_path);
        //     close(socket_client);
        //     exit(0);
        // }
        handle_client(socket_client, home_path);
        close(socket_client); // parent doesn't need this
    }
    return 0;
  }