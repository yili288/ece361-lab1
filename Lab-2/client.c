// utilized Beej's Guide (pg. 40) for assistance with coding some parts
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "message.h"
#include <time.h>

void login(char client_ID, char password, char server_ID, char server_port, int *sockfd);

int logged_in = 0;

int main(int argc, char *argv[]) {

   // global socket
   int sockfd;

   // get command   
   char command[MAX_GENRAL]; 
   scanf("%s", command);

   // store command related info, send info to dedicated functions
   
   // login   
   if (strcmp(command, "/login") == 0 && logged_in == 0) {
      char client_ID[MAX_GENRAL];
      char password[MAX_GENRAL];
      char server_IP[MAX_GENRAL];
      char server_port[MAX_GENRAL]; 
      scanf("%s %s %s %s", client_ID, password, server_IP, server_port);
      printf("login \n");
      login(client_ID, password, server_IP, server_port, &sockfd);
   }
   
   // logout
   else if (strcmp(command, "/logout") == 0) {
      logged_in = 0;
      printf("logout \n");
   }
   
   // join
   else if (strcmp(command, "/joinsession") == 0) {
      char session_ID[MAX_GENRAL]; 
      scanf("%s", session_ID);
      printf("join \n");
   }

   //leave
   else if (strcmp(command, "/leavesession") == 0) {
      printf("leave \n");
   }

   // create
   else if (strcmp(command, "/createsession") == 0) {
      char session_ID[MAX_GENRAL]; 
      scanf("%s", session_ID);
      printf("create \n");
   }

   // list
   else if (strcmp(command, "/list") == 0) {
      printf("list \n");
   }

   // quit
   else if (strcmp(command, "/quit") == 0) {
      printf("quit \n");
   }

   // text
   else {
      char text[MAX_DATA]; 
      scanf("%s", text);
      printf("text \n");
   }

   // send length of message
   // send message
   
   return 0;
}

//login
void login(char client_ID, char password, char server_ID, char server_port, int *sockfd) {

   // connect to server
   
   // open  socket
   // int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
   int rv;
   struct addrinfo hints,  *servinfo, *p;    // server host & IP address
   char s[INET6_ADDRSTRLEN];

   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;      // IPv4 
   hints.ai_socktype = SOCK_STREAM;

   if ((rv = getaddrinfo(server_ID, server_port, &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return;
   }

   // loop through all the results and connect to the first we can
   for(p = servinfo; p != NULL; p = p->ai_next) {
      if ((*sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
         perror("client: socket");
      continue;
      }
      if (connect(*sockfd, p->ai_addr, p->ai_addrlen) == -1) {
         perror("client: connect");
         close(*sockfd);
      continue;
      }
      break; // if we get here, we must have connected successfully
   }

   if (p == NULL) {
   fprintf(stderr, "client: failed to connect\n");
   return 2;
   }

   inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
   printf("client: connecting to %s\n", s);

   freeaddrinfo(servinfo); // all done with this structure



}

// logout


// join

// leave

// create

// list

// quit

// text