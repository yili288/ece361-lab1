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

/* TESTING
/login a aa 128.100.13.250 1055
*/

int login(char *server_ID, char *server_port);
int logout();
int joinsession();
int leavesession();
int createsession(char * new_session_name);
int list_all();
int quit();
int send_data (struct message packet);
struct message stringToPacket(char * buffer);
void receive(int sent_type);

int logged_in = 0;
int current_socket = -1;
struct addrinfo *servinfo = {0};

char client_ID[MAX_GENRAL]  = "0";
char password[MAX_GENRAL]   = "0";
char session_ID[MAX_GENRAL] = "0"; 

fd_set master;    // master file descriptor list
fd_set read_fds;  // temp file descriptor list for select()
int fdmax;        // maximum file descriptor number

int newfd;        // newly accept()ed socket descriptor
struct sockaddr_storage remoteaddr; // client address
socklen_t addrlen;

char buf[256];    // buffer for client data
int nbytes;

char remoteIP[AF_INET];

int yes=1;        // for setsockopt() SO_REUSEADDR, below
int i, j, rv;

struct addrinfo hints, *ai, *p;

int through;

int main(int argc, char *argv[]) {
   // initial message
   printf("To begin text conferencing, you must login as follows \n");
   printf("/login <client ID> <password> <server-IP> <server-port>\n");

   FD_ZERO(&master);    // clear the master and temp sets
   FD_ZERO(&read_fds);

   struct timeval tv;

   tv.tv_sec = 2;
   tv.tv_usec = 500000;

   // loop until exit 
   while (1) {
      // get command   
      char command[MAX_GENRAL]; 
      scanf("%s", command);

      // store command related info, send info to dedicated functions
      
      // login   
      if (strcmp(command, "/login") == 0 && logged_in == 0) {
         char server_IP[MAX_GENRAL];
         char server_port[MAX_GENRAL]; 
         scanf("%s %s %s %s", client_ID, password, server_IP, server_port);
         printf("login \n");
         
         login(server_IP, server_port);
      }
      else if (strcmp(command, "/login") == 0 && logged_in == 1) {
         printf("Already logged in. To login to a diffrent account you must first logout of this account.");
      }
      // logout
      else if (strcmp(command, "/logout") == 0 && logged_in == 1) {
         printf("logout \n");
         logout();
      }
      
      // join
      else if (strcmp(command, "/joinsession") == 0 && logged_in == 1) {
         char session_to_join[MAX_GENRAL]; 
         scanf("%s", session_to_join);
         printf("join \n");
         joinsession(session_to_join);
      }

      //leave
      else if (strcmp(command, "/leavesession") == 0 && logged_in == 1) {
         printf("leave \n");
         leavesession();
      }

      // create
      else if (strcmp(command, "/createsession") == 0 && logged_in == 1) {

         // check if you're in a session already
         if (strcmp(session_ID, "0") != 0) {
            printf("Unable to create session: already in a session");
            return 0;
         }

         char new_session[MAX_GENRAL]; 

         scanf("%s", new_session);
         strcpy(session_ID, new_session);
         printf("new session name: %s ", new_session);
         printf("create \n");
         createsession(new_session);
      }

      // list
      else if (strcmp(command, "/list") == 0 && logged_in == 1) {
         printf("list \n");
         list_all();
      }

      // quit
      else if (strcmp(command, "/quit") == 0) {
         printf("Quitting the program \n");
         close(current_socket);
         return 0;
      }

      // text
      else if (logged_in == 1) {
         // get rest of text
         char text[MAX_DATA];
         scanf("%[^\n]s", text);
         // add remaining text to end of first word
         strcat(command, text);
         printf("text \n");
         
         struct message packet = {0};
         packet.type = 10;
         strcpy(packet.source, client_ID);

         strcpy(packet.data, command);
         packet.size = sizeof(packet);

         printf("%d:%d:%s:%s\n", packet.type, packet.size, packet.source, packet.data);

         // send username and password
         send_data(packet);

         printf("sent success\n");
      }
      
      // not logged in but sending things
      else {
         printf("Please login first. \n"); 
         printf("/login <client ID> <password> <server-IP> <server-port> \n");
      }


      // send length of message
      // send message
      if (logged_in == 1) {
         for(;;) {
            read_fds = master; // copy it
            select(fdmax+1, &read_fds, NULL, NULL, &tv);
            // run through the existing connections looking for data to read
            if (FD_ISSET(fdmax, &read_fds)) { // we got one!!
                  
               // handle data from a client
               if ((nbytes = recv(fdmax, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client
                     if (nbytes == 0) {
                        // connection closed
                        printf("selectserver: socket %d hung up\n", i);
                     } else {
                        perror("recv");
                     }
                     close(i); // bye!
                     FD_CLR(i, &master); // remove from master set
               } 
               else {
                  printf("received %d\n", nbytes);
                  struct message received = {0};
                  received = stringToPacket(buf);
                  printf("%s\n", received.data);
                  printf("!receiver %d\n", received.type);
               }
            } // END got new incoming connection
            else {
               break;   
            }
         } // END for(;;)--and you thought it would never end!
      }

   }
}

//login
int login(char *server_ID, char *server_port) {

   // connect to server
   
   // open  socket
   // int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
   struct addrinfo hints, *p;    // server host & IP address
   int rv;
   int num_bytes;

   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_INET;      // IPv4 
   hints.ai_socktype = SOCK_STREAM;

   if ((rv = getaddrinfo(server_ID, server_port, &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return -1;
   }

   // loop through all the results and connect to the first we can
   for(p = servinfo; p != NULL; p = p->ai_next) {
      if ((current_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
         perror("client: socket");
      continue;
      }
      if (connect(current_socket, p->ai_addr, p->ai_addrlen) == -1) {
         perror("client: connect");
         close(current_socket);
      continue;
      }
      break; // if we get here, we must have connected successfully
   }

   if (p == NULL) {
      fprintf(stderr, "client: failed to connect\n");
      return -1;
   }

   printf("socket %d\n", current_socket);

   // add the listener to the master set
    FD_SET(current_socket, &master);

    // keep track of the biggest file descriptor
    fdmax = current_socket; // so far, it's this one

   // create packet
   struct message packet = {0};
   packet.type = 0;
   strcpy(packet.source, client_ID);
   strcpy(packet.data, password);
   packet.size = sizeof(password);

   printf("%d:%d:%s:%s\n", packet.type, packet.size, packet.source, packet.data);

   // send username and password
   send_data(packet);

   printf("sent success\n");

   receive(0);

  return 0;
}

// logout
int logout() {
   
   // leave session
   if (strcmp(session_ID, "0") != 0) {
      leavesession();
   }

   // exit server
   // create packet
   struct message packet = {0};
   packet.type = 3;
   strcpy(packet.source, client_ID);
   strcpy(packet.data, "0");
   packet.size = sizeof(packet);

   printf("%d:%d:%s:%s\n", packet.type, packet.size, packet.source, packet.data);

   // send username and password
   send_data(packet);

   printf("sent success\n");

   // reset username, password, loggedin, current_socket 
   strcpy(client_ID, "0");
   strcpy(password, "0");
   close(current_socket);
   current_socket = -1;
   logged_in = 0;

   return 0;
}

// join
int joinsession(char* session_to_join) {
   // check if user in session already, only 1 session allowed
   if (strcmp(session_ID, "0") != 0){
      printf("INVALID: Currently in a session");
      return 0;
   }
   strcpy(session_ID, session_to_join);
   
   // send join details to server
   struct message packet = {0};
   packet.type = 4;
   strcpy(packet.source, client_ID);
   strcpy(packet.data, session_to_join);
   packet.size = sizeof(packet);

   printf("%d:%d:%s:%s\n", packet.type, packet.size, packet.source, packet.data);

   // send username and password
   send_data(packet);

   printf("sent success\n");

   receive(1);
   
   printf("In session %s", session_ID);
   
   return 0;
}

// leave
int leavesession() {
   // check if you're in a session 
   if (strcmp(session_ID, "0") == 0){
      printf("INVALID: Not currently in a session");
      return 0;
   }

   // tell server you want to leave current
   struct message packet = {0};
   packet.type = 7;
   strcpy(packet.source, client_ID);
   strcpy(packet.data, "0");
   packet.size = sizeof(packet);

   printf("%d:%d:%s:%s\n", packet.type, packet.size, packet.source, packet.data);

   // send username and password
   send_data(packet);

   printf("sent success\n");

   strcpy(session_ID, "0");
   printf("In session %s", session_ID);

   return 0;
}

// create
int createsession(char * new_session_name) {
   
   // tell server you want to create a session
   struct message packet = {0};
   packet.type = 8;
   strcpy(packet.source, client_ID);
   strcpy(packet.data, new_session_name);
   packet.size = sizeof(packet);

   printf("%d:%d:%s:%s\n", packet.type, packet.size, packet.source, packet.data);

   // send username and password
   send_data(packet);

   printf("sent success\n");

   receive(2);

   return 0;
}

// list
int list_all() {
   // ask client for list of clients and sessions
   struct message packet = {0};
   packet.type = 11;
   strcpy(packet.source, client_ID);
   strcpy(packet.data, session_ID);
   packet.size = sizeof(packet);

   printf("%d:%d:%s:%s\n", packet.type, packet.size, packet.source, packet.data);

   // send username and password
   send_data(packet);

   printf("sent success\n");
   
   receive(3);
}

// quit
int quit() {
   logout(current_socket);
   printf("quit");
   return 0;
}

// send data
int send_data (struct message packet) {
   int num_bytes;
   char packet_buffer[sizeof (struct message)];
      
   int message = sprintf(packet_buffer, "\n%d:%d:%s:%s", packet.type, packet.size, packet.source, packet.data);
   printf("sent %d\n", message);
   printf("%s \n", packet_buffer);

   if((num_bytes = send(current_socket, packet_buffer, sizeof(packet_buffer), 0)) == -1) { 
      printf("send error");
   }
   return 0;
}

struct message stringToPacket(char * buffer){
    
    char * current_char = buffer;
    struct message recv_packet = {0};

    if(buffer == NULL){
        return recv_packet;
    }
    
    char type[2] = "";  
    while(current_char[0] != ':'){
        strncat(type,current_char,1);
        current_char += 1;
    }
    recv_packet.type = atoi(type);
    current_char += 1;

    char size[4] = "";
    while(current_char[0] != ':'){
        strncat(size,current_char,1);
        current_char += 1;
    }
    recv_packet.size = atoi(size);
    current_char += 1;

    printf("packet type %d, size %d\n", recv_packet.type, recv_packet.size);

    char source[1] = {0};
    while(current_char[0] != ':'){
        strncat(source,current_char,1);
        current_char += 1;
    }
    strcpy(recv_packet.source, source);
    printf("source: %s\n", recv_packet.source);
    current_char += 1;

    //data
    for(int i = 0; i < recv_packet.size; i++){
        recv_packet.data[i] = *current_char;
        current_char += 1;
    }

    printf("data: %s\n", recv_packet.source);

    return recv_packet;
}

void receive(int sent_type) {
   while (1) {
      struct sockaddr_storage server_addr;
      socklen_t server_addr_len = sizeof server_addr;
      int num_bytes;

      char buf[1000];

      if ((num_bytes = recv(current_socket, buf, sizeof(buf), 0)) <= 0){
         fprintf(stderr,"Recvfrom error\n");
         return;
      }

      else {
         printf("received %d\n", num_bytes);
         struct message received = {0};
         received = stringToPacket(buf);

         printf("!receiver %d, %d\n", received.type, sent_type);
         
         // broadcast from server
         if (received.type == 10) {
            printf("%s", received.data);
         }
         
         // login sent_type = 0
         else if (received.type == 1 && sent_type == 0) {
            printf("Successfully logged in\n");
            logged_in = 1;
            return;
         }
         else if (received.type == 2 && sent_type == 0) {
            printf("Unable to login: %s \n", received.data);
            return;
         }

         // join session sent_type = 1
         else if (received.type == 5 && sent_type == 1) {
            printf("Successfully joined session\n");
            return;
         }
         else if (received.type == 6 && sent_type == 1){
            printf("Unable to join %s: %s \n", session_ID, received.data);
            return;
         }

         // create session sent_type = 2
         else if (received.type == 9 && sent_type == 2) {
            printf("Successfully created session\n");
            return;
         }
          // query sent_type = 3
         else if (received.type == 12 && sent_type == 3) {
            printf("Successfully requested list of clients and available sessions\n");
            // print list
            printf("%s", received.data);
            return;
         }
         
         else if (sent_type = 2){
            printf("Unable to create session \n");
            return;
         }

        
         else if (sent_type == 3) {
            printf("Unable to receive requested list \n");
            return;
         }
      }
   }
}