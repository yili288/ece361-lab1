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
   char fileName[50];
   scanf("%s", type);
   scanf("%s", fileName);

   // check if a file exists
   if (strcmp(type, "ftp") != 0) {
      fprintf(stderr, "Only type ftp accepted\n");
      return 0;
   }
   else {
      // doesnt exist: exit
      if (access(fileName, F_OK) == -1) {
         fprintf(stderr,"File doesn't exist\n");
         return 0;
      }
      // exists: send "ftp" to server
      else {
         printf("File exists\n");
      }
   }

   // send "ftp" to server
   int numbytes;
   numbytes = sendto(sockfd, "ftp\n", 3, 0, servinfo->ai_addr, servinfo->ai_addrlen);
   
   if (numbytes < 0) {
      fprintf(stderr,"Sendto error\n");
         return 0;
   }

   // receive message from server
   struct sockaddr_from* from_addr;
   socklen_t from_addr_len = sizeof(from_addr);
   char buf[50]; 
   
   numbytes = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) from_addr, &from_addr_len);

   if (numbytes < 0) {
      fprintf(stderr,"Recvfrom error\n");
      return 0;
   }

   if(numbytes > 0) {
      // check if message is "yes" - print "A file transfer can start."
      if (strncmp(buf, "yes", 3) == 0) {
         printf("A file transfer can start.\n");
      }
   }

   close(sockfd);

   return 0;
}
