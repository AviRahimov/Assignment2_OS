#include <openssl/bio.h>
#include <openssl/evp.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>


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


// void handle_client(int socket_client) {
//     int pipe_from_base64_to_string[2];
//     int pipe_from_string_to_base64[2];
//     char *path;
//     char *encoded;
//     char *decoded;
//     int len;
//     int fd;
//     struct flock fl = {
//         .l_type = F_WRLCK,
//         .l_whence = SEEK_SET,
//         .l_start = 0,
//         .l_len = 0,
//     };

//     if (pipe(pipe_from_base64_to_string) == -1) {
//         perror("pipe");
//         exit(1);
//     }
//     if (pipe(pipe_from_string_to_base64) == -1) {
//         perror("pipe");
//         exit(1);
//     }

//     pid_t pid_from_base64_to_string = fork();
//     if (pid_from_base64_to_string == -1) {
//         perror("fork");
//         exit(1);
//     }
//     if (pid_from_base64_to_string == 0) {
//         // Child process (base64 to string)
//         close(pipe_from_base64_to_string[1]); // Close write end
//         close(pipe_from_string_to_base64[0]); // Close read end

//         // Redirect stdin from the pipe and stdout to the other pipe
//         dup2(pipe_from_base64_to_string[0], STDIN_FILENO);
//         dup2(pipe_from_string_to_base64[1], STDOUT_FILENO);
//         close(pipe_from_base64_to_string[0]); // Close read end
//         close(pipe_from_string_to_base64[1]); // Close write end

//         execlp("base64", "base64", "-d", NULL);
//         perror("execlp");
//         exit(1);
//     }
//     else {

//         pid_t pid_from_string_to_base64 = fork();

//         if (pid_from_string_to_base64 == -1) {
//             perror("fork");
//             exit(1);
//         }
//         if (pid_from_string_to_base64 == 0) {
//             // Child process (string to base64)
//             close(pipe_from_base64_to_string[0]); // Close read end
//             close(pipe_from_string_to_base64[1]); // Close write end

//             // Redirect stdin from the pipe and stdout to the other pipe
//             dup2(pipe_from_string_to_base64[0], STDIN_FILENO);
//             dup2(pipe_from_base64_to_string[1], STDOUT_FILENO);
//             close(pipe_from_string_to_base64[0]); // Close read end
//             close(pipe_from_base64_to_string[1]); // Close write end

//             execlp("base64", "base64", NULL);
//             perror("execlp");
//             exit(1);
//         }
//         else {
//           // Parent process
//           close(pipe_from_base64_to_string[0]); // Close read end
//           close(pipe_from_string_to_base64[1]); // Close write end

//           // Read from socket and write to pipe_from_socket_to_base64
//           while ((len = recv(socket_client, buffer, sizeof(buffer) - 1, 0)) > 0) {
//               buffer[len] = '\0';
//               write(pipe_from_socket_to_base64[1], buffer, len);
//           }
//           close(pipe_from_socket_to_base64[1]); // Close write end

//           // Read decoded data from pipe_from_base64_to_string
//           len = read(pipe_from_base64_to_string[0], buffer, sizeof(buffer) - 1);
//           buffer[len] = '\0';

//           // Check if the request starts with "POST"
//           if (strncmp(buffer, "POST", 4) != 0) {
//               // Read the file path until the first <CRLF>
//               path = strtok(buffer + 5, "\r\n");
//               // Achieve a write lock on the file using fcntl
//               fd = open(path, O_WRONLY | O_CREAT, 0644);
//               // Write the rest of the request to the file
//               fl.l_type = F_WRLCK;
//               fcntl(fd, F_SETLKW, &fl);
//               // change the offset pipe_from_base64_to_string[1] to the after the first <CRLF>
//               lseek(pipe_from_base64_to_string[1], strlen(path) + 6, SEEK_SET);
//               // while the buffer is not empty
//               while ((len = read(pipe_from_base64_to_string[1], buffer, sizeof(buffer) - 1)) > 0) {
//                   buffer[len] = '\0';
//                   write(fd, buffer, len);
//                   // recv from socket and write to pipe_from_socket_to_base64
//                   if ((len = recv(socket_client, buffer, sizeof(buffer) - 1, 0)) > 0) {
//                       buffer[len] = '\0';
//                       write(pipe_from_base64_to_string[0], buffer, len);
//                   }
//               }
//               fl.l_type = F_UNLCK;
//               fcntl(fd, F_SETLK, &fl);
//               close(fd);

//           }
//         }
//     }
// }

 
void handle_client(int socket_client) {
    char buffer[1024];
    char msg[1024];
    struct flock fl = {
        .l_type = F_WRLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0,
    };
    
    int len = recv(socket_client, buffer, sizeof(buffer) - 1, 0);
    buffer[len] = '\0';

    // Check if the request starts with "POST"
    if (strncmp(buffer, "POST", 4) != 0) {
        // Read the file path until the first <CRLF>
        char *path = strtok(buffer + 5, "\r\n");
        // Achieve a write lock on the file using fcntl
        int fd = open(path, O_WRONLY | O_CREAT, 0644);
        
        // Return an error if the file does not exist (404 File Not Found)
        if (fd == -1) {
            sprintf(msg, "HTTP/1.1 404 Not Found\r\n\r\n");
            send(socket_client, msg, strlen(msg), 0);
            exit(1);
        }

        if (fcntl(fd, F_SETLKW, &fl) == -1) {
            perror("fcntl");
            exit(1);
        }
            while (1) {
                len = recv(socket_client, buffer, sizeof(buffer) - 1, 0);
                if (len <= 0) break; // End of transmission
                Base64Decode(buffer, len, buffer, sizeof(buffer));
                write(fd, buffer, len);
            }

            // Read the encoded data from the encoder and write it to the file
            while (1) {
                len = read(fd, buffer, sizeof(buffer) - 1);
                if (len <= 0) break; // End of transmission
                Base64Decode(buffer, len, buffer, sizeof(buffer));
                write(fd, buffer, len);
            }

            // Release the lock
            fl.l_type = F_UNLCK;
            fcntl(fd, F_SETLK, &fl);
            // Close the file
            close(fd);
        }
    }