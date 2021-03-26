#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>


#define LIMIT 256
static char *args[512];
char *input_filename, *output_filename;

#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"

// Function for giving user their current directory
char get_directory()
{
    char cwd[1024];
    // Printing the current directory
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        return printf(ANSI_COLOR_MAGENTA "%s:" ANSI_COLOR_CYAN "~" ANSI_COLOR_RESET "$ " ANSI_COLOR_RESET, cwd); 
    }
    else {
        perror("Can't find directory, try again\n");
    }	
}

void showHelp(){
	printf(ANSI_COLOR_GREEN   "--------------------Help--------------------"   ANSI_COLOR_RESET "\n");
	printf(ANSI_COLOR_CYAN    "Not all the internal commands are supported."   ANSI_COLOR_RESET "\n");
	printf(ANSI_COLOR_CYAN    "Supported internal commands: cd, exit and help "   ANSI_COLOR_RESET "\n");
	printf(ANSI_COLOR_CYAN    "Output redirection to file is supported: ex. ls > fileOutput "   ANSI_COLOR_RESET "\n");
	printf(ANSI_COLOR_CYAN    "Output redirection to file with append is supported: ex. ls >> fileOutput "   ANSI_COLOR_RESET "\n");
	printf(ANSI_COLOR_CYAN    "Input redirection from file is supported: ex. wc -c < fileInput "   ANSI_COLOR_RESET "\n");
}
// Handle input redirect
void handle_ip_redirect(char* file_name)
{
    
    // open file for read-only
    int ip_file_descr = open(file_name, O_RDONLY);
    // 0 is stdin
    dup2(ip_file_descr, 0);
    execvp(args[0], args);
    close(ip_file_descr);
    exit(1);    
    
    
}

// Handle output redirect
void handle_op_redirect(char* file_name)
{
    // open file and either: create file, OR open in write-only mode, OR truncate
    int op_file_descr = open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 0666);
   
    // 1 er stdout
    dup2(op_file_descr, 1);
    
    execvp(args[0], args);
    close(op_file_descr);
    
    exit(1);
}

//  Handle input/output redirect
void handle_IO_redirect(char** args)
{
    int i = 0 ;

    while(args[i] != NULL)
    {
        // Looking for input
        if (! strcmp(args[i], "<"))
        {
            strcpy(args[i], "\0");
            input_filename = args[i+1];
            handle_ip_redirect(input_filename);
            break;
        }
        // Looking for output
        else if(! strcmp(args[i], ">"))
        {
            output_filename = args[i+1];
            args[i] = NULL;
            handle_op_redirect(output_filename);
            
        }
        i++;
    }
    
}


int main(int argc, char **argv)
{
    char buffer[500];
    const char delim[2] = " ";
    char * tokens[LIMIT];
	int numTokens;
    pid_t child_pid;
    int stat_loc;


    printf(ANSI_COLOR_GREEN   "\n----------------------------------- SHELLY -------------------------------------"   ANSI_COLOR_RESET "\n");
    printf(ANSI_COLOR_BLUE   "Our woefully inadequate shell :))"   ANSI_COLOR_RESET "\n");
    while (1)
    {
        /* ----- GET DIRECTORY ----- */
        get_directory();

        /* ----- READ USER INPUT ----- */
		fgets(buffer, 500, stdin);

        /* ----- TOKENIZE ----- */        
        numTokens = 1;
        args[0] = strtok(buffer, " \n\t");
        while ((args[numTokens] = strtok(NULL," \n\t")) != NULL){
            numTokens ++;
        }
        args[numTokens] = NULL;


        /* ----- CD OG EXIT ----- */  
        // When we use the comman cd we want to change the directory for the parent process, not the child process
        // Thats why we need to check for these before we fork 
        if (strcmp(args[0], "cd") == 0) 
        {
            if (chdir(args[1]) < 0) 
            {
                perror(args[1]);
            }
            continue;
        }
        
        else if (strcmp(args[0], "exit") == 0) 
        {
            exit(0);
        }
        else 
        {
            handle_IO_redirect(args);        
        
            /* ----- FORKER HER I FRA ----- */
            child_pid = fork();   

            // Error handling for fork 
            if (child_pid < 0) {
                perror("Fork failed");
                exit(1);
            }

            /* ----- KJØRER EXECVP HERIFRA ----- */
            else if (child_pid == 0) 
            {
                // Never returns if the call is successful
                execvp(args[0], args);
                // If setningene under her blir bare executet dersom linjen over failer
                // Må definere denne her, fordi den er egendefinert
                if (strcmp(args[0],"help") == 0)
                {
                    showHelp();
                }
                // Error handling for execvp 
                else if (execvp(args[0], args) < 0) 
                {
                    perror(args[0]);
                    exit(1);
                }
            } 
            else 
            {
                waitpid(child_pid, &stat_loc, WUNTRACED);
            }
        }
    }
}