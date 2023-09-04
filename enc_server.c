// By Andrew Osborne 15 March 2023
// Based on provided OSU code and concurrent server design found in
// 'The Linux Programming Interface' p. 1244-5

#define _GNU_SOURCE
#include <stdio.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

static char const alpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

// SIGCHLD handler to reap dead child processes
static void grimReaper(int sig) {
  // Save 'errno' here in case it changes
  int saved_errno;

  saved_errno = errno;
  while (waitpid(-1, NULL, WNOHANG) > 0) {
    continue;
  }
  // reset 'errno' back
  errno = saved_errno;
}

// Error function used for reporting issues
void error(const char *msg) {
  perror(msg);
  exit(1);
} 

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);
  // Allow a client at any address to connect to this server
  address->sin_addr.s_addr = INADDR_ANY;
}

int main(int argc, char *argv[]){
  int connectionSocket;
  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo = sizeof(clientAddress);

  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  sa.sa_handler = grimReaper;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    error("ERROR from sigaction()");
    exit(EXIT_FAILURE);
  }

  // Check usage & args
  if (argc < 2) { 
    fprintf(stderr,"USAGE: %s port\n", argv[0]); 
    exit(1);
  } 
  
  // Create the socket that will listen for connections
  int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocket < 0) {
    error("ERROR opening socket");
  }

  // Set up the address struct for the server socket
  setupAddressStruct(&serverAddress, atoi(argv[1]));

  // Associate the socket to the port
  if (bind(listenSocket, 
          (struct sockaddr *)&serverAddress, 
          sizeof(serverAddress)) < 0){
    error("ERROR on binding");
  }

  // Start listening for connetions. Allow up to 5 connections to queue up
  listen(listenSocket, 5); 
  if (listenSocket == -1) {
    error("ERROR creating server socket");
    exit(EXIT_FAILURE);
  }
  
  for (;;) {
    
    connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
    if (connectionSocket == 0) {
      error("ERROR on accept");
    }

    // Handle each client request in a new child process
    switch (fork()) {
      case -1:
        error("ERROR creating child process");
        close(connectionSocket);
        break;
      case 0:
        close(listenSocket);
        char buffer[3];
        ssize_t num_read;
        char enc_buf[2];
        int a, b, c;

        // First, verify that the client is enc_client
        int verify_request = recv(connectionSocket, buffer, 2, 0);
        if (verify_request < 0) {
          error("ERROR verifying client");
        }
        if (buffer[1] == 69) {
          buffer[0] = 'E';
          int verify_sent = send(connectionSocket, &buffer, sizeof(char), 0);
          if (verify_sent < 0) {
            error("ERROR verifying client (sending)");
          }
        } else {
          exit(EXIT_FAILURE);
        }
        
        while ((num_read = recv(connectionSocket, buffer, 2, 0)) == 2) {
          buffer[2] = '\0';
          // Grab plaintext byte and subtract 65 from the ASCII value to index
          // it into our alpha[] array
          a = buffer[0] - 65;
          if (buffer[0] == 32) {
            a = 26;
          }
          // Grab key byte
          b = buffer[1] - 65;
            if (buffer[1] == 32) {
              b = 26;
            }
          // Encrypt
          c = (a + b) % 27;
          enc_buf[0] = alpha[c];
          enc_buf[1] = '\0';
          int charsWritten = send(connectionSocket, &enc_buf, sizeof(char), 0);
          if (charsWritten != 1) {
            error("ERROR sending data to client");
          }
        }

        if (num_read == -1) {
          error("ERROR from read");
          exit(EXIT_FAILURE);
        }
        _exit(EXIT_SUCCESS);
      default:
        close(connectionSocket);
        break;
    }
  }
}
