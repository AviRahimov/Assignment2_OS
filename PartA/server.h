# define BUFFER_SIZE 4096

// This function handles the client requests
void handle_client(int client_socket);

// This function will deoode base 64 encoded to string
int Base64Decode(char* b64message, char** buffer)

// This function will encode string to base 64 encoded
int Base64Encode(const char* message, char** buffer)

// return the length of a decoded base64 string
int calcDecodeLength(const char* b64input)

