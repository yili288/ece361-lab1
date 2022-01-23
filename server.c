#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main() {
    printf("Hello from server!\n");
    printf("Put in command of this form (port number > 1024)\n");
    printf("     ftp <UDP listen port>\n");

    // take in user input - port number
    int user_input;
    scanf("%d", &user_input);
    if(user_input < 1024 && user_input > 65535){
        printf("port number invalid\n");
        return 0;
    }
    //open UDP socket
    int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    printf("fd %d\n", udp_fd);

    struct sockaddr_in server_addr= {0};   //server host & IP address
    server_addr.sin_family = AF_INET;  //problem here!
    server_addr.sin_port = htons(user_input);
    server_addr.sin_addr = (struct in_addr) { htonl(INADDR_ANY) };

    int bind_result = bind(udp_fd, (struct sockaddr *) &server_addr, sizeof(server_addr) );

    if(bind_result < 0){
        return -1;
    }

    //liten to port
    char receive_buf[10];
    struct sockaddr_storage client_addr= {0};     //client host & IP address
    socklen_t clientAddrLen = sizeof(client_addr);

    ssize_t bytes_received = 0;

    while(1){       //waits until a message is received 
        bytes_received = recvfrom(udp_fd, receive_buf, sizeof(receive_buf), 0, (struct sockaddr*) &client_addr, &clientAddrLen);

        if(bytes_received > 0){
            printf("buff %s\n", receive_buf);

            //Reply client
            char* message = "";
            if (strncmp(receive_buf, "ftp", 3) == 0){   //not read as true
                message = "yes\n";
            }else{
                message = "no\n";
            }

            int size = strlen(message) + 1;
            ssize_t bytes_sent = 0;
            bytes_sent = sendto(udp_fd, message, size, 0, (struct sockaddr*) &client_addr, clientAddrLen);
        
            if(bytes_sent < 0){
                printf("fail to send yes/no reply to client\n");
            }else{
                printf("client successfully replied\n");
            }
        }
    }
   return 0;
}