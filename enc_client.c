// By Andrew Osborne 3/18/23
// Based on boilerplate code provided by OSU

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()
#include <ctype.h>

/**
* Client code
* 1. Create a socket and connect to the server specified in the command arugments.
* 2. Prompt the user for input and send that input as a message to the server.
* 3. Print the message received from the server and exit the program.
*/

// Error function used for reporting issues
void error(const char *msg) { 
  perror(msg); 
  exit(0); 
} 

// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber, 
                        char* hostname){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  // AF_INET = Address Family IPv4
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);

  // Get the DNS entry for this host name
  struct hostent* hostInfo = gethostbyname(hostname); 
  if (hostInfo == NULL) { 
    fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
    exit(0); 
  }
  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, 
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}

int main(int argc, char *argv[]) {
  int socketFD, charsWritten, charsRead;
  struct sockaddr_in serverAddress;
  FILE* fp_pt; 
  FILE* fp_key;
  int plain_sz, key_sz;
  char buffer[2];
  char buf_key[2];
  char two_bit_buf[3];

  // Check usage & args
  if (argc < 4) { 
    fprintf(stderr, "USAGE: %s plaintext key port\n", argv[0]);
    exit(0); 
  }

  // Open plaintext and mykey files and measure sizes
  fp_pt = fopen(argv[1], "r");
  fseek(fp_pt, sizeof(char), SEEK_END);
  plain_sz = ftell(fp_pt);
  rewind(fp_pt);
  fp_key = fopen(argv[2], "r");
  fseek(fp_key, sizeof(char), SEEK_END);
  key_sz = ftell(fp_key);
  rewind(fp_key);
  // Key cannot be shorter than the plaintext file!
  if (key_sz < plain_sz) {
    fprintf(stderr, "Invalid key: Key too short\n");
    exit(1);
  }
  
  // Run through file and check for invalid chars
  // Only allow 'ABCDEF..Z' and ' ' (space)
  int c;
  while ((c = fgetc(fp_pt)) != EOF && c != 10) {
    if (isupper(c) == 0) {
      if (isspace(c) == 0) error("CLIENT: Bad characters found");
    }
  }
  rewind(fp_pt);

  // Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); 
  if (socketFD < 0){
    error("CLIENT: ERROR opening socket");
  }

   // Set up the server address struct
  setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

  // Connect to server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    error("CLIENT: ERROR connecting");
  }

  // Handshake:
  // Only allow connection with enc_server
  buffer[0] = '*';
  buffer[1] = 'E';
  int verify_sent = send(socketFD, buffer, sizeof(char)*2, 0);
  if (verify_sent < 0) {
    error("CLIENT: ERROR connecting");
  }
  int verify_read = recv(socketFD, buffer, sizeof(buffer)-1, 0);
  if (verify_read < 1) {
    error("CLIENT: ERROR verifing");
  }
  if (buffer[0] != 69) {
    error("CLIENT: ERROR cannot use this server");
  }

  // Clear out the buffer array
  memset(buffer, '\0', sizeof(buffer));

  // Read 1 char from plaintext and 1 char from mykey. These are 
  // combined into a buffer (two_bit_buf) and sent to the enc_server
  // Expected return is 1 char of encrypted data
  while (fread(two_bit_buf, sizeof(char), 1, fp_pt) == 1) {
    if (strncmp(two_bit_buf, "\n", 1) == 0) break;
    fread(buf_key, sizeof(char), 1, fp_key);
    two_bit_buf[1] = *buf_key;
    two_bit_buf[2] = '\0';
    charsWritten = send(socketFD, two_bit_buf, sizeof(char)*2, 0);
    charsRead = recv(socketFD, buffer, sizeof(buffer)-1, 0);
    if (charsRead < 1) {
      error("CLIENT: ERROR reading from socket");
    }
    printf("%s", buffer);
    memset(buffer, '\0', sizeof(buffer));
    memset(two_bit_buf, '\0', sizeof(two_bit_buf));
    memset(buf_key, '\0', sizeof(buf_key));

    if (charsWritten < 0){
      error("CLIENT: ERROR writing to socket");
    }
    if (charsWritten < strlen(buffer)){
      printf("CLIENT: WARNING: Not all data written to socket!\n");
    }
  }
  printf("\n");
  // Close the socket
  close(socketFD); 
  return 0;
}
