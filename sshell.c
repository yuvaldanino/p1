#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CMDLINE_MAX 512
#define MAX_ARGS 10


//include for syscall functions 
#include <sys/types.h>
#include <sys/wait.h>

struct Command{
        char *argv[MAX_ARGS];
        int argc; 
};


void parseCommand(char *cmd, struct Command *command) {
    char *nl = strchr(cmd, '\n');
    if (nl) {
        *nl = '\0';
    }

    command->argc = 0;
    char *token = strtok(cmd, " ");
    while (token != NULL && command->argc < MAX_ARGS) {
        command->argv[command->argc++] = token;
        token = strtok(NULL, " ");
    }
    command->argv[command->argc] = NULL;
}

void executeCommand(struct Command *command) {
        // for wait 
        int status;
        pid_t pid;

        pid = fork();

        if(pid == -1){
                perror("fork");
                exit(1);
        }else if (pid > 0){  // parents
                waitpid(pid,&status, 0); 
                fprintf(stdout, "Return status value for '%s': %d\n",command->argv[0], WEXITSTATUS(status));
        }else {    // pid is 0 -> child 
                //execvp -> number of arguments is not known 
                fprintf(stdout, "Argument is '%s'\n",command->argv[0]);

                execvp(command->argv[0], command->argv);

                // if fails and doesnt execute 
                perror("execvp");
                exit(EXIT_FAILURE);
        } 


}

void handleBuiltInCommands(struct Command *command, int *shouldContinue) {
        if (command->argc > 0 && strcmp(command->argv[0], "exit") == 0) {
                fprintf(stderr, "Bye...\n");
                *shouldContinue = 0;
    }
}



int main(void){
        char cmd[CMDLINE_MAX]; 
        struct Command command;
        int keepGoing = 1;

        while (keepGoing) {

                /* Print prompt */
                printf("sshell$ ");
                fflush(stdout);  // flushes STDout so it prints instantly 

                // error 
                if (!fgets(cmd, CMDLINE_MAX, stdin)) {
                        perror("fgets");
                        continue;
                }

                // parse the command line 
                parseCommand(cmd, &command);
                handleBuiltInCommands(&command, &keepGoing);

                if (keepGoing){
                        executeCommand(&command);
                }
                


               
                /* Get command line */
                //fgets(cmd, CMDLINE_MAX, stdin);

                /* Print command line if stdin is not provided by terminal */
                /*if (!isatty(STDIN_FILENO)) {
                        printf("%s", cmd);
                        fflush(stdout);
                }*/

                /* Regular command */
                //retval = system(cmd);
                //fprintf(stdout, "Return status value for '%s': %d\n",
                        //cmd, retval);
        }

        return EXIT_SUCCESS;
}
