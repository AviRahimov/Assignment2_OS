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
        char* decodedContent;
        if(Base64Decode((const char*)buffer, &decodedContent, numbytes) != 0){
            perror("Error decoding the content of the file");
            exit(1);
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
void list_file_handler(char *file_path) {

    file1   scok_server1 -> read(sock_server1) -> write(file1 - copy of file1 in the client)
    file2   sock_server2   
    file3   sock_server3

    pfds = pollfd[inf]; // count \n in the file - add a helper function to count the number of lines in the file
    test1\ntest2\ntest3

    read the file line by line
    for each line, create a new socket where the first argument is the host name and the second argument is the file path
    link each socket to poll array

    poll_count (pfds, POLLIN) // with timeout 1 seconds
    poll_count == 0 ==> no data to read for any of the sockets
    poll_count > 0 ==> data to read for at least one of the sockets
    for 1....pfds.size()
        if pfds[i].revents & POLLIN
            read(pfds[i].fd, buffer, BUFFER_SIZE-1)
            write(file_fd, buffer, numbytes)
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