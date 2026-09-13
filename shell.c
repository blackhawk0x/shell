#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include "debug.h"
#include <fcntl.h>


int childId;
int childReturnCode = 0;
const int P_READ = 0;
const int P_WRITE = 1;
int  infd[2], outfd[2];

void read_en(int * infd)
{
    dup2(infd[P_READ], STDIN_FILENO);
    close(infd[P_READ]);
    close(infd[P_WRITE]);
}

//Pipe write
void write_en(int * outfd)
{
    dup2(outfd[P_WRITE], STDOUT_FILENO);
    close(outfd[P_READ]);
    close(outfd[P_WRITE]);
}


void sigHandler(int signo) {
    kill(signo,childId);
    printf("\n");
}

void sigHandler2(int signo){
    printf("signal : %d\n", signo);
}


void removeLeading(char *str){
    int idx = 0, j, k = 0;
    while (str[idx] == ' ' || str[idx] == '\t' || str[idx] == '\n')
    {
        idx++;
    }
    char str1[10];
    for (j = idx; str[j] != '\0'; j++)
    {
        str1[k] = str[j];
        k++;
    }
 
    str1[k] = '\0';
    strcpy(str,str1);
}

void split(char* string, char* del, char*arr[]){
    char *token;
    token = strtok(string,del);
    int i = 0;
    while(token != NULL){
        arr[i] = token;
        token = strtok(NULL, del);
    i++;
    }
}


int pathFinder(char * stringArr[15],char string[30],char * arguments){
    struct stat buff;
    for (int i = 0; i < 8; i++){
        char string2[30];
        strcpy(string2, stringArr[i]);
        strcat(string2,"/");
        strcat(string2, arguments);
        int ret = stat(string2,&buff);
        if( ret == 0) {
            strcpy(string,string2);
           return 0;
        }
    }
    return -1;
}



int main (void) {
    char hostName[10];
    gethostname(hostName,10);
    char *env = getenv("PATH");
    char *userName = getenv("USER");
    char* stringArr[15];
    split(env,":",stringArr);

    while(1) {
        printf("%s%c%s: ",userName,'%',hostName);
        char *arguments = malloc(50*sizeof(char));
        scanf("%[^\n]%*c",arguments);
        char * pipeUse[10] = {NULL};
        int PIPE = 0;
        split(arguments,"|",pipeUse);
        int num = 0;
        while(pipeUse[num] != NULL){
            num++;
        }
        while(pipeUse[PIPE] != NULL){

            int OperatorUse = 0;
            char * fileName[1];
            int newFd;
            char *argumentArr[10] ={NULL};
            removeLeading(pipeUse[PIPE]);
            split(pipeUse[PIPE]," ",argumentArr);
            int i = 0;
            while(argumentArr[i] != NULL){
                if(strcmp(argumentArr[i],">>")== 0){
                    fileName[0] = argumentArr[i+1];
                    argumentArr[i] = NULL;
                    OperatorUse = 1;
                }
                else if(strcmp(argumentArr[i],">") == 0){
                    fileName[0] = argumentArr[i+1];
                    argumentArr[i] = NULL;
                    OperatorUse = 2;
                }
                else if(strcmp(argumentArr[i],"&") == 0){
                    argumentArr[i] = NULL;
                    OperatorUse = -1;
                }
                else if(strcmp(argumentArr[i],"<")==0){
                    fileName[0] = argumentArr[i+1];
                    argumentArr[i] = NULL;
                    OperatorUse = 3;
                }
                else if(strcmp(argumentArr[i], "exit")==0){
                    return 0;
                }
                i++;
            }
            if(PIPE != num -1){
                pipe(outfd);
            }

            int pid = fork();
            if(0 != pid){
                childId = pid;
            }
            if(signal(SIGINT,sigHandler)==SIG_ERR);
            if(0 == pid){
                if(PIPE == 0 ){
                    write_en(outfd);
                }
                else if (PIPE == num -1){
                    read_en(infd);
                }
                else {
                    write_en(outfd);
                    read_en(infd);
                }
                if(arguments[0] == '$'&& arguments[1] == '?'){
                    printf("-bash: %d: command not found\n",childReturnCode);
                }
                if(OperatorUse != 0) {
                    if(OperatorUse == 1){
                        newFd = open(fileName[0], O_WRONLY| O_APPEND| O_CREAT,0644);
                        dup2(newFd,1);
                    }
                    else if(OperatorUse == 2){
                        newFd = open(fileName[0], O_CREAT|O_WRONLY|O_TRUNC,0644);
                        dup2(newFd,1);
                    }
                    else if (OperatorUse == 3){
                        newFd = open(fileName[0],O_RDONLY);
                        dup2(newFd,0);
                    }
                }
            
                char string[30];
                int isPath = pathFinder(stringArr,string,pipeUse[PIPE]);
                if(isPath == 0){
                    execv(string,argumentArr);
                }
                else if( isPath != 0) {
                    char *args[] = {arguments,NULL};
                    execv(arguments,args);
                }

            }else if(OperatorUse != -1) {
                int stat = 0;
                if(PIPE == 0 ){
                    infd[P_READ] = outfd[P_READ];
                    infd[P_WRITE] = outfd[P_WRITE];
                }
                else if (PIPE == num -1 ){
                    close(infd[P_READ]);
                    close(infd[P_WRITE]);
                }
                else{
                    close(infd[P_READ]);
                    close(infd[P_WRITE]);
                    infd[P_READ] = outfd[P_READ];
                    infd[P_WRITE] = outfd[P_WRITE];
                }
                wait(&stat);
                OperatorUse = 0;
                childReturnCode = WEXITSTATUS(stat);
            }
            PIPE++;
        }
    }
}
