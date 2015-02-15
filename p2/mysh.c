/* 
 * mysh.c for CS 537 Fall 2014 p2
 * by Sean Morton
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFERSIZE 1024

void check_status(int status);
char** get_args(char* line);
char** get_pipe_args(char** args, int first, int size);

int main(int argc, char* argv[]) {
  char line[BUFFERSIZE];
  char** args;
  int num_args;
  int running = 1;
  
  while(running) {
    printf("mysh> ");

    /* Read in the line of user input */
    fgets(line, BUFFERSIZE, stdin); 
    if (line == NULL) {
      fprintf(stderr, "Error!\n");
      continue;
    } else if (line[0] == '\n') { // check for blank line
      fprintf(stderr, "Error!\n");
      continue;
    }
    line[strlen(line) - 1] = '\0'; // remove trailing new line
    
    /* Build the list of arguments and get number of arguments */
    args = get_args(line);
    if (args == NULL) {
      fprintf(stderr, "Error!\n");
      continue;
    }
    num_args = 0;
    while(args[num_args]) num_args++;
    
    /* Check for built-in commands */ 
    if (strcmp(args[0], "exit") == 0) {
      if (args[1] == NULL) {
        exit(0); 
      } else {
        fprintf(stderr, "Error!\n");
      }
 
    } else if (strcmp(args[0], "cd") == 0) {
      int ret_val;

      if (num_args < 2) {
        ret_val = chdir(getenv("HOME"));
      } else {
        ret_val = chdir(args[1]);
      } 
      if (ret_val == -1) {
        fprintf(stderr, "Error!\n"); 
      }

    } else if (strcmp(args[0], "pwd") == 0) {
      getcwd(line, BUFFERSIZE);
      if (line == NULL) {
        fprintf(stderr, "Error!\n");
      }
      printf("%s\n", line);

    } else {
      
      /* The user has input a non-built-in command */ 
      pid_t pid;
      pid_t pid2;
      int status;
      int overwrite_redirection = 0;  // true if '>' detected
      int append_redirection = 0;     // true if '>>' detected
      int using_pipe = 0;             // true if '|' detected
      int no_operands = 0;            // true if a special feature has no operands
      char* filename;                 // used for file redirection
      char** pipe_args;               // the arguments for the piped command

      /* Check for special features */
      int i;
      for (i = 0; i < num_args; i++) {
        if (strcmp(args[i], ">") == 0) {
          filename = args[i+1];
          if (i == 0 || filename == NULL) {
            no_operands = 1;
            break;
          } 
          overwrite_redirection = 1;
          args[i] = NULL;
          break;
        
        } else if (strcmp(args[i], ">>") == 0) {
          filename = args[i+1];
          if (i == 0 || filename == NULL) {
            no_operands = 1;
            break;
          }
          append_redirection = 1;
          args[i] = NULL;
          break;
        
        } else if (strcmp(args[i], "|") == 0) {
          if (i == 0 || args[i+1] == NULL) {
             no_operands = 1;
             break;
          }
          using_pipe = 1;
          args[i] = NULL; // Terminate args for first command
          pipe_args = get_pipe_args(args, i+1, num_args);
          break;
        }
      }
        
      if (no_operands) {
        fprintf(stderr, "Error!\n");
        continue;
      } 
       
      pid = fork();
      if (pid < 0) {
        fprintf(stderr, "Error!\n"); 
      } else if (pid == 0) {
        
        if (overwrite_redirection) {
          FILE *outfile = fopen(filename, "w");
          int fd = fileno(outfile);
          dup2(fd, fileno(stdout));
        
        } else if (append_redirection) {
          FILE *outfile = fopen(filename, "a"); 
          int fd = fileno(outfile);
          dup2(fd, fileno(stdout));
        
        } else if (using_pipe) {
          int pipefd[2];
          pipe(pipefd);
         
          pid2 = fork();
          if (pid2 < 0) {
            fprintf(stderr, "Error!\n");
          } else if (pid2 == 0) {
            close(pipefd[1]); // Close write end
            dup2(pipefd[0], fileno(stdin)); // Copy read end to standard input
            if ((execvp(pipe_args[0], pipe_args)) == -1 ) {
              fprintf(stderr, "Error!\n"); 
              exit(0);
            } 
          } else {
            close(pipefd[0]); // Close read end
            dup2(pipefd[1], fileno(stdout)); // Copy write end to standard output
            if ((execvp(args[0], args)) == -1 ) {
              fprintf(stderr, "Error!\n");
              exit(0);
            } 
          }
        }
 
        if ((execvp(args[0], args)) == -1 ) {
          fprintf(stderr, "Error!\n");
          exit(0);
        }
      } else {
        pid = waitpid(pid, &status, 0);
        pid2 = waitpid(pid2, &status, 0);
        check_status(status);
      }
    }
    free(args);
  }
  exit(0);
}

/*
 * Checks the status set by the wait() call, and prints out an appropriate
 * message if necessary.
 */
void check_status(int status) {
  if (WIFSIGNALED(status)) {
    printf("warning: child process exited abnormally due to %s\n", strsignal(WTERMSIG(status)));
  }
}

/* 
 * Tokenizes the input string line and returns an array containing of all the tokens.
 * If an error occurs, returns NULL
 */
char** get_args(char* line) {
  int i = 0;
  int size = 10;
  char* token;
  char** res = calloc(sizeof(char*), size*sizeof(char*));

  if (res == NULL) {
    return NULL;
  }
  
  token = strtok(line, " ")  ;
  while (token != NULL) {
    res[i] = token; 
    token = strtok(NULL, " ");
    i++;
    if (i >= size) {
      res = realloc(res, (size*2)*sizeof(char*));
    }
  }
  return res;
}

/*
 * Gets the arguments for the command after a pipe
 * Params:
 *  args is the original set of arguments
 *  first is the index at which the first argument of the command occurs
 *  size is the size of the original set of arguments
 * Example:
 * if args = "ls -l | wc" then return "wc"
 */
char** get_pipe_args(char** args, int first, int size) {
  int j;
  int k = 0;
  char** second_args = calloc(sizeof(char*), size*sizeof(char*));

  for (j = first; j < size; j++) {
    second_args[k] = args[j];
    k++; 
  }
  return second_args;
}

