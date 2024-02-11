
# define PORT "8080"

// this function is used for extracting the client's IP address
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// This client is sendinding requests to the server asynchronously
// The client is able to send multiple requests to the server without waiting for the response
// Using the poll() function, the client is able to check if the server has responded to any of the requests
int main(int argc, char *argv[]) {
    
}