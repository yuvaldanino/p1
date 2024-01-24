#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h> // For PATH_MAX
#include <fcntl.h>


#define CMDLINE_MAX 512
#define MAX_ARGS 10
#define PATH_MAX 4096


//include for syscall functions 
#include <sys/types.h>
#include <sys/wait.h>

struct Command{
        char *argv[MAX_ARGS];
        int argc; 

        // output file for redirection 
        char *outfile;  
        // Flag if redirect is needed 
        int redirect;   
        // pointer to next cmd in pipeline 
        struct Command *next; 
};


void parseCommand(char *cmd, struct Command *command) {
        
        
    char *nl = strchr(cmd, '\n');
    if (nl) {
        *nl = '\0';
    }

    char *segments[MAX_ARGS];
    int numSegments = 0;

    // Split the command line into segments by pipes
    char *pipeSegment = strtok(cmd, "|");
    // if no pipe continues 
    while (pipeSegment != NULL && numSegments < MAX_ARGS) {
        segments[numSegments++] = pipeSegment;
        pipeSegment = strtok(NULL, "|");
    }

    struct Command *currentCmd = command;
    for (int i = 0; i < numSegments; i++) {
        
        //intializee Command 
        currentCmd->argc = 0;
        currentCmd->redirect = 0;
        currentCmd->outfile = NULL;
        currentCmd->next = NULL;

        // Process each segment separately
        char *segment = segments[i];
                //check for redirects 
                char *redirectSymb = strchr(segment, '>');
                // if we have redirecy symbol
                if(redirectSymb){
                        //idea: split at redirect, assign command-> outfile and deleted spaces 

                        //point to > and swictch it to null terminator
                        *redirectSymb = '\0';

                        //assign command-> file to actual file which is 1 over > location 
                        currentCmd->outfile = redirectSymb + 1;

                        // indicate a redirect occured 
                        currentCmd->redirect = 1;

                        //to handel weird spaces moves file pointer after spaces 
                        while(*currentCmd->outfile == ' '){
                                currentCmd->outfile++;
                        }


                        //printf("file: %s ", command->outfile);

                }

                
                //space handeling 
                char *token = strtok(segment, " ");
                while (token != NULL && currentCmd->argc < MAX_ARGS) {
                        currentCmd->argv[currentCmd->argc] = token;
                        
                        // Print each argument for debugging
                        //printf("Argument[%d]: %s\n", command->argc, command->argv[command->argc]);

                        currentCmd->argc++;
                        
                        token = strtok(NULL, " ");
                }
                currentCmd->argv[currentCmd->argc] = NULL;



        // Debug print
        printf("Command %d: ", i + 1);
        for (int j = 0; j < currentCmd->argc; j++) {
            printf("%s ", currentCmd->argv[j]);
        }
        printf("\n");


         // Print redirection information, if any
        if (currentCmd->redirect) {
                printf("Redirect to: '%s'\n", currentCmd->outfile);
        }

        // Prepare for the next command in the pipeline
        if (i < numSegments - 1) {
            currentCmd->next = (struct Command *)malloc(sizeof(struct Command));
            currentCmd = currentCmd->next;
        }



    }
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
                //fprintf(stdout, "Return status value for '%s': %d\n",command->argv[0], WEXITSTATUS(status));
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
                        //changes direcory 
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

void handleRedirection(struct Command *command, int *std_out){

        //open file 
        int fd = open(command->outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        //file no open 
        if (fd == -1){
                perror("open");
                exit(EXIT_FAILURE);  
        }

        //in order to restore stdout -> save where stdout is directed to 
        // dup = duplicate 
        *std_out = dup(STDOUT_FILENO);\
        //if no location (error)
        if(*std_out == -1){
                perror("dup");
                close(fd);
                exit(EXIT_FAILURE);    
        }

        // do redirection 
        //if error report error 
        if ( dup2(fd, STDOUT_FILENO) == -1){
                perror("dup2");
                close(fd);
                exit(EXIT_FAILURE);    
        }

        close(fd);

}

void restoreSTDOUT(int std_out){
        //stdout to original file descriptor 
        dup2(std_out, STDOUT_FILENO);
        close(std_out);
}


void executePipeline(struct Command *pipeline) {
    
    // file descriptor for pipe fd[0] R and fd[1] W 
    int fd[2];  
    // initial fd     
    int fd_in = 0;  
    //cuurent command in pipline 
    struct Command *currentCmd = pipeline;

    // as long as there is a command loops thru commands in pipeline 
    while (currentCmd != NULL) {
        // if not last command in pipeline 
        
        if (currentCmd->next) {
                
                // create another pipe
            if (pipe(fd) < 0) {
                 // if pipe error 
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        // new command indicates new process 
        pid_t pid = fork();

        // Child process
        if (pid == 0) {  
            if (fd_in != 0) {
                //curr command reads input from prev command 
                dup2(fd_in, STDIN_FILENO);  
                close(fd_in);
            }
            
            // when last operan has redirect 
            if (!currentCmd->next && currentCmd->redirect){
                int out_fd = open(currentCmd->outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (out_fd < 0) {
                        perror("open");
                        exit(EXIT_FAILURE);
                }
                dup2(out_fd, STDOUT_FILENO);
                close(out_fd);
            }
            
            // if not last command 
            if (currentCmd->next) {
                // Close the read  of the current pipe
                close(fd[0]);  
                // Set up output to the next pipe
                dup2(fd[1], STDOUT_FILENO);  
                close(fd[1]);
            }

            // Execute the command
            execvp(currentCmd->argv[0], currentCmd->argv);  
            // reaches here only if error 
            perror("execvp");  
            exit(EXIT_FAILURE);
        } // fork error 
        else if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        // closes unused file descriptors 
        if (fd_in != 0) {
            close(fd_in);
        }

        if (currentCmd->next) {
            // Close the write end of the current pipe
            close(fd[1]);  
            // Save the read end for the next command
            fd_in = fd[0];  
        }

        currentCmd = currentCmd->next;
    }

    // Wait for all child processes to finish
    while (wait(NULL) > 0);
}

int isBuiltInCommand(struct Command *command) {
    
    if (command == NULL || command->argc == 0) {
        return 0; // If command is NULL or has no arguments, it's not a built-in command
    }

    // Check if the command is one of the built-in commands
    if (strcmp(command->argv[0], "exit") == 0) {
        return 1; // If the command is "exit", return true
    } else if (strcmp(command->argv[0], "pwd") == 0) {
        return 1; // If the command is "pwd", return true
    } else if (strcmp(command->argv[0], "cd") == 0) {
        return 1; // If the command is "cd", return true
    }

    return 0; // If none of the above, return false
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

                //check if we need redirection 

                // handle the built in commands exit, pwd, cd 
                //handleBuiltInCommands(&command, &keepGoing);

                // Check if it's a built-in command
                if(isBuiltInCommand(&command)) {  
                        handleBuiltInCommands(&command, &keepGoing);

                // execute no built in commands 
                }else{
                        // if there is a next command meaning its a pipe 
                        if(command.next != NULL){
                                printf("executePipeline \n ");

                                executePipeline(&command);
                        
                        // if a redirect flag was signaled
                        }else if(command.redirect){
                                //save orignal stdout destination 
                                int std_out;
                                // handles the redirction
                                handleRedirection(&command, &std_out);
                                // does command with the new redirect 
                                executeCommand(&command);
                                //change stdout back to terminal (or original directed location (std_out))
                                restoreSTDOUT(std_out);
                        }else{
                                executeCommand(&command);
                        }
                        
                }
                
                
        }

        return EXIT_SUCCESS;
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