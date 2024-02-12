#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <poll.h>
#include <fcntl.h>

#define PORT "8080"  // the port users will be connecting to
#define BUFFER_SIZE 1024

#define BACKLOG 100	 // how many pending connections queue will hold

void sigchld_handler(int s)
{
	(void)s; // quiet unused variable warning

	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
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
    int file_fd = open(file_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (file_fd < 0) {
        perror("Error opening file");
        exit(1);
    }
    while (1) {
        numbytes = recv(sock_fd, buffer, BUFFER_SIZE-1, 0);
        if (numbytes < 0) {
            perror("recv");
            exit(1);
        }
        if (numbytes == 0) {
            break;
        }
        buffer[numbytes] = '\0';
        // if buffer ends with "HTTP/1.1 200 OK <CRLF><CRLF>" then remove it from the buffer and break
        if (ends_with(buffer, "\nHTTP/1.1 200 OK\r\n\r\n")) {
            buffer[numbytes-19] = '\0';
            numbytes -= 19;
        }
        if (write(file_fd, buffer, numbytes) < 0) {
            perror("Error writing to file");
            exit(1);
        }
    }
    close(file_fd);
    printf("File downloaded.\n");
}

// list file handler
// this funciton will sends a get request to the server for each line in the file
// that represent a file path to be downloaded, and then download the file
// using POLL for async IO
// if the file is a regular file, it will be downloaded using the file_handler function
// if not, it will be recursively downloaded using the list_file_handler function
void list_file_handler(char *file_path, int sock_fd) {
    struct pollfd pfds[2];
    char buffer[BUFFER_SIZE];
    int timeout = 5000; // Timeout in milliseconds

    // Open the file, enable creation if it doesn't exist
    int file_fd = open(file_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (file_fd < 0) {
        perror("Error opening file");
        exit(1);
    }

    pfds[0].fd = file_fd;
    pfds[0].events = POLLIN;
    pfds[1].fd = sock_fd;
    pfds[1].events = POLLIN;

    // read the file line by line and send a get request for each line
    while (1) {
        int numbytes;
        int poll_count = poll(pfds, 2, timeout);
        if (poll_count < 0) {
            perror("poll");
            exit(1);
        }
        if (poll_count == 0) {
            printf("Timeout occurred! No data after 5 seconds.\n");
            break;
        }
        if (pfds[0].revents & POLLIN) {
            numbytes = read(file_fd, buffer, BUFFER_SIZE-1);
            if (numbytes < 0) {
                perror("read");
                exit(1);
            }
            if (numbytes == 0) {
                break;
            }
            buffer[numbytes] = '\0';
            // remove the newline character
            buffer[numbytes-1] = '\0';
            printf("Received file path: %s\n", buffer);
            // / create new socket file using open for the new host name, but use the same port
            // TO DO
            // ....
            // clone the socket file descriptor and connect to the new host
            close (sock_fd);
            int new_sock_fd = sock_fd;
            char *first_word = strtok(buffer, " ");
            // connect to the server which is address in the first word of the file
            // using DNS lookup to get the address
            struct addrinfo hints, *servinfo, *p;
            int rv;
            memset(&hints, 0, sizeof hints);
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            if ((rv = getaddrinfo(first_word, PORT, &hints, &servinfo)) != 0) {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                exit(1);
            }
            for(p = servinfo; p != NULL; p = p->ai_next) {
                if ((new_sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                    perror("Error creating socket");
                    continue;
                }
                // connect to the server
                if (connect(new_sock_fd, p->ai_addr, p->ai_addrlen) < 0) {
                    close(new_sock_fd);
                    perror("Error connecting to server");
                    continue;
                }
                // read buffer from the server after 'null' terminator
                char *file_path = buffer + strlen(first_word) + 1;
                if (ends_with(buffer, ".list")) {
                    list_file_handler(file_path, new_sock_fd);
                } else {
                    file_handler(file_path, new_sock_fd);
                }
            }
            freeaddrinfo(servinfo);
        }
    }
}

// post request handler
// this function will send a post request to the server
void post_request_handler (char * file_path, int sock_fd) {
    char buffer [BUFFER_SIZE];
    int numbytes;
    printf("Sending POST request for: %s\n", file_path);
    snprintf(buffer, BUFFER_SIZE, "POST %s\r\n\r\n", file_path);
    if (write(sock_fd, buffer, strlen(buffer)) < 0) {
        perror("Error writing to socket");
        exit(1);
    }
    printf("POST request sent.\n");
    // read the response from the server
    numbytes = recv(sock_fd, buffer, BUFFER_SIZE-1, 0);
    if (numbytes < 0) {
        perror("recv");
        exit(1);
    }
    buffer[numbytes] = '\0';
    printf("Received response: %s\n", buffer);
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
            list_file_handler(remotePath, sockfd);
        }
    }
    else if (strcmp(operation, "POST") == 0) {
        post_request_handler(remotePath, sockfd);
    } 
    else {
        printf("Invalid operation\n");
    }
		
	close(sockfd);

	return 0;
}