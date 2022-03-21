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

struct message stringToPacket(char * buffer);
int login(struct message packet, int receiver_fd);

int exit_conf(struct message packet, int receiver_fd);
int join(struct message packet, int receiver_fd);
int leave_sess(struct message packet, int receiver_fd);
int new_sess(struct message packet, int receiver_fd);
int broadcast(struct message packet, int receiver_fd);
int getActiveUserSessions(struct message packet, int receiver_fd);

int sendPacket(struct message packet, int receiver_fd);

//index of account_db correlates with index of users_db

typedef struct account {
    char name[1];
    char password[2];
} Account;

Account accounts_db[NUM_ACC] = {{"a","aa"},{"b","bb"},{"c","cc"},{"d","dd"},
                            {"e","ee"},{"f","ff"},{"g","gg"},{"h","hh"},
                            {"i","ii"},{"j","jj"},{"k","kk"},{"l","ll"},
                            {"m","mm"},{"n","nn"},{"o","oo"},{"p","pp"},
                            {"q","qq"},{"r","rr"},{"s","ss"},{"t","tt"}};


typedef struct client {
    char* name;
    char* session_id;  
    int socket_fd;
    bool isActive;
} Client;

Client users_db[NUM_ACC];

typedef struct session {
    char* session_id;
    int num_ppl;
} Session;

Session sessions_db[NUM_ACC];

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

    //make user database
    
    for(int i=0; i < NUM_ACC; i++){
        users_db[i].name = NULL;
        users_db[i].session_id = NULL;
        users_db[i].socket_fd = -1;
        users_db[i].isActive = false;
    }

    for(int i=0; i < NUM_ACC; i++){
        sessions_db[i].session_id = NULL;
        sessions_db[i].num_ppl = 0;
        
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

    while(1){
        int client_fd = accept(tcp_socket, (struct sockaddr*) &client_addr, &clientAddrLen);
        printf("client fd is %d \n", client_fd); // -----------CHECK fd

        //might have to do polling to accept next packet in queue

        char host[1024];
        char service[20];
        //change 0 to NI_NOFQDN
        getnameinfo((struct sockaddr*) &client_addr, clientAddrLen, host, sizeof(host), service, sizeof(service), 0);  
        printf(" client %s connected\n", host); // e.g. "www.example.com"


        //Extract info from the accepted packet
        //void * recv_buff;
        char recv_buff[1000]; //------------------------------- MODIFIED

        int num_chars = recv(client_fd, recv_buff, 1000, 0);
        if(num_chars == 0){
            printf("client closed connection on you\n");
        }
        if(num_chars < 0) {
            printf("problem with recv()\n");
            return -1;
        }else{
            printf("recevied amount %d", num_chars);
        }

        //extract info (type,data) out from recv_buff
        struct message recv_packet = stringToPacket(recv_buff);
        
        switch (recv_packet.type)
        {   
            case 0: //login
                login(recv_packet, client_fd);
                break;

            case 3: //exit
                exit_conf(recv_packet, client_fd);
                break;
            case 4: //join
                join(recv_packet, client_fd);
                break;
            case 7: //leave
                leave_sess(recv_packet, client_fd);
                break;
            case 8: //new
                new_sess(recv_packet, client_fd);
                break;
            case 10: //message
                broadcast(recv_packet, client_fd);
                break;
            case 11: //query
                getActiveUserSessions(recv_packet, client_fd);
                break;
        }

        void* ack = "1:"; //acknowledgement packet
        int len = strlen(ack);
        //send: to client
        int send_res = send(client_fd, ack, len, 0);
        if(send_res < 0){
            return -1;
        }
    }

    
}


//handle login packet:
//there are 4 fields and 3 semicolons
//check that for 3rd field, the 4th field is the password (accounts_db)
//if already active, send LO_NACK
//if yes, set to active (users_db) & send LO_ACK with no data
//if no, send LO_NACK with data

int login(struct message packet, int receiver_fd){
    struct message ack_pack = {0};
    strcpy(ack_pack.source, packet.source);
    //printf("!! %s == %s", accounts_db[0].name, packet.source);

    FILE* accs;
    char line[10];

    accs = fopen("accounts.txt", "r");
    int res;

    while(res != EOF || res == 0){
        
        res = fscanf(accs,"%s", line);
        printf("%s\n", line); 
        if(strstr(line, packet.source) != NULL ){ //user exists
            int i = (int)line[0] - (int)'a'; 
            printf("index %d", index);

            //check password
            if(strstr(line, packet.data) != NULL ){
                if(users_db[i].isActive == false){
                    users_db[i].isActive = true;

                    //send LO_ACK
                    ack_pack.type = 1;
                    strcpy(ack_pack.data, "0");
                    ack_pack.size = sizeof(packet);
                    sendPacket(ack_pack, receiver_fd);
                    return 1;
                }else{
                    //send LO_NACK
                    ack_pack.type = 2;
                    strcpy(ack_pack.data, "user already logged in");
                    ack_pack.size = sizeof(packet);
                    sendPacket(ack_pack, receiver_fd);
                    return -1;
                }
            }else{
                //password is wrong
                //send LO_NACK
                ack_pack.type = 2;
                strcpy(ack_pack.data, "wrong password");
                ack_pack.size = sizeof(packet);
                sendPacket(ack_pack, receiver_fd);
                return -1;
            }
            
        }  
    }
    
    //send LO_NACK
    ack_pack.type = 2;
    strcpy(ack_pack.data, "user account not found");
    ack_pack.size = sizeof(packet);
    sendPacket(ack_pack, receiver_fd);
    return -1;

}



//handle exit packet:
//for 3rd field, set active to false (users_db)
int exit_conf(struct message packet, int receiver_fd){

    /*for(int i=0; i < NUM_ACC; i++){
        if(strcmp(accounts_db[i].name, packet.source) == 0){
            users_db[i].isActive = false;
        }
    }*/

    
    int i = (int)*packet.source - (int)'a'; 
    users_db[i].isActive = false;

    for(int i=0; i < NUM_ACC; i++){
        if(strcmp(sessions_db[i].session_id, users_db[i].session_id) == 0){
            sessions_db[i].num_ppl -= 1;
        }

        if(sessions_db[i].num_ppl == 0){
            sessions_db[i].session_id = NULL;
        }

        users_db[i].session_id = NULL;
    }
        
}



//join pack:

//find is session id exists
//yes: find user and set session id, num of ppl += 1, send JN_ACK with session ID
//no: session not found, send JN_NACK with data which says "session not found"

int join(struct message packet, int receiver_fd){

    bool sessionExists = false;

    for(int i=0; i < NUM_ACC; i++){
        if(strcmp(users_db[i].session_id, packet.data) == 0){
            sessionExists = true;
        }
    }

    if(sessionExists){
        int i = (int)*packet.source - (int)'a'; 
                if(users_db[i].isActive){
                    strcpy(users_db[i].session_id, packet.data);
                    
                    //find session and add num of ppl
                    for(int i=0; i < NUM_ACC; i++){
                        if(strcmp(sessions_db[i].session_id, packet.data) == 0){
                            sessions_db[i].num_ppl += 1;
                            //JN_ACK
                            packet.type = 5;
                            sendPacket(packet, receiver_fd);
                            return 1;
                        }
                    }      

                }else{
                    //JN_NAK
                    //user not active
                    packet.type = 6;
                    strcpy(packet.data, "account not active, please login");
                    sendPacket(packet, receiver_fd);
                }
                
            
        
    }else{
        //JN_NAK
        //user not exists
        packet.type = 6;
        strcpy(packet.data, "session does not exists");
        sendPacket(packet, receiver_fd);
    }
    return -1;

}

//leave session:
//change user_db (active set false) and sessions_db (num of ppl -= 1)
//if num of ppl after this is 0, then delete session
int leave_sess(struct message packet, int receiver_fd){

    
    int i = (int)*packet.source - (int)'a'; 
    users_db[i].isActive = false;

    for(int i=0; i < NUM_ACC; i++){
        if(strcmp(sessions_db[i].session_id, packet.data) == 0){
            sessions_db[i].num_ppl -= 1;
        }

        if(sessions_db[i].num_ppl == 0){
            sessions_db[i].session_id = NULL;
        }
    }

    users_db[i].session_id = NULL;
        
    
            
}

//new session:
//find first empty element in session_db
//fill it with information (session id, num ppl = 1)
//success: NS_ACK
int new_sess(struct message packet, int receiver_fd){
    int index = (int)*packet.source - (int)'a'; 
    printf("new sess %s\n", packet.data);
    for(int i=0; i < NUM_ACC; i++){
        if(sessions_db[i].session_id == NULL){
            strcpy(sessions_db[i].session_id, packet.data);
            sessions_db[i].num_ppl = 1;
            printf("%s", packet.data);

            //
            strcpy(users_db[index].session_id,packet.data);

            //send NS_ACK
            packet.type = 9;
            strcpy(packet.data, "0");
            packet.size = sizeof(packet);
            sendPacket(packet, receiver_fd);
            return 1;
        }
    }

}



//message:
//sent to every in that session except the person that sent it
//get session id for this user
//get all users with this session id
int broadcast(struct message packet, int receiver_fd){

    char* session;

    int index = (int)*packet.source - (int)'a';
            session = users_db[index].session_id;

            for(int i=0; i < NUM_ACC; i++){
                if(strcmp(users_db[i].session_id, session) == 0){

                    packet.type = 10;
                    packet.size = sizeof(packet);
                    strcpy(packet.source, accounts_db[i].name);
                    //packet data kept the same

                    sendPacket(packet, users_db[i].socket_fd);

                }
            }
       

}

//query:
//go thru user_db, if active is true, add user ID and session num to list
//if not active users send 'none' in message
//QU_ACK: create message to send
//User1: 1,2,3,4
//User2: 5,6,7

int getActiveUserSessions(struct message packet, int receiver_fd){
    char all_info[1000];

    for(int i=0; i < NUM_ACC; i++){
        if(users_db[i].isActive){
            strcat(all_info, accounts_db[i].name);
            strcat(all_info, ": ");
            strcat(all_info, users_db[i].session_id);
            strcat(all_info, ", ");
        }
    }

    packet.type = 12;
    strcpy(packet.data, all_info);
    packet.size = sizeof(packet);
    
    sendPacket(packet, receiver_fd);

}


int sendPacket(struct message packet, int receiver_fd){
    char packet_buff[1000];

    int message = sprintf(packet_buff, "\n%d:%d:%s:%s:", packet.type, packet.size, packet.source, packet.data);
    printf("%d", message);
    printf("%s \n", packet_buff);
    int len = strlen(packet_buff);

    //send: to client
    int send_res = send(receiver_fd, packet_buff, len, 0);
    if(send_res < 0){
        printf("send error\n");
        return -1;
    }
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

    printf("\npacket type %d, size %d\n", recv_packet.type, recv_packet.size);

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

    printf("data: %s\n", recv_packet.data);

    return recv_packet;
}
