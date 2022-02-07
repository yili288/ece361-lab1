#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "packet.h"

#define BUFSIZE 1100

struct packet stringToPacket(char * buffer){
    char * current_char = buffer;
    struct packet recv_packet = {0};
    
    char total_frag[4] = "";  
    while(current_char[0] != ':'){
        strncat(total_frag,current_char,1);
        current_char += 1;
    }
    recv_packet.total_frag = atoi(total_frag);
    current_char += 1;

    char frag_no[4] = "";
    while(current_char[0] != ':'){
        strncat(frag_no,current_char,1);
        current_char += 1;
    }
    recv_packet.frag_no = atoi(frag_no);
    current_char += 1;

    char size[4] = "";
    while(current_char[0] != ':'){
        strncat(size,current_char,1);
        current_char += 1;
    }
    recv_packet.size = atoi(size);
    current_char += 1;

    char filename[100] = "";
    while(current_char[0] != ':'){
        strncat(filename,current_char,1);
        current_char += 1;
    }
    recv_packet.filename = filename;
    current_char += 1;

    for(int i = 0; i < recv_packet.size; i++){
        recv_packet.filedata[i] = *current_char;
        current_char += 1;
    }

    return recv_packet;
}

int main(int argc, char *argv[]) {

    // check number of arguments
    if(argc != 2){
        fprintf(stderr,"Make sure to use: server <UDP listen port> (port number > 1024)\n");
        return 0;
    }
  
    // take in user input 
    char * ptr;
    int port_num = strtol(argv[1], &ptr , 10);
    printf("Server is receiving on port %d\n", port_num);
    if(port_num < 1024 && port_num > 65535){
        printf("port number invalid\n");
        return 0;
    }

    // open UDP socket
    int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    //printf("fd %d %d\n", udp_fd, port_num);

    struct sockaddr_in server_addr= {0};   //server host & IP address
    server_addr.sin_family = AF_INET;  //problem here!
    server_addr.sin_port = htons(port_num);
    server_addr.sin_addr = (struct in_addr) { htonl(INADDR_ANY) };

    int bind_result = bind(udp_fd, (struct sockaddr *) &server_addr, sizeof(server_addr) );

    if(bind_result < 0){
        return -1;
    }

    //liten to port
    char receive_buf[BUFSIZE];
    struct sockaddr_storage client_addr= {0};     //client host & IP address
    socklen_t clientAddrLen = sizeof(client_addr);

    ssize_t bytes_received = 0;

    while(1){       //waits until a message is received 
        bytes_received = recvfrom(udp_fd, receive_buf, sizeof(receive_buf), 0, (struct sockaddr*) &client_addr, &clientAddrLen);

        if(bytes_received > 0){
            //printf("buff %s\n", receive_buf);
            struct packet receive_pack = stringToPacket(receive_buf);

            //Reply client
            char* message = "";
            if (receive_pack.total_frag > 0){
                message = "yes\n";       //success
            }else{
                message = "no\n";
            }
            
            int size = strlen(message) + 1;
            ssize_t bytes_sent = 0;
            bytes_sent = sendto(udp_fd, message, size, 0, (struct sockaddr*) &client_addr, clientAddrLen);
        
            if(bytes_sent < 0){
                printf("File transfer failed\n");
            }
                            
            //Copy data into new file
            FILE* file_ptr = fopen(receive_pack.filename, "a");

            if(file_ptr != NULL){
                fprintf(file_ptr, receive_pack.filedata);
            }
            
            if(receive_pack.frag_no = receive_pack.total_frag){
                fclose(file_ptr);
                printf("Client file transferred successfully\n");
            }
        }
    }
   return 0;
}