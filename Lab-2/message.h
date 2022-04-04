#define MAX_NAME 10
#define MAX_DATA 600
#define MAX_GENRAL 200

struct message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];   //client ID (eg 'a')
    unsigned char data[MAX_DATA];
};
