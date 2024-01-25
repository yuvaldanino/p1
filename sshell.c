#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h> // For PATH_MAX
#include <fcntl.h>
#include <ctype.h>

#define CMDLINE_MAX 512
#define MAX_ARGS 16 // max arguments 
#define PATH_MAX 4096


//include for syscall functions 
#include <sys/types.h>
#include <sys/wait.h>

struct Command{
        char *argv[MAX_ARGS];
        int argc; 

        // output file for redirection 
        char *outfile;  
        // Flag if redirect is needed  1 for > 2 for >>
        int redirect;   
        // pointer to next cmd in pipeline 
        // with this i implement linked lists and I can now see when i have a pipe argument and when its just a regular one 
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
    // Split the command line into segments by pipes
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
                //check for appending
                char *appendRedirect = strstr(segment, ">>");
                if(appendRedirect){
                        *appendRedirect = '\0';
                        currentCmd->outfile = appendRedirect + 2;
                        // 2 indicates >>
                        currentCmd->redirect = 2;
                }
                
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
                        //testing code 
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

        //testing code 
        // Print redirection information, if any
        // if (currentCmd->redirect) {
        //         printf("Redirect to: '%s'\n", currentCmd->outfile);
        // }

        // Prepare for the next command in the pipeline
        // allocate mem for new command and set the current command to the next one as we are in the loop of piping. 
        if (i < numSegments - 1) {
            currentCmd->next = (struct Command *)malloc(sizeof(struct Command));
            currentCmd = currentCmd->next;
        }



    }
}

void executeCommand(struct Command *command, char *fullCMD) {
        // for wait 
        int status;
        pid_t pid;

        pid = fork();

        if(pid == -1){
                perror("fork");
                exit(1);
        }else if (pid > 0){  // parents
                waitpid(pid,&status, 0); 
                fprintf(stderr, "+ completed '%s' [%d]\n", fullCMD, WEXITSTATUS(status));
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

void handleBuiltInCommands(struct Command *command, int *shouldContinue, char *fullCMD) {
        
        int status = 0;

        //error wrong size
        if(command->argc <= 0){
                perror("no arguments");
                exit(1); 
                status = 1;              
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
                        status = 1;
                
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
                        status = 1;   
                      }  
                }else{
                         // no arguments for cd 
                      fprintf(stderr, "cd: missing argument\n");
                      status = 1;   
                }
        }

        fprintf(stderr, "+ completed '%s' [%d]\n", fullCMD, status);

}

char* trimWhitespace(char* str) {
    char* end;

    // Trim leading space
    while(isspace((unsigned char)*str)) str++;

    if(*str == 0)  // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator character
    end[1] = '\0';

    return str;
}


void handleRedirection(struct Command *command, int *std_out){

        //open file 
        int fd; 

        //bug testing
        //fprintf(stderr, "Redirecting to file: '%s'\n", command->outfile);
        char* trimmedFilename = trimWhitespace(command->outfile);

        
        if (command->redirect == 2) {  // Append mode
                fd = open(trimmedFilename, O_WRONLY | O_CREAT | O_APPEND, 0644);
                //test print
                //fprintf(stderr, "append mode, fd: %d\n", fd);
        } else {  // Standard redirection (overwrite mode)
                fd = open(trimmedFilename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        }


        //file no open 
        if (fd == -1){
                perror("open");
                exit(EXIT_FAILURE);  
        }
        
        //fprintf(stderr, "File descriptor for redirection: %d\n", fd);



        //in order to restore stdout -> save where stdout is directed to 
        // dup = duplicate 
        *std_out = dup(STDOUT_FILENO);
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


// struct to store the error codes of the pipe so i can later print them ex:"+ completed 'cat /dev/urandom | base64 -w 80 | head -5' [0][0][0]" 
// had problems with how i get the exit status for each command in the pipe 
struct ProcessInfo {
    pid_t pid;
    char commandString[CMDLINE_MAX];
};


struct ProcessInfo processInfo[MAX_ARGS];
int processCount = 0;

void executePipeline(struct Command *pipeline, char *fullCMD) {
    
    // file descriptor for pipe fd[0] R and fd[1] W 
    int fd[2];  
    // initial fd  in (used to know which pipe input we are on )   
    int fd_in = 0;  
    //cuurent command in pipline 
    struct Command *currentCmd = pipeline;

    processCount = 0;

    // as long as there is a command loops thru commands in pipeline 
    while (currentCmd != NULL) {
        
        // if not last command in pipeline 
        if (currentCmd->next) {
                // create  pipe
            if (pipe(fd) < 0) {
                 // if pipe error 
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        // with new pipe we want to fork process to execute command, we want to connect the command to the pipe so it can read and write to it 

        // new command indicates new process 
        pid_t pid = fork();

        // Child process
        if (pid == 0) {
                
                // Ignore SIGPIPE signal 
                // had an error where exit status was not printing accordingly 
                // when i run ls | nonExsistendCommand return + completed 'ls | nonExsistendCommand ' [1][127] sometimes it would be [127][1]
                //signal(SIGPIPE, SIG_IGN) fixed this 
                // used : https://stackoverflow.com/questions/108183/how-to-prevent-sigpipes-or-handle-them-properly to find fix 
                // website suggestsed to ignore sigpipe to get proper error handeling 
                signal(SIGPIPE, SIG_IGN);  

                // if we arent processsing the first command in the pipe
                if (fd_in != 0) {
                        //curr command reads input from prev command (only works when there is a previous command)
                        dup2(fd_in, STDIN_FILENO);  
                        close(fd_in);
                }
            
            // when last operan has redirect 
            if (!currentCmd->next && currentCmd->redirect){
                // same logic as my redirect function 
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
                // close the write of the pipe  
                close(fd[1]);
            }

            // Execute the command
            execvp(currentCmd->argv[0], currentCmd->argv);  
            
            // reaches here only if error 
            perror("execvp");  
            exit(127);
        } else if (pid > 0){
                processInfo[processCount].pid = pid;
                strncpy(processInfo[processCount].commandString, currentCmd->argv[0], CMDLINE_MAX);
                processCount++;
            

        }else { // fork error 
            perror("fork");
            exit(EXIT_FAILURE);
        }

        // closes input we read from as its already connected to the pipe 
        // same code in child process because we are checking parent
        if (fd_in != 0) {
            close(fd_in);
        }

        // if there is another command in the pipeline 
        if (currentCmd->next) {
            // Close the write end of the current pipe
            close(fd[1]);  
            // Save the read end for the next command
            fd_in = fd[0];  
        }

        // goes onto next command in pipe
        currentCmd = currentCmd->next;
    }

    // Wait for all child processes to finish
    //while (wait(NULL) > 0);


    // Build and print the completion message
    char completionMessage[CMDLINE_MAX] = "";
    snprintf(completionMessage, sizeof(completionMessage), "+ completed '%s' ", fullCMD);

    for (int i = 0; i < processCount; ++i) {
        int status;
        waitpid(processInfo[i].pid, &status, 0);

        //debug statemnt
        //printf("]Command '%s' with PID %d exited with status %d\n", processInfo[i].commandString, processInfo[i].pid, status);


        // Append each process's status to the completion message
        char statusPart[50];
        if (WIFEXITED(status)) {
            snprintf(statusPart, sizeof(statusPart), "[%d]", WEXITSTATUS(status));
        } else {
            snprintf(statusPart, sizeof(statusPart), "[1]");  // Non-normal exit
        }
        //combine 
        strncat(completionMessage, statusPart, sizeof(completionMessage) - strlen(completionMessage) - 1);
    }

    // final message 
    fprintf(stderr, "%s\n", completionMessage);


}




int isBuiltInCommand(struct Command *command) {
    
    // no arguments 
    if (command == NULL || command->argc == 0) {
        return 0; 
    }

    // built in command?
    // exit, pwd, cd 


    if (strcmp(command->argv[0], "exit") == 0) {
        return 1; 
    } else if (strcmp(command->argv[0], "pwd") == 0) {
        return 1; 
    } else if (strcmp(command->argv[0], "cd") == 0) {
        return 1; 
    }
    // otherwise return false 
    return 0; 
}





int main(void){
        char cmd[CMDLINE_MAX]; 
        struct Command command;
        int keepGoing = 1;

        //to store cmd to print complete message 
        char fullCommandLine[CMDLINE_MAX];

        while (keepGoing) {

                /* Print prompt */
                printf("sshell$ ");
                fflush(stdout);  // flushes STDout so it prints instantly 

                // error 
                if (!fgets(cmd, CMDLINE_MAX, stdin)) {
                        perror("fgets");
                        continue;
                }
                
                //remove new line in command 
                size_t len = strlen(cmd);
                if(  len > 0 && cmd[len - 1] == '\n' ) {
                        cmd[len - 1 ] = '\0';
                }
                //copies cmd to fullcommand line to store command line input to print later 
                strcpy(fullCommandLine, cmd);

                // parse the command line 
                parseCommand(cmd, &command);

                //check if we need redirection 

                // handle the built in commands exit, pwd, cd 
                //handleBuiltInCommands(&command, &keepGoing);

                // Check if it's a built-in command
                if(isBuiltInCommand(&command)) {  
                        handleBuiltInCommands(&command, &keepGoing, fullCommandLine);

                // execute no built in commands 
                }else{
                        // if there is a next command meaning its a pipe 
                       

                        // if a redirect flag was signaled
                         if(command.redirect){
                                //save orignal stdout destination 
                                int std_out;
                                // handles the redirction
                                handleRedirection(&command, &std_out);
                                // does command with the new redirect 
                                executeCommand(&command, fullCommandLine);
                                //change stdout back to terminal (or original directed location (std_out))
                                if (command.redirect){
                                        restoreSTDOUT(std_out);
                                }
                        }else if(command.next != NULL){
                                executePipeline(&command, fullCommandLine);
                        }
                        else{
                                executeCommand(&command, fullCommandLine);
                        }
                        
                }
                
                
        }

        return EXIT_SUCCESS;
}

