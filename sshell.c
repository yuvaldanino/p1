#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h> // For PATH_MAX

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
                
                // checking statement to ensure reading right arguments 
                //fprintf(stdout, "Argument is '%s'\n",command->argv[0]);
               
                //execvp -> number of arguments is not known 
                execvp(command->argv[0], command->argv);

                // if fails and doesnt execute 
                perror("execvp");
                exit(EXIT_FAILURE);
        } 


}

void handleBuiltInCommands(struct Command *command, int *shouldContinue) {
        
        //error wrong size
        if(command->argc <= 0){
                perror("no arguments");
                exit(1);               
        }

        if (strcmp(command->argv[0], "exit") == 0){
                
                fprintf(stderr, "Bye...\n");
                *shouldContinue = 0; 

        }else if(strcmp(command->argv[0], "pwd") == 0){
                //pwd
                // declare char for max path size 
                char cwd[PATH_MAX];

                //checks if there exists a path 
                if( getcwd(cwd, sizeof(cwd)) != NULL ){
                        //prints cwd if not NULL
                        printf("%s\n", cwd);   
                }else{
                        perror("getcwd error");
                
                }

        }else if(strcmp(command->argv[0], "cd") == 0){
                // cd

                // ensure cd has argument 
                if(command->argc > 1){
                        //chdir returns 0 if worked correctly, error for other values 
                      if(chdir(command-> argv[1]) != 0 ){
                        // if error chdir
                        perror("chdir");
                      }  
                }else{
                        // no arguments for cd 
                      fprintf(stderr, "cd: missing argument\n");  
                }
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
                // handle the built in commands exit, pwd, cd 
                handleBuiltInCommands(&command, &keepGoing);

                // execute commands 
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
