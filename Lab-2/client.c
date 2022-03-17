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

int login(char *client_ID, char *password, char *server_ID, char *server_port, int *sockfd);
int logout(int *sockfd);
int joinsession(int *sockfd, char *session_ID);
int leavesession(int *sockfd);
int createsession(int *sockfd, char *session_ID);
int list(int *sockfd);
int quit(int *sockfd);

int logged_in = 0;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
   if (sa->sa_family == AF_INET) {
      return &(((struct sockaddr_in*)sa)->sin_addr);
   }

   return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

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
int login(char *client_ID, char *password, char *server_ID, char *server_port, int *sockfd) {

   // connect to server
   
   // open  socket
   // int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
   int rv;
   struct addrinfo hints,  *servinfo, *p;    // server host & IP address
   char s[INET6_ADDRSTRLEN];
   int num_bytes;

   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;      // IPv4 
   hints.ai_socktype = SOCK_STREAM;

   if ((rv = getaddrinfo(server_ID, server_port, &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return -1;
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
      return -1;
   }

   inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
   printf("client: connecting to %s\n", s);

   freeaddrinfo(servinfo); // all done with this structure

   // send username and password


   // receive LO_ACK
   struct sockaddr_storage server_addr;
   socklen_t server_addr_len = sizeof server_addr;

   char buf[50];
   if ((num_bytes = recvfrom(*sockfd, buf, sizeof(buf), 0, (struct sockaddr *) &server_addr, &server_addr_len)) == -1){
      fprintf(stderr,"Recvfrom error\n");
   }
   else {
      if (strncmp(buf, "yes", 3) == 0) {
         printf("A file transfer can start.\n");
         return 0;
      }
      
   }
  

}

// logout
int logout(int *sockfd) {
   // send logout to server

   // leave session

   // reset username, password, loggedin, sockfd 

   return 0;
}

// join
int joinsession(int *sockfd, char *session_ID) {
   // check if user in session already, only 1 session allowed

   // send join details to server
   
   return 0;
}

// leave
int leavesession(int *sockfd) {
   // check if you're in a session 

   // tell server you want to leave current
   return 0;
}

// create
int createsession(int *sockfd, char *session_ID) {
   // check if you're in a session already

   // tell server you want to create a session
   return 0;
}

// list
int list(int *sockfd) {
   // ask client for lisr of clients and sessions

   // receive list

}

// quit
int quit(int *sockfd) {
   logout(sockfd);
   printf("quit");
   return 0;
}
// text

// old packet code for reference
/*
Packets *prepare_file(char *file_name, int sockfd,  struct addrinfo *servinfo, int num_bytes, struct timeval timeout) {
   FILE *transfer_file;
   transfer_file = fopen(file_name, "rb");
   
   // check if file is corrupt
   // get length of file
   fseek(transfer_file, 0, SEEK_END);
   int transfer_file_size = ftell(transfer_file);
   fseek(transfer_file, 0, SEEK_SET);

   // determine number of fragments of file
   int total_frag = (transfer_file_size/1000) + 1; 

   // length remaining of last frag
   int remainder = transfer_file_size%1000;

   // allocate memory for packets
   char** all_packets_buffer =  malloc(sizeof(char*) * total_frag);

   // create a list of packets, containing all info
   Packets  *previous, *root, *next;
   for (int fragment = 1; fragment <= total_frag; fragment++) { // update max -----------------------------------------
      Packets *current = malloc(sizeof(Packets));
      if (fragment == 1) {
         root = current;
      }     
      else {
         previous->next = current;
      }
      current->total_frag = total_frag;
      
      current->frag_no = fragment;
   
      if (fragment == total_frag) {
         current->size = remainder;
      }
      else {
         current->size = 1000;
      }
      
      current->filename = file_name;
      char file_data[current->size];
      if (fragment != total_frag) {
         fread(file_data, sizeof(char), 1000, transfer_file);
      }
      else {
         fread(file_data, sizeof(char), remainder, transfer_file); 
      }
      memcpy(current->filedata, file_data, current->size);
      
      current->next=NULL;
      previous = current;
   }
   // set up packets
   Packets *packets_current = root;
   
   int packet_num = 1;
   int resend = 1;
   long total_time = 0;
   struct timeval start, end;

   while (packets_current != NULL) {
      char packet_buffer[1100];
      
      int message = sprintf(packet_buffer, "\n%d:%d:%d:%s:", packets_current->total_frag, packets_current->frag_no, packets_current->size, packets_current->filename);
      memcpy(&packet_buffer[message], packets_current->filedata, packets_current->size);

      //gettimeofday(&start, NULL);  //----------------

      num_bytes = sendto(sockfd, packet_buffer, message+packets_current->size, 0, servinfo->ai_addr, servinfo->ai_addrlen);
      //printf("sent packet %d\n", packet_num);

      // receive message from server
      struct sockaddr_from* from_addr;
      socklen_t from_addr_len = sizeof(from_addr);
      char buf[50]; 
   
      //num_bytes = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) &from_addr, &from_addr_len);
      
      int received = 1;

      while (received == 1) {
         if ((num_bytes = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) &from_addr, &from_addr_len)) < 0) {
            // gettimeofday(&end, NULL);  //-----------------

            // total_time = (end.tv_usec - start.tv_usec);
            
            // if (total_time >= timeout.tv_usec) {
               received = 0;
               resend = 1;
               printf("Time Out! Timeout = %lu, packet %d\n", timeout.tv_usec, packet_num);
            // }
            
         }
         else {
            // check if message is "yes" - print "A file transfer can start."
            if (strncmp(buf, "yes", 3) == 0) {
               //printf("received packet %d\n", packet_num);
               packet_num = packet_num + 1;
               received = 0;
               resend = 0;
               total_time = 0;
            }
         }  
      }
      if (resend == 0) {
         packets_current = packets_current -> next;
      }
      
      
   }

   return root;
}
*/