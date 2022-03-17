#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include "message.h"


#define NUM_ACC 20
#define MAX_SESSION_NAME 30
#define MAX_ONQUEUE 20


//index of account_db correlates with index of users_db

typedef struct account {
    char name;
    char password[2];
} Account;

Account accounts_db[NUM_ACC] = {{'a',"aa"},{'b',"bb"},{'c',"cc"},{'d',"dd"},
                            {'e',"ee"},{'f',"ff"},{'g',"gg"},{'h',"hh"},
                            {'i',"ii"},{'j',"jj"},{'k',"kk"},{'l',"ll"},
                            {'m',"mm"},{'n',"nn"},{'o',"oo"},{'p',"pp"},
                            {'q',"qq"},{'r',"rr"},{'s',"ss"},{'t',"tt"}};


typedef struct client {
    char session_id[MAX_SESSION_NAME];  
    int socket_fd;
    bool isActive;
    struct sockaddr_storage addr;   //IP and port number of client
} Client;

Client users_db[NUM_ACC];

typedef struct session {
    int session_num;
    char session_id[MAX_SESSION_NAME];
} Session;

Session sessions_db[NUM_ACC] = {0};

//construct
//lo_ack,lo_nak, jn_ack,jn_nak, ns_ack, qu_ack

void makeAckPacket(char* data){
    struct message packet = {0};
}

//deconstruct
//login, exit, join, leave_sess,new_sess, message, query

int main(int argc, char *argv[]) {


    // check number of arguments
    if(argc != 2){
        fprintf(stderr,"Make sure to use: server <TCP port number to listen on> (port number > 1024)\n");
        return 0;
    }
  
    // take in user input 
    char * ptr;
    int port_num = strtol(argv[1], &ptr , 10);
    printf("Server is receiving on TCP port %d\n", port_num);
    if(port_num < 1024 && port_num > 65535){
        printf("port number invalid\n");
        return 0;
    }

    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr= {0};   //server host & IP address
    server_addr.sin_family = AF_INET; 
    server_addr.sin_port = htons(port_num);
    server_addr.sin_addr = (struct in_addr) { htonl(INADDR_ANY) };

    int bind_result = bind(tcp_socket, (struct sockaddr *) &server_addr, sizeof(server_addr) );
    if(bind_result < 0){
        return -1;
    }

    //remove 'address already in use' message
    int yes=1;
    if (setsockopt(tcp_socket,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) {
        perror("setsockopt");
        exit(1);
    }


    //listen: establish queue
    int listen_result = listen(tcp_socket, MAX_ONQUEUE);
    if(listen_result < 0){
        return -1;
    }

    //accept:
    struct sockaddr_storage client_addr= {0};     //client host & IP address
    socklen_t clientAddrLen = sizeof(client_addr);
    int client_fd = accept(tcp_socket, (struct sockaddr*) &client_addr, &clientAddrLen);

    char host[1024];
    char service[20];

    //change 0 to NI_NOFQDN
    getnameinfo((struct sockaddr*) &client_addr, clientAddrLen, host, sizeof(host), service, sizeof(service), 0);  
    printf(" host: %s\n", host); // e.g. "www.example.com"


    //Extract info from the accepted packet
    void * buff;
    int num_chars = recv(client_fd, buff, 200, 0);
    if(num_chars == 0){
        printf("client closed connection on you\n");
    }

    if(num_chars < 0) {
        printf("problem with recv()\n");
        return -1;
    }

    //might have to do polling to accept next packet in queue

    void* ack; //acknowledgement packet
    int len = strlen(ack);
    //send: to client
    int send_res = send(client_fd, ack, len, 0);
    if(send_res < 0){
        return -1;
    }

    
}