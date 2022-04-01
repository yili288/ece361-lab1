#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

    FILE* accs;
    //char username[10];
    //char passw[10];

    accs = fopen("accounts.txt", "a");
/*
    while(fscanf(accs,"%s %s\n", username, passw) != EOF){
        printf("%s %s\n", username, passw);
    }
*/
    char new_user[10] = "e";
    char new_passw[10] = "55";
    fprintf(accs, "%s %s\n", new_user, new_passw);

    fclose(accs);
    return 0;
}