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
#include "packet.h"

Packets *prepare_file(char *file_name, int sockfd, struct addrinfo *servinfo, int num_bytes);


int main(int argc, char *argv[]) {
   
   // check number of arguments
   if(argc != 3){
      fprintf(stderr,"usage: deliver <server address> <server port number>\n");
      return 0;
   }
   
   // open  socket
   int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
   
   struct addrinfo hints,  *servinfo;    // server host & IP address
   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_INET;      // IPv4 
   hints.ai_socktype = SOCK_DGRAM;

   // get server info using "ug###", exit if issues occur
   if (getaddrinfo(argv[1], argv[2], &hints, &servinfo) !=0){
      fprintf(stderr,"Getaddrinfo error\n");
      return 0;
   }

   // ask user for file in particular format  
   printf("Input a message of the following form:\n");
   printf("        ftp <file name>\n");

   // read what the user inputted
   char type[50];
   char file_name[50];
   scanf("%s", type);
   scanf("%s", file_name);

   // check if a file exists
   if (strcmp(type, "ftp") != 0) {
      fprintf(stderr, "Only type ftp accepted\n");
      return 0;
   }
   else {
      // doesnt exist: exit
      if (access(file_name, F_OK) == -1) {
         fprintf(stderr,"File doesn't exist\n");
         return 0;
      }
      // exists: send "ftp" to server
      else {
         printf("File exists\n");
      }
   }

   // send "ftp" to server
   int num_bytes;
   num_bytes = sendto(sockfd, "ftp\n", 3, 0, servinfo->ai_addr, servinfo->ai_addrlen);
   
   if (num_bytes < 0) {
      fprintf(stderr,"Sendto error\n");
         return 0;
   }

   // receive message from server
   struct sockaddr_from* from_addr;
   socklen_t from_addr_len = sizeof(from_addr);
   char buf[50]; 
   
   num_bytes = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) from_addr, &from_addr_len);

   if (num_bytes < 0) {
      fprintf(stderr,"Recvfrom error\n");
      return 0;
   }

   if (num_bytes > 0) {
      // check if message is "yes" - print "A file transfer can start."
      if (strncmp(buf, "yes", 3) == 0) {
         printf("A file transfer can start.\n");
      }
   }

   // send packets
   prepare_file(file_name, sockfd, servinfo, num_bytes);
   
   close(sockfd);

   return 0;
}

Packets *prepare_file(char *file_name, int sockfd,  struct addrinfo *servinfo, int num_bytes) {
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

   for (int fragment = 1; fragment <= total_frag; fragment++) { 
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
      printf("current fragment size: %d\n", current->size); //-----
      
      current->filename = file_name;

      char file_data[current->size];

      if (fragment != total_frag) {
         fread(file_data, sizeof(char), 1000, transfer_file);
      }
      else {
         fread(file_data, sizeof(char), remainder, transfer_file); 
      }
      memcpy(current->filedata, file_data, current->size);

      current->next = NULL;
      previous = current;
   }

   Packets *packets_current = root;

   while (packets_current != NULL) {
      char packet_buffer[1100];
      
      int message = sprintf(packet_buffer, "\n%d:%d:%d:%s:", packets_current->total_frag, packets_current->frag_no, packets_current->size, packets_current->filename);
      memcpy(&packet_buffer[message], packets_current->filedata, packets_current->size);
      printf("%s\n", packet_buffer);

      num_bytes = sendto(sockfd, packet_buffer, message+packets_current->size, 0, servinfo->ai_addr, servinfo->ai_addrlen);
      printf("sent\n");

      // receive message from server
      struct sockaddr_from* from_addr;
      socklen_t from_addr_len = sizeof(from_addr);
      char buf[50]; 
   
      num_bytes = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) from_addr, &from_addr_len);
      
      if (num_bytes > 0) {
         printf("received\n");
         num_bytes = 0;
      }

      packets_current = packets_current -> next;
   }


   return root;
}
