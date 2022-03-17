#define MAX_NAME 100
#define MAX_DATA 1000
#define MAX_GENRAL 100

struct message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};

// char commands[][] = {"/login", "/logout", "/joinsession", 
//                      "/leavesession", "createsession", "/list", "/quit"};