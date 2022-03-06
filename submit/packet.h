
#define MAX_DATA_SIZE 1000

typedef struct packet {
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[MAX_DATA_SIZE];
    struct packet *next;
} Packets;
