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


#define NUM_ACC 30
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
int newAccount(struct message packet, int receiver_fd);
int privateMsg(struct message packet, int source_fd);


int sendPacket(struct message packet, int receiver_fd);


typedef struct client {
    char* name;
    char* session_id;  
    int socket_fd;
    bool isActive;
} Client;

Client users_db[NUM_ACC];
int lastUserIndex = 0;

typedef struct session {
    char* session_id;
    int num_ppl;
} Session;

Session sessions_db[NUM_ACC];

//construct
//lo_ack,lo_nak, jn_ack,jn_nak, ns_ack, qu_ack

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

    FILE* accs;
    char username[10];
    char passw[10];

    accs = fopen("accounts.txt", "r");

    int j = 0;
    while(fscanf(accs,"%s %s\n", username, passw) != EOF){
        char * ptr = malloc(sizeof(username));
        strcpy(ptr, username);
        users_db[j].name = ptr;
        j++;
        lastUserIndex++;
    }

    for(int i=0; i < NUM_ACC; i++){
        sessions_db[i].session_id = NULL;
        sessions_db[i].num_ppl = -1;
        
    }

    // variables to handle multiple clients
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int newfd;        // newly accept()ed socket descriptor
    char remoteIP[AF_INET];

    char buf[1100];    // buffer for client data
    int nbytes;

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);


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
//

    //listen: establish queue
    int listen_result = listen(tcp_socket, MAX_ONQUEUE);
    if(listen_result < 0){
        perror("listen");
        exit(3);
    }

    // add the listener to the master set
    FD_SET(tcp_socket, &master);

    // keep track of the biggest file descriptor
    fdmax = tcp_socket; // so far, it's this one

    //accept:
    struct sockaddr_storage client_addr= {0};     //client host & IP address
    socklen_t clientAddrLen = sizeof(client_addr);
    
    int i = 0;
    // main loop
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {   
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            
            if (FD_ISSET(i, &read_fds)) { // we got one!! this is an fa flag
                if (i == tcp_socket) {
                    // handle new connections
                    clientAddrLen = sizeof client_addr;
					newfd = accept(tcp_socket, (struct sockaddr *)&client_addr, &clientAddrLen);

					if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }

                        char host[1024];
                        char server[20];
                        getnameinfo((struct sockaddr *)&client_addr, clientAddrLen, 
                                        host, sizeof(host),server, sizeof(server), 0);
                        printf("selectserver: new connection from %s on "
                                "socket %d\n", host, newfd);
                    }
                } else {
                    // handle data from a existing connected client
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);

                            for(int j=0; j < NUM_ACC ; j++){
                                if(users_db[j].socket_fd == i){   //reset user info
                                    users_db[j].name = NULL;
                                    users_db[j].session_id = NULL;
                                    users_db[j].socket_fd = -1;
                                    users_db[j].isActive = false;
                                }
                            }
                        } else {
                            perror("recv");
                        }

                        
                        
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } else {
                        //extract info (type,data) out from recv_buff
                        //printf("start convert");
                        struct message recv_packet = {0};
                        recv_packet = stringToPacket(buf);
                        //printf("end converting %d \n", recv_packet.type);
                        
                        if (recv_packet.type == 0){ 
                            //login
                            login(recv_packet, i);
                        }else if(recv_packet.type == 3){
                            //exit
                            exit_conf(recv_packet, i);
                        }else if(recv_packet.type == 4){
                            //join
                            join(recv_packet, i);
                        }else if(recv_packet.type == 7){
                            //leave
                            leave_sess(recv_packet, i);
                        }else if(recv_packet.type == 8){
                            //new
                            new_sess(recv_packet, i);
                        }else if(recv_packet.type == 10){
                            //message
                            broadcast(recv_packet, i);
                        }else if(recv_packet.type == 11){
                            //query
                            getActiveUserSessions(recv_packet, i);
                        }else if(recv_packet.type == 13){
                            //printf("correct\n");
                            newAccount(recv_packet, i);
                        }else if(recv_packet.type == 16){
                            privateMsg(recv_packet, i);
                        }
                    } //end extracting data
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
    
    return 0;
    
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

    FILE* accs;
    char username[10];
    char passw[10];

    accs = fopen("accounts.txt", "r");
    

    while(fscanf(accs,"%s %s\n", username, passw) != EOF){
        if(strcmp(username, packet.source) == 0){

         for(int idx=0; idx< NUM_ACC; idx++){  //account exists in file
            if(strcmp(users_db[idx].name, packet.source) == 0){  //get index
                //if(strstr(username, packet.source) != NULL ){ //user exists
                //int idx = username[0] - 'a'; 

                //check password
                printf("pass %s %s", passw, packet.data);
                if(strstr(passw, packet.data) != NULL ){
                    if(users_db[idx].isActive == false){
                        users_db[idx].isActive = true;
                        users_db[idx].socket_fd = receiver_fd;
                        //printf("saved value %d\n", users_db[idx].socket_fd);
                        
                        //send LO_ACK
                        ack_pack.type = 1;
                        strcpy(ack_pack.data, "0");
                        ack_pack.size = 1;
                        sendPacket(ack_pack, receiver_fd);
                        fclose(accs);
                        return 1;
                    }else{
                        //send LO_NACK
                        ack_pack.type = 2;
                        strcpy(ack_pack.data, "user already logged in");
                        ack_pack.size = sizeof(ack_pack.data);
                        sendPacket(ack_pack, receiver_fd);
                        fclose(accs);
                        return -1;
                    }
                }else{
                    //password is wrong
                    //send LO_NACK
                    ack_pack.type = 2;
                    strcpy(ack_pack.data, "wrong password");
                    ack_pack.size = sizeof(ack_pack.data);
                    sendPacket(ack_pack, receiver_fd);
                    fclose(accs);
                    return -1;
                }
                
            }  
         }
        }
    }

  
            //
            //send LO_NACK
            ack_pack.type = 2;
            strcpy(ack_pack.data, "user account not found");
            ack_pack.size = sizeof(ack_pack.data);
            sendPacket(ack_pack, receiver_fd);
            fclose(accs);
            return -1;
        
    
    

}

int newAccount(struct message packet, int receiver_fd){  //registration does not login user
    
    FILE* accs;
    char username[10];
    char passw[10];

    accs = fopen("accounts.txt", "a+");
    while(fscanf(accs,"%s %s\n", username, passw) != EOF){
        
        if(strstr(username, packet.source) != NULL ){ //user exists
            packet.type = 15;
            strcpy(packet.data, "username already exists");
            packet.size = strlen(packet.data);
            sendPacket(packet, receiver_fd);
            fclose(accs);
            return -1;
        }
    }

    if(accs!= 0){ //user doesnt exist
        fprintf(accs, "%s %s\n", packet.source, packet.data);

        char * ptr = malloc(sizeof(packet.source));
        //printf("source %s\n", packet.source);
        strcpy(ptr, packet.source);

        users_db[lastUserIndex].name = ptr;
        users_db[lastUserIndex].session_id = NULL;
        users_db[lastUserIndex].socket_fd = receiver_fd;
        users_db[lastUserIndex].isActive = false;
        lastUserIndex++;
        
        packet.type = 14;
        strcpy(packet.data, "0");
        packet.size = strlen(packet.data);
        //printf("sending to %d", receiver_fd);
        sendPacket(packet, receiver_fd);
        fclose(accs);
        return 1;
    }
}



//handle exit packet:
//for 3rd field, set active to false (users_db)
int exit_conf(struct message packet, int receiver_fd){

    //printf("exit func: %s %d\n", packet.source, (int)'a');
    //int i = (int)packet.source[0] - (int)'a';
    //printf("user location %d\n", i); 

    for(int idx=0; idx< NUM_ACC; idx++){
        if(users_db[idx].name != NULL && strcmp(users_db[idx].name, packet.source) == 0){
            users_db[idx].isActive = false;
            users_db[idx].socket_fd = -1;
        }
    }
    

    for(int i=0; i < NUM_ACC; i++){
        if(users_db[i].session_id != NULL && sessions_db[i].session_id != NULL
            && strcmp(sessions_db[i].session_id, users_db[i].session_id) == 0){
            sessions_db[i].num_ppl -= 1;
            free(users_db[i].session_id);
            users_db[i].session_id = NULL;
        }

        if(sessions_db[i].num_ppl == 0){
            sessions_db[i].session_id = NULL;
        }
        
    }
        
}



//join pack:

//find is session id exists
//yes: find user and set session id, num of ppl += 1, send JN_ACK with session ID
//no: session not found, send JN_NACK with data which says "session not found"

int join(struct message packet, int receiver_fd){

    bool sessionExists = false;

    for(int i=0; i < NUM_ACC; i++){
        if(users_db[i].session_id != NULL && strcmp(users_db[i].session_id, packet.data) == 0){
            sessionExists = true;
        }
    }

    
    //int i = (int)*packet.source - (int)'a'; 
    int i = 0;
    for(int idx=0; idx< NUM_ACC; idx++){
        if(strcmp(users_db[idx].name, packet.source) == 0){
            i = idx;
            break;
        }
    }

    if(users_db[i].isActive){
        if(users_db[i].session_id == NULL){
            char * ptr = malloc(sizeof(packet.data));
            strcpy(ptr, packet.data);
            users_db[i].session_id = ptr;
        }else{
            //JN_NAK
            //already in a session
            packet.type = 6;
            strcpy(packet.data, "user already in a session");
            packet.size = strlen(packet.data);
            sendPacket(packet, receiver_fd);
            return -1;
        }

        if(sessionExists){
            //find session and add num of ppl
            for(int i=0; i < NUM_ACC; i++){
                if(sessions_db[i].session_id != NULL && strcmp(sessions_db[i].session_id, packet.data) == 0){
                    sessions_db[i].num_ppl += 1;
                    //JN_ACK
                    packet.type = 5;
                    packet.size = strlen(packet.data);
                    sendPacket(packet, receiver_fd);
                    return 1;
                }
            }    
        }else{
            //JN_NAK
            //session does not exists
            packet.type = 6;
            strcpy(packet.data, "session does not exists");
            packet.size = strlen(packet.data);
            sendPacket(packet, receiver_fd);
        }
            

    }else{
        //JN_NAK
        //user not active
        packet.type = 6;
        strcpy(packet.data, "account not active, please login");
        packet.size = strlen(packet.data);
        sendPacket(packet, receiver_fd);
    }
    
            
        
    
    return -1;

}

//leave session:
//change user_db (active set false) and sessions_db (num of ppl -= 1)
//if num of ppl after this is 0, then delete session
int leave_sess(struct message packet, int receiver_fd){

    
    //int i = (int)*packet.source - (int)'a'; 

    for(int i=0; i < NUM_ACC; i++){
        if(sessions_db[i].session_id != NULL && strcmp(sessions_db[i].session_id, packet.data) == 0){
            sessions_db[i].num_ppl -= 1;
        }

        if(sessions_db[i].num_ppl == 0){
            sessions_db[i].session_id = NULL;
        }
    }

    for(int idx=0; idx< NUM_ACC; idx++){
        if(strcmp(users_db[idx].name, packet.source) == 0){
            free(users_db[idx].session_id);
            users_db[idx].session_id = NULL;
            break;
        }
    }
    
        
    
            
}

//new session:
//find first empty element in session_db
//fill it with information (session id, num ppl = 1)
//success: NS_ACK
int new_sess(struct message packet, int receiver_fd){
    //int index = (int)*packet.source - (int)'a'; 

    int index = 0;
    for(int idx=0; idx< NUM_ACC; idx++){
        if(strcmp(users_db[idx].name, packet.source) == 0){
            index = idx;
            break;
        }
    }

    for(int i=0; i < NUM_ACC; i++){
        if(users_db[i].isActive == true && sessions_db[i].session_id == NULL){
            sessions_db[i].session_id = packet.data;
            sessions_db[i].num_ppl = 1;

            if(users_db[index].session_id == NULL){
                char * ptr = malloc(sizeof(packet.data));
                strcpy(ptr, packet.data);
                users_db[index].session_id = ptr;
                
                printf("new sess name: %s\n", users_db[index].session_id);
            }else{
                strcpy(users_db[index].session_id,packet.data);
            }

            //send NS_ACK
            packet.type = 9;
            strcpy(packet.data, "0");
            packet.size = strlen(packet.data);
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

    //int index = (int)*packet.source - (int)'a';
    

    int index = 0;
    for(int idx=0; idx< NUM_ACC; idx++){
        if(strcmp(users_db[idx].name, packet.source) == 0){
            index = idx;
            break;
        }
    }

    char* session = users_db[index].session_id;
    //packet.source stays the same because the receiver needs to know who the sender is

    for(int i=0; i < NUM_ACC; i++){
        if(users_db[i].session_id != NULL && strcmp(users_db[i].session_id, session) == 0){
            
            if(strcmp(users_db[i].name,packet.source) != 0){
                packet.type = 10;
                //packet data kept the same
                printf("message reply data: %s\n", packet.data);
                packet.size = strlen(packet.data);
                sendPacket(packet, users_db[i].socket_fd);
            }else{
                //don't send to the initial sender
            }
        }
    }

    return 1;

}

int privateMsg(struct message packet, int source_fd){
    const char s[2] = ":";
    char *token;
    char receiver[10];
    char msg[100];
    int receiver_fd;
   
    /* get the first token */
    token = strtok(packet.data, s);
    
    strcpy(receiver,token);

    /* walk through other tokens */
    token = strtok(NULL, s);
    strcpy(msg, token);

    //receiver_fd
    for(int i=0; i < NUM_ACC; i++){
        if(users_db[i].name != NULL && strcmp(users_db[i].name, receiver) == 0){
            //found!
            receiver_fd = users_db[i].socket_fd;
        }
    }

    packet.type = 16;
    //dont change source
    strcpy(packet.data, receiver);
    strcat(packet.data, ":");
    strcat(packet.data, msg);
    packet.size = strlen(packet.data);
    sendPacket(packet, receiver_fd);

}

//query:
//go thru user_db, if active is true, add user ID and session num to list
//if not active users send 'none' in message
//QU_ACK: create message to send
//User1: 1,2,3,4
//User2: 5,6,7

int getActiveUserSessions(struct message packet, int receiver_fd){
    char all_info[1000] = "";

    for(int i=0; i < NUM_ACC; i++){
        //printf("user %c is %d", users_db[i].name, users_db[i].isActive);
        if(users_db[i].isActive){
            strcat(all_info, users_db[i].name);
            strcat(all_info, ":");
            if(users_db[i].session_id == NULL){
                strcat(all_info, "no session");
            }else{
                strcat(all_info, users_db[i].session_id);
            }
            strcat(all_info, " ");
        }
    }

    packet.type = 12;
    strcpy(packet.data, all_info);
    packet.size = strlen(packet.data);
    sendPacket(packet, receiver_fd);
    return 1;

}


int sendPacket(struct message packet, int receiver_fd){
    // char packet_buff[1000];
    char packet_buff[sizeof (struct message)];

    int message = sprintf(packet_buff, "\n%d:%d:%s:%s", packet.type, packet.size, packet.source, packet.data);
    printf("sent: %s \n", packet_buff);
    int len = sizeof(packet_buff);

    //send: to client
    int send_res = send(receiver_fd, packet_buff, len, 0);
    //printf("recv %d", receiver_fd);
    if(send_res < 0){
        printf("send error\n");
        return -1;
    }
}

struct message stringToPacket(char * buffer){
    
    char * current_char = buffer;
    struct message pack = {0};

    if(buffer == NULL){
        return pack;
    }
    
    char type[2] = "";  
    while(current_char[0] != ':'){
        strncat(type,current_char,1);
        current_char += 1;
    }
    int number = atoi(type);
    if(number >= 50){
        return pack;
    }
    pack.type = number;

    
    current_char += 1;

    char size[4] = "";
    while(current_char[0] != ':'){
        strncat(size,current_char,1);
        current_char += 1;
    }
    pack.size = atoi(size);
    current_char += 1;

    //printf("\nReceived packet type: %d, size: %d\n", pack.type, pack.size);

    char source[10] = "";
    while(current_char[0] != ':'){
        strncat(source,current_char,1);
        current_char += 1;
    }

    strcpy(pack.source, source);
    //printf("source: %s\n", pack.source);
    current_char += 1;

    //data
    for(int i = 0; i < pack.size; i++){
        pack.data[i] = *current_char;
        current_char += 1;
    }

    //printf("data: %s\n", pack.data);

    return pack;
}