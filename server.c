#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>


typedef struct client {
    char name;
    char password[2];
} Client;

Client user_database[20] = {{'a',"aa"},{'b',"bb"},{'c',"cc"},{'d',"dd"},
                            {'e',"ee"},{'f',"ff"},{'g',"gg"},{'h',"hh"},
                            {'i',"ii"},{'j',"jj"},{'k',"kk"},{'l',"ll"},
                            {'m',"mm"},{'n',"nn"},{'o',"oo"},{'p',"pp"},
                            {'q',"qq"},{'r',"rr"},{'s',"ss"},{'t',"tt"}};


typedef struct connected {
    char session_id[20]     //session name

} Connected;

struct wc {
	int size;
	struct countNode** hashtable;
};

struct countNode {
	char* key;
	int value;
};

unsigned hash(char *s, int HASHSIZE)
{
    unsigned hashval;

    for (hashval = 0; *s != '\0'; s++)
        hashval = *s + 31*hashval;
    return hashval % HASHSIZE;
}

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

    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr= {0};   //server host & IP address
    server_addr.sin_family = AF_INET; 
    server_addr.sin_port = htons(port_num);
    server_addr.sin_addr = (struct in_addr) { htonl(INADDR_ANY) };

    int bind_result = bind(tcp_socket, (struct sockaddr *) &server_addr, sizeof(server_addr) );
    if(bind_result < 0){
        return -1;
    }
/* prevent 
    int yes=1;
    if (setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) {
        perror("setsockopt");
        exit(1);
    }
*/

    //establish queue
    int listen_result = listen(tcp_socket, 20);
    if(listen_result < 0){
        return -1;
    }

    struct sockaddr_storage client_addr= {0};     //client host & IP address
    socklen_t clientAddrLen = sizeof(client_addr);
    int recv_fd = accept(tcp_socket, (struct sockaddr*) &client_addr, &clientAddrLen);

    //how to extract info from the accepted packet?
    void * buff;
    int num_chars = read(recv_fd, buff, 100);
    if(num_chars <= 0) {
        printf("problem with read() ");
        return -1;
    }

    //might have to do polling to accept next packet in queue


    
}