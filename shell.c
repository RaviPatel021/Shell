#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
// #define _GNU_SOURCE
// #define _POSIX_SOURCE

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>


#include <readline/readline.h>
#include <readline/history.h>


struct JobsList* tail = NULL;
struct JobsList* head = NULL;

struct JobsList{
    int order;
    int running;
    int pgid;
    int pid1;
    int pid2;
    char* command;
    struct JobsList *next;
    struct JobsList *previous;

};

int upgid;
int upid1;
int upid2;
char* ucommand;

void addNode(int pgid, int pid1, int pid2, char * command, int running){
    struct JobsList* newJob = NULL;
    newJob = malloc(sizeof(struct JobsList));

    newJob->pgid = pgid;
    newJob->pid1 = pid1;
    newJob->pid2 = pid2;
    newJob->command = command;
    newJob->running = running;

    if(head==NULL){
        head = newJob;
        tail = newJob;
        newJob->next=NULL;
        newJob->previous=NULL;
    }
    else{
        struct JobsList* temp = head;
        newJob->next = head;
        newJob->previous = NULL;
        head->previous = newJob;
        head = newJob;
    }

    if(newJob->next==NULL){
        newJob->order=1;
    }
    else{
        newJob->order = newJob->next->order + 1;
    }

}

void removeNode(int pgid){
    // printf("remove node");
    struct JobsList* pointer = head;
    while(pointer!=NULL){
        if(pointer->pgid == pgid){
            struct JobsList* ntemp = pointer->next;
            struct JobsList* ptemp = pointer->previous;
            if (ntemp==NULL && ptemp==NULL){
                head = NULL;
                tail = NULL;
            }
            else if(ntemp==NULL){
                tail = ptemp;
                ptemp->next=NULL;
            }
            else if(ptemp==NULL){
                head = ntemp;
                ntemp->previous=NULL;
            }
            else{
                ntemp->previous =ptemp;
                ptemp->next = ntemp;
            }
            return;
        }
        else{
            pointer = pointer->next;
        }
    }

}



int background = 0;
int pgid=0;
int pgid2=0;



char ** divider(char* string, char* arr[]){
    int length =0;
    arr[0]=NULL;
    int numSpaces = 0;
    while(*string != '\0'){
        if(*string == ' '){
            if (length>0){
                char* substring = strndup((string-length), length);
                // printf("%s\n", substring);
                arr[numSpaces] = substring;
                numSpaces ++;
                length = 0;
            }
        }
        else{
            length ++;
        }
        string++;
    }
    if (length>0){
        char* substring = strndup((string-length), length);
        // printf("%s\n", substring);
        length = 0;
        arr[numSpaces] = substring;
        arr[numSpaces+1] = NULL;
        
    }
    else{
        arr[numSpaces] = NULL;
    }
    
    return arr;
}


void remove_element(char* *array, int index)
{
    int x=0;
    while(array[x]!=NULL){
        x++;
    }
    int array_length = x+1;
   int i;
   for(i = index; i < array_length - 1; i++) array[i] = array[i + 1];
}


int isChar (char* c, char** array){
    int i=0;
    while(array[i]!=NULL){
        if(strcmp(array[i], c)==0){
            return i;
        }
        i++;
    }
    return -1;
}

void normalCall(char** array, int numSpaces){
    
    int cpid;
    int stdin1 = 0;
    int stdout1 =0;
    int stderr1 =0;
    int callChild =1;
    int filenumin=0;
    int filenumout=0;
    int filenumerr= 0;

    int pipecomm[2];
    pipe(pipecomm);
    // int in =0;
    // while(array[in]!=NULL){
    //     printf("%s\n", array[in]);
    //     in++;
    // }
    int i=isChar("<",array);
    if(i!=-1){
        if(array[i+1]!=NULL){
            filenumin = open(array[i+1], O_RDWR, 0644);
            if(filenumin==-1){
                callChild=0;
                perror(array[i+1]);
            }
            stdin1 = 1;
            remove_element(array, i);
            remove_element(array, i);                   
                    
        }
    }

    i = isChar(">", array);
    if(i!=-1){
        if(array[i+1]!=NULL){
            filenumout = open(array[i+1], O_RDWR | O_CREAT | O_APPEND, 0644);
            stdout1 = 1;
            remove_element(array, i);
            remove_element(array, i);
        }
    }

    i = isChar("2>", array);
    if(i!=-1){
        if(array[i+1]!=NULL){
            filenumerr = open(array[i+1], O_RDWR | O_CREAT | O_APPEND, 0644);
            stderr1 =1;
            remove_element(array, i);
            remove_element(array, i);
        }
    }    
    // in =0;
    // while(array[in]!=NULL){
    //     printf("%s\n", array[in]);
    //     in++;
    // }
    int currpgrp = getpgrp();
    
    if(callChild==1){
        cpid = fork();
        if(cpid == 0){
            setpgid(0,0);
            tcsetpgrp(0, getpgrp());
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            int childpgid = getpgrp();
            close(pipecomm[0]);
            write(pipecomm[1], &childpgid, sizeof(childpgid));
            close(pipecomm[1]);

            // printf("%i \n", childpgid);
            if(stdin1==1){
                dup2(filenumin, 0);
            }
            if(stdout1==1){
                dup2(filenumout, 1);
            }
            if(stderr1==1){
                dup2(filenumerr, 2);
            }

            if(execvp(array[0], array)==-1){
                exit(0);
            }
        }
        close(pipecomm[1]);
        read(pipecomm[0], &pgid, sizeof(pgid));
        upid1 = pgid;
        upgid = pgid;
        close(pipecomm[0]);
        // else{
        //     wait((int*)NULL);
        // }
        int status;
        if(background==1){
            waitpid(cpid, &status, WUNTRACED | WNOHANG);
            addNode(upgid, upid1, upid2, ucommand, 1);
        }
        else{
            waitpid(cpid, &status, WUNTRACED);
            if(waitpid(cpid, &status, WNOHANG)==0){
                addNode(upgid, upid1, upid2, ucommand, 0);
            }
        }
        tcsetpgrp(0, currpgrp);
        
    }

}

void pipeCall(char** array, int pos){

    char** arrayOne = array;
    char** arrayTwo = &array[pos+1];
    array[pos] = NULL;
    int callChild =1;
    int cpid;
    int childpid1;
    int childpid2;


    int stdin1 = 0;
    int stdout2 =0;
    int stderr1 =0;
    int stderr2 =0;
    int filenumin1=0;
    int filenumout2=0;
    int filenumout1= 0;
    int filenumerr1= 0;
    int filenumerr2= 0;
    int filenumin2 = 0;
    
    int x=isChar("<",arrayOne);
    if(x!=-1){
        if(arrayOne[x+1]!=NULL){
            filenumin1 = open(arrayOne[x+1], O_RDWR, 0644);
            if(filenumin1==-1){
                callChild=0;
                perror(arrayOne[x+1]);
            }
            stdin1 = 1;
            remove_element(arrayOne, x);
            remove_element(arrayOne, x);                   
                    
        }
    }
    x = isChar("2>", arrayOne);
    if(x!=-1){
        if(arrayOne[x+1]!=NULL){
            filenumerr1 = open(arrayOne[x+1], O_RDWR | O_CREAT | O_APPEND, 0644);
            stderr1 =1;
            remove_element(arrayOne, x);
            remove_element(arrayOne, x);
        }
    }
    x = isChar(">", arrayOne);
    if(x!=-1){
        if(arrayOne[x+1]!=NULL){
            filenumout1 = open(arrayOne[x+1], O_RDWR | O_CREAT | O_APPEND, 0644);
            // stderr1 =1;
            remove_element(arrayOne, x);
            remove_element(arrayOne, x);
        }
    }


    x = isChar(">", arrayTwo);
    if(x!=-1){
        if(arrayTwo[x+1]!=NULL){
            filenumout2 = open(arrayTwo[x+1], O_RDWR | O_CREAT | O_APPEND, 0644);
            stdout2 = 1;
            remove_element(arrayTwo, x);
            remove_element(arrayTwo, x);
        }
    }

    x = isChar("2>", arrayTwo);
    if(x!=-1){
        if(arrayTwo[x+1]!=NULL){
            filenumerr2 = open(arrayTwo[x+1], O_RDWR | O_CREAT | O_APPEND, 0644);
            stderr2 =1;
            remove_element(arrayTwo, x);
            remove_element(arrayTwo, x);
        }
    }

    x=isChar("<",arrayTwo);
    if(x!=-1){
        if(arrayTwo[x+1]!=NULL){
            filenumin2 = open(arrayTwo[x+1], O_RDWR, 0644);
            if(filenumin2==-1){
                callChild=0;
                perror(arrayTwo[x+1]);
            }
            remove_element(arrayTwo, x);
            remove_element(arrayTwo, x);                   
                    
        }
    }

    int currpgrp = getpgrp();
    
    if(callChild==1){
        int pipefd[2];
        pipe(pipefd);
        int pipecomm1[2];
        pipe(pipecomm1);
        int pipecomm2[2];
        pipe(pipecomm2);
        if(filenumout1==0){
            filenumout1 = pipefd[1];
        }
        if(filenumin2==0){
            filenumin2 = pipefd[0];
        }
        // sleep(5);

        cpid = fork();
        if(cpid==0){
            setpgid(0,0);
            int childpid1= getpid();
            close(pipecomm1[0]);
            write(pipecomm1[1], &childpid1, sizeof(childpid1));
            close(pipecomm1[1]);
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            close(pipefd[0]);
            dup2(filenumout1, STDOUT_FILENO);
            
            if(stdin1==1){
                dup2(filenumin1, 0);
            }
            if(stderr1==1){
                dup2(filenumerr1, 2);
            }
            
            if(execvp(arrayOne[0], arrayOne)==-1){
                exit(0);
            }

        }
        close(pipecomm1[1]);
        read(pipecomm1[0], &childpid1, sizeof(childpid1));
        pgid = childpid1;
        upid1 = pgid;
        upgid = pgid;

        close(pipecomm1[0]);

        cpid =fork();
        if(cpid==0){
            setpgid(0, childpid1);
            tcsetpgrp(0, childpid1);
            int childpid2= getpid();
            close(pipecomm2[0]);
            write(pipecomm2[1], &childpid2, sizeof(childpid2));
            close(pipecomm2[1]);
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            close(pipefd[1]);
            dup2(filenumin2, STDIN_FILENO);
            if(stdout2==1){
                dup2(filenumout2, 1);
            }
            if(stderr2==1){
                dup2(filenumerr2, 2);
            }
            if(execvp(arrayTwo[0], arrayTwo)==-1){
                exit(0);
            }
        }

        close(pipefd[1]);
        close(pipefd[0]);
        close(pipecomm2[1]);
        read(pipecomm2[0], &childpid2, sizeof(childpid2));
        pgid2 = childpid2;
        upid2 = pgid2;
        close(pipecomm2[0]);

        int status=0;
        if(background==1){
            waitpid(childpid1, &status, WUNTRACED | WNOHANG);
            waitpid(childpid2, &status, WUNTRACED | WNOHANG);
            addNode(upgid, upid1, upid2, ucommand,1);
        }
        else{
            waitpid(childpid1, &status, WUNTRACED);
            waitpid(childpid2, &status, WUNTRACED);
            if(waitpid(childpid1, &status, WNOHANG)==0 || waitpid(childpid2, &status, WNOHANG)==0){
                addNode(upgid, upid1, upid2, ucommand, 0);
            }

        }
        tcsetpgrp(0, currpgrp);
        

    }


    // int i=0;
    // while(arrayOne[i]!=NULL){
    //     printf("%s ", arrayOne[i]);
    //     i++;
    // }
    // i=0;
    // printf("\n");
    // while (arrayTwo[i]!=NULL)
    // {
    //     printf("%s ", arrayTwo[i]);
    //     i++;
    // }
    // printf("\n");
    
}

struct JobsList* nextJob(){
    if (head==NULL){
        return NULL;
    }
    else{
        struct JobsList* pointer = head;
        while(pointer!=NULL){
            if(pointer->running==0){
                return pointer;
            }
            pointer= pointer->next;
        }
        return head;
    }

}

void finishedJobs(){

    // printf("finished jobs");
    int status =0;
    struct JobsList* pointer = tail;
    while(pointer!=NULL){
        if(pointer->running==1){
            if(waitpid(pointer->pid1, &status, WNOHANG)!=0 &&(pointer->pid2==-1 || waitpid(pointer->pid2, &status, WNOHANG)!=0)){
                char * str = "DONE\0";
                printf("[%i] %s %s\n", pointer->order, str, pointer->command);
                fflush(stdout);
                removeNode(pointer->pgid);
            }
        }
        pointer = pointer->previous;

    }
}
        // if(pointer->running==1){
            

//             if(waitpid(pointer->pid1, &status, WNOHANG)==-1 &&(pointer->pid2==-1 || waitpid(pointer->pid2, &status, WNOHANG)==-1)){
//                 int i=0;
//                 char ** strarr = pointer->command;
//                 while(strarr[i]!=NULL){
//                     printf("%s ", strarr[i]);
//                     i++;
//                 }
//                 removeNode(pointer->pgid);
                
            // }
        // }
//         pointer = pointer->previous;
//     }
// }


int main(){

    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    char* inString;
    
    while(inString = readline("#")){
        finishedJobs();
        // printf("HI");
        int numSpaces=0;
        char* counter = inString;
        while(*counter != '\0'){
            if(*counter == ' '){
                numSpaces++;
            }
            counter++;
        }
        // printf("%i\n", numSpaces);
        char* arr[numSpaces+2];

        char** test = divider(inString, arr);

        char *fullstring= malloc(sizeof(char)*2000) ;
        *fullstring = '\0';
        int i=0;
        while(test[i]!=0){
            strcat(fullstring, test[i]);
            strcat(fullstring, " ");
            i++;
        }

        ucommand = fullstring;
        upid2 = -1;
        int index=0;
        while(test[index]!=NULL){
            // printf("%s\n",storedcommand[index]);
            index++;
        }

        background=0;
        if(index!= 0){
            if(strcmp(test[index-1], "&")==0){
                background=1;
                test[index-1]=NULL;
            }
        }
        // printf("HI");
        int pid = 0;
        if (pid==0){
            int x =isChar("|",test);
            if(x!=-1){
                // printf("%i pipe present\n", x);
                pipeCall(test, x);
            }
            else if(test[0]!=NULL && test[1]==NULL && strcmp(test[0], "fg")==0){
                struct JobsList* jl = nextJob();
                if(jl!=NULL){

                    int status=0;
                    int pgid1stat =waitpid(jl->pid1, &status, WNOHANG);
                    int pgid2stat;
                    if(jl->pid2!=-1){
                        pgid2stat =waitpid(jl->pid2, &status, WNOHANG);
                    }
                    
                    int forknum = fork();
                    int currpgid = getpgrp();
                    if(forknum==0){
                        
                        // printf("%i \n", pgid);
                        // printf("%i \n",tcsetpgrp(0, pgid));
                        tcsetpgrp(0, jl->pgid);
                        kill(-(jl->pgid), SIGCONT);
                        signal(SIGTTOU, SIG_IGN);
                        // signal(SIGTSTP, SIG_DFL);
                        int num=0;
                        // waitpid(pgid, &num, WIFEXITED);
                        // waitpid(pgid, &num, 0);
                    
                        // tcsetpgrp(0, getpgrp());
                        // printf("hi %i\n", tcgetpgrp(0));
                        return 0;
                    }
                    // int status=0;
                    // if(pgid1stat==0){
                    //     waitpid(jl->pid1, &status, WCONTINUED|WUNTRACED);
                    // }
                    // if(jl->pid2!= -1 && pgid2stat==0){
                    //     waitpid(jl->pid2, &status, WCONTINUED|WUNTRACED);
                    // }
                    if(pgid1stat==0){
                        waitpid(jl->pid1, &status, WUNTRACED);
                    }
                    if(jl->pid2!= -1 && pgid2stat==0){
                        waitpid(jl->pid2, &status, WUNTRACED);
                    }
                    if(waitpid(jl->pid1, &status, WNOHANG) !=0 && (jl->pid2==-1 || waitpid(jl->pid2, &status, WNOHANG)!=0)){
                        removeNode(jl->pgid);
                    }
                    else{
                        jl->running=0;
                    }
                    // printf("continued");
                
                    tcsetpgrp(0, currpgid);
                    // printf("completed");
                }
                
            }
            else if(test[0]!=NULL && test[1]==NULL && strcmp(test[0], "bg")==0){
                struct JobsList* jl = nextJob();
                if(jl!=NULL){
                    int status=0;
                    int pgid1stat =waitpid(jl->pid1, &status, WNOHANG);
                    int pgid2stat;
                    if(jl->pid2!=-1){
                        pgid2stat =waitpid(jl->pid2, &status, WNOHANG);
                    }
                    int forknum = fork();
                    if(forknum==0){
                        signal(SIGTTOU, SIG_IGN);
                        kill(-(jl->pgid), SIGCONT);
                        return 0;
                    }
                    // if(pgid1stat==0){
                    //     waitpid(jl->pid1, &status, WCONTINUED);
                    // }
                    
                    // if(jl->pid2!= -1 && pgid2stat==0){
                    //     waitpid(jl->pid2, &status, WCONTINUED);
                    // }

                    if(pgid1stat==0){
                        waitpid(jl->pid1, &status, WUNTRACED|WNOHANG);
                    }
                    
                    
                    if(jl->pid2!= -1 && pgid2stat==0){
                        waitpid(jl->pid2, &status, WUNTRACED|WNOHANG);
                    }
                    jl->running=1;

                }
                

                
            }
            else if(test[0]!=NULL && test[1]==NULL && strcmp(test[0], "jobs")==0){
                struct JobsList* pointer = tail;
                struct JobsList* nextexec = nextJob(); 
                while(pointer !=NULL){
                    char * str = "STOPPED\0";
                    if(pointer->running==1){
                        str = "RUNNING\0";
                    }
                    
                    // int i=1;
                    // while(pointer->command[i] !=NULL){
                    //     strcat(comm, " ");
                    //     strcat(comm, pointer->command[i]);
                    //     i++;
                    // }
                    // printf("[%i] %s \t %s", pointer->order, str, comm);
                    if(nextexec->pgid == pointer->pgid){
                        printf("[%i]+ %s %s\n", pointer->order, str, pointer->command);
                    }
                    else{
                        printf("[%i]- %s %s\n", pointer->order, str, pointer->command);
                    }
                    
                    pointer= pointer->previous;
                }
                // printf("%i", count);

            }
            else{
                normalCall(test, numSpaces);
            }
        }
        background=0;

    }
}