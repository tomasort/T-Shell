

//Name: Tomas Ortega
//PantherID: 5677483

//I affirm that this program is entirely my own work and none of
//it is the work of any other person.

//This program simulates a shell environment in which you can exeute most system calls
//it does not support cd but I/O redirections and pipes are incorporated

#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

//#define BUFSIZ 1024
#define MAX_ARGS 20
#define MAX_CMD 6

int get_args(char* cmdline, char* args[]){
  int i = 0;

  /* if no args */
  if((args[0] = strtok(cmdline, "\n\t ")) == NULL)
    return 0;

  while((args[++i] = strtok(NULL, "\n\t ")) != NULL) {
    if(i >= MAX_ARGS) {
      printf("Too many arguments!\n");
      exit(1);
    }
  }
  /* the last one is always NULL */
  return i;
}

int piper(char* args[], int nargs){
    
	char *input_file;
	char *output_file;
    int in = 0;
    int out = 0;
    int ap = 0;
    int num_cmds = 0;
	char *command[MAX_ARGS];
	int filedes[2]; // 0 output, 1 input of pipe
	int filedes2[2];

	//calculate the number of commands
    int f;
	for(f = 0; f < nargs; f++){
		if (strcmp(args[f], "|") == 0){
			num_cmds++;
		}
	}
	num_cmds++;
	if(num_cmds <= 1){//if it is just one command then return
		return -1;
	}
	int end = 0;
	pid_t pid;
	//execute each command before and after the pipes
    int j = 0;
    int i = 0;
	while(args[j] != NULL && end != 1){
		int k = 0;
		while (strcmp(args[j],"|") != 0){
			command[k] = args[j];
			j++;
			if (args[j] == NULL){
				// end is to keep the program from entering if we are finished
				end = 1;
				k++;
				break;
			}
			k++;
		}
		// Last position of the command will be NULL to indicate that it is its end when we pass it to the exec function
		command[k] = NULL;
		j++;
        
		if(i == 0){
			//if we are in the first command then we want to search for an input file
			int h = 0;
			while(command[h] != NULL){
				if(strcmp(command[h], "<") == 0){
					 input_file = command[h+1];
					 in = 1;
					 command[h] = NULL;
				}
				h++;
			}
		}
		if(i == num_cmds - 1){
            //if we are in the first command then we want to search for an output file
			int l = 0;
			while(command[l] != NULL){

				if(strcmp(command[l], ">") == 0){
					output_file = command[l+1];
					out = 1;
					command[l] = NULL;
				}else if(strcmp(command[l], ">>") == 0){
					out = 1;
					output_file = command[l+1];
					ap = 1;
					command[l] = NULL;
				}
				l++;
			}
		}
        // a pipe will be shared between each two
		// iterations, so that we can connect the inputs and outputs of the commands.
		if (i % 2 != 0){
			pipe(filedes); // i = odd
		}else{
			pipe(filedes2); // i = even
		}
		pid=fork();
		if(pid==-1){
			if (i != num_cmds - 1){
				if (i % 2 != 0){
					close(filedes[1]); // i = odd
				}else{
					close(filedes2[1]); // i = even
				}
			}
			printf("error\n");
			return -4;
		}
		if(pid==0){
			// If we are in the first command
			 if (i == 0){
				if(in){//if there is an input file for the first command
					int input = open(input_file, O_RDONLY);
					dup2(input, STDIN_FILENO);
					close(input);
				}
				dup2(filedes2[1], STDOUT_FILENO);

			}
			else if (i == num_cmds - 1){
                //we are in the last command, depending on if it is in an odd or even position, we will set the standard input for one pipe or another.
				if(out == 1){//only the last comand can use an output file
				     int output;
				     if(ap){
                         //this will only append or create a file
				         output = open(output_file, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
				     }else{
                         //if the file exists then overwrite it if not reate it
				         output = open(output_file, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
				     }
				     dup2(output, STDOUT_FILENO);
				     close(output);
				}

				if (num_cmds % 2 != 0){ // for odd num commands
					dup2(filedes[0],STDIN_FILENO);
				}else{ // for even num commands
					dup2(filedes2[0],STDIN_FILENO);
				}
			// If we are in a command in the middle, we need to use two pipes, one for input and the other one is for the output.
			}else{ //odd i
				if (i % 2 != 0){
					dup2(filedes2[0],STDIN_FILENO);
					dup2(filedes[1],STDOUT_FILENO);
				}else{ //even i
					dup2(filedes[0],STDIN_FILENO);
					dup2(filedes2[1],STDOUT_FILENO);
				}
			}
			execvp(command[0],command);
		}

		//close file descriptors
		if (i == 0){
			close(filedes2[1]);
		}
		else if (i == num_cmds - 1){
			if (num_cmds % 2 != 0){
				close(filedes[0]);
			}else{
				close(filedes2[0]);
			}
		}else{
			if (i % 2 != 0){
				close(filedes2[0]);
				close(filedes[1]);
			}else{
				close(filedes[0]);
				close(filedes2[1]);
			}
		}

		waitpid(pid,NULL,0);

		i++;
	}
	return 0;



}

//this function returns -1 if there are no I/O redirection
//it also modifies args[] and sets input_file, output_file
int searchIO(char* args[], int *nargs, char** input_file, char** output_file, int* in, int* out, int* ap){
	int ioRedirection = -1;
    int i;
    for(i = 0; i < *nargs; i++){
        if(strcmp(args[i], "<") == 0){
            *input_file = args[i+1];
            //we need to remove the < and input_file from args[] so that we just execute the command
            int j;
            for(j = i; j < *nargs; j++){
            	//move what is in args[j+2] to args[j];
            	args[j] = args[j+2];
            }
            //then we need to execute the command
            *nargs = *nargs-2;//since we removed two items from the array then decrease nargs
            *in = 1;
            i = 0;//start from the beginning
            ioRedirection = 1;
        }else if (strcmp(args[i], ">") == 0){
            //if you are here then the output file is in args[i+1]
            *output_file = args[i+1];
            //we need to remove the > and output_file from args[]
            int j;
            for(j = i; j < *nargs; j++){
                //move what is in args[j+2] to args[j];
                if(j < *nargs){
                    args[j] = args[j+2];
                }
            }
            *nargs = *nargs-2;
            *out = 1;
            i = 0;
            ioRedirection = 1;
        }else if (strcmp(args[i], ">>") == 0){
            *output_file = args[i+1];
            int j;
            for(j = i; j < *nargs; j++){
                args[j] = args[j+2];
            }
            *nargs = *nargs-2;
            *out = 1;
            *ap = 1;
            i = 0;
            ioRedirection = 1;
        }

    }
    return ioRedirection;

}

void execute(char* cmdline){

	int pid, async;
  	char* args[MAX_ARGS];
    char *input_file;
    char *output_file;
    int in = 0, out = 0, ap = 0;

    int nargs = get_args(cmdline, args);
    int c = piper(args, nargs);//it returns -1 if there is only one command and there are no pipes
    if(c == -1){
    	searchIO(args, &nargs, &input_file, &output_file, &in, &out, &ap);
    	if(nargs <= 0) return;
    	if(!strcmp(args[0], "quit") || !strcmp(args[0], "exit")) {
    		exit(0);
    	}
    	if(!strcmp(args[nargs-1], "&")) { async = 1; args[--nargs] = 0; }
    	else async = 0;

    	pid = fork();
    	if(pid == 0) { /* child process */
    		if(in == 1){
    			int input = open(input_file, O_RDONLY);
    			dup2(input, STDIN_FILENO);
    			close(input);
    		}
    		if(out == 1){
    			int output;
    			if(ap){
                   		//this will only append or create a file
    				output = open(output_file, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
    			}else{
    				output = open(output_file, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    			}

    			dup2(output, STDOUT_FILENO);
    			close(output);
    		}
    		execvp(args[0], args);
    		/* return only when exec fails */
    		perror("exec failed");
    		exit(-1);
    	} else if(pid > 0) { /* parent process */
    		if(!async) waitpid(pid, NULL, 0);
    		else printf("this is an async call\n");
    	} else { /* error occurred */
    		perror("fork failed");
    		exit(1);
    	}
    }
 }

int main (int argc, char* argv [])
{
  char cmdline[BUFSIZ];

  for(;;) {
    printf("COP4338$ ");
    if(fgets(cmdline, BUFSIZ, stdin) == NULL) {
      perror("fgets failed");
      exit(1);
    }
    execute(cmdline) ;
  }
  return 0;
}
