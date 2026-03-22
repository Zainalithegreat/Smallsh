// DO NOT DELETE ANY OF THESE INCLUDES. YOU MAY ADD MORE IF YOU NEED.
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>

void set_process_limit(int bound); // DO NOT DELETE THIS LINE OF CODE

volatile sig_atomic_t fgm = 0; // This is used to switch between foreground and background

// This is the command stuct which will store all of the users commands 
struct Command{
	char * arguments[513]; 	// used to store the arguments
	char * inputFile;	// used to store the inputFile
	char * outputFile;	// used to store the outputFile
	int background;		// used to check if command is background or foreground
	int size;		// size is used as a incrementer for arguments
};

// This function parse the commands and splits them up 
void parse(struct Command* c, char *line_of_text){
	// here we first initilize all the values
	c->size = 0;
	for(int i = 0; i < 513; i++){
		c->arguments[i] = NULL;
	}
	c->inputFile = NULL;
	c->outputFile = NULL;
	c->background = 0;

	// Then we set up strtok so it breaks the string (line_of_text) when it sees " ",\t, and \n
	char *token = strtok(line_of_text, " \t\n");
	
	// We check if token did not return NULL
	while(token != NULL){
		// We compare the value < with token if it is that value we copy the next value into inputFile
		if(strcmp(token, "<") == 0){
			token = strtok(NULL, " \t\n");
			c->inputFile = token;
		// Compare token with > if it matches we copy the next value into outputFile
		}else if(strcmp(token, ">") == 0){
			token = strtok(NULL, " \t\n");
			c->outputFile = token;
		// Compare toekn with & if it matches we assign background to 1
		}else if(strcmp(token, "&") == 0){
			c->background = 1;
		// After all the if and else if we know that this value is going to be arguments
		// So we copy it into arguments given size
		}else{
			c->arguments[c->size++] = token;
		}
		
		token = strtok(NULL, " \t\n");

	}
	// The last value of arguments will be NULL for the exec function
	c->arguments[c->size] = NULL;
}

// We expand dollar sign so it prints the pid of the smallsh ($$)
void expandDollars(char* line, size_t cap) {
	// we get the pid here
    	pid_t pid = getpid();
    	char pid_str[32];
	// We write the pid into pid_str given the size of byte (sizeof(pid_str))
    	snprintf(pid_str, sizeof(pid_str), "%d", (int)pid);
	
	// This is for output buffer
    	char out[2049];                 // assignment cap + 1
    	size_t oi = 0;
	
	// We scan through the input line
    	for (size_t i = 0; line[i] != '\0' && oi < sizeof(out) - 1; ) {
		// If the line and line + 1 is $$
        	if (line[i] == '$' && line[i+1] == '$') {
            	// append pid_str to out
            		for (size_t k = 0; pid_str[k] != '\0' && oi < sizeof(out) - 1; k++) {
				// Here we append the whole pid_str to out
                		out[oi++] = pid_str[k];
            		}
            		i += 2; // skip both $
        	} else {
            		out[oi++] = line[i++]; // If not $$ we add line to out
        	}
    	}
	
	// NULL TERMINATING out
    	out[oi] = '\0';

    	// copy back into line (make sure it fits)
    	strncpy(line, out, cap - 1);
    	line[cap - 1] = '\0';
}

// Removes the pid from a given array
void removePid(pid_t arr[], int *count, pid_t pid){
	// from 0 to count
	for(int i = 0; i < *count; i++){
		// if at i mathces pid we copy the last element to i, decrement count, and return
		if (arr[i] == pid){
			arr[i] = arr[*count -1];
			(*count)--;
			return;
		}
	}
}

// This is a signal handler for SIGTSTP
void signstp_handler(int signo){
	// Printing for foreground and background
	char arr_fg[100] = "Entering foreground-only mode (& is now ignored)";
	char arr_bg[100] = "Exiting foreground-only mode";

	// Checking if signo is SIGTSTP
	if(signo == SIGTSTP){
		// if foregroundmode is 0 then we cheange fgm to 1 and write arr_fg to terminal 
		if(fgm == 0){
			fgm = 1;
			// Here we write arr_fg to the terminal given strlen(arr_fg) bytes
			write(STDOUT_FILENO, arr_fg, strlen(arr_fg));
			write(STDOUT_FILENO, "\n:", 2);
		}else if(fgm == 1){
			fgm = 0;
			// Here we write arr_bg to the terminal given strlen(arr_bg) bytes
			write(STDOUT_FILENO, arr_bg, strlen(arr_bg));
			write(STDOUT_FILENO, "\n:", 2);
		}
	}
}

int main() {
	// DO NOT DELETE THIS LINE OF CODE. IT HELPS PREVENT FORKBOMBS. When the
	// program first starts, this line of code determines how many processes
	// are currently running on your user account. It then adds 30 to that
	// value and sets this sum, which I'll denote as K, as the process limit.
	// From that point on, whenever this process or any of its descendents
	// call the fork() function, it will fail (and return -1) if the
	// number of processes running on your user account is already greater than
	// or equal to K. Basically, this prevents smallsh and its descendents from
	// creating more than 30 total processes that exist concurrently.
	// Importantly, this includes zombie processes, so you must correctly
	// clean up your child processes via wait() / waitpid(), or else the
	// zombies will build up (and once there are 30 of them, fork() will start
	// failing, as intended).
	set_process_limit(30);



	// TODO The rest of your program should start below this line. See the
	// pseudocode in Canvas to get an idea of where to start
	
	// Creating a few sigaction structs
	struct sigaction sigint_sa = {0};
	struct sigaction sigstp_sa = {0};

	// Here we ignore sigint by setting the sa_handler to SIG_IGN
	sigint_sa.sa_handler = SIG_IGN;
	// Here we specify the SIGINT to be ignored
	sigaction(SIGINT, &sigint_sa, NULL);

	// Here we block all signals while SIGTSTP handler executes
	sigfillset(&sigstp_sa.sa_mask);
	// This is to restart system level processes when a signal get called
	sigstp_sa.sa_flags = SA_RESTART;

	// assigning the function to sa_handler
	sigstp_sa.sa_handler = signstp_handler;
	// Here we secify the SIGTSTP 
	sigaction(SIGTSTP, &sigstp_sa, NULL);
	
	
	// Vars for pids, status, and much more
	int status = 0;
	int bgCount = 0;
	pid_t bgPids[200];
	char* line_of_text = NULL;
	size_t buffer_size = 0;
	while(1){
		// Vars for Child processes
		int childStatus = 0;
		pid_t donePid = 0;
		int runBackground = 0;

		// Here we check if the child background process has completed
		while((donePid = waitpid(-1, &childStatus, WNOHANG)) > 0){
			printf("background pid %d is done: ", donePid);
			
			// Checking if child process terminated normally
			if(WIFEXITED(childStatus)){
				// If terminated normal we print the exit code
				printf("exit value %d\n", WEXITSTATUS(childStatus));
			// Check if process terminated due to a signal
			}else if (WIFSIGNALED(childStatus)) {
				// We print the signal if terminated due to a signal
				printf("terminated by signal %d\n", WTERMSIG(childStatus));
			}
			// Forces output to appear immediately
			fflush(stdout);

			// We now remove the background pid that has concluded
			removePid(bgPids, &bgCount, donePid);

		}

		// we print : and force output
		printf(":");
		fflush(stdout);

		// We read a line from the user (Command)
		ssize_t line_length = getline(&line_of_text, &buffer_size, stdin);

		// Check if getline failed
		if(line_length == -1){
			clearerr(stdin);
			continue;
		}

		// if the line is 1 we skip (\n)
		if (line_length == 1) continue;
		
		// if the first char is # (comment) we skip
		if(line_of_text[0] == '#') continue;
		
		// Here we change $$ in our command to the shells pid
		expandDollars(line_of_text, buffer_size);

		// Creating the command struct
		struct Command c;
		// Parseing the char arr and storing the parsed values into command c
		parse(&c, line_of_text);

		// if there are no commands we skip
		if(c.size == 0) continue;
		
		// We check if foregroundmode is 0 this is becuase of foregound and background toggle
		if(fgm == 0){
			// If background is 1 we assign it to runBackground else we assign it 0
			if(c.background == 1){
				runBackground = c.background;
			}else{
				runBackground = 0;
			}
		}
		
		// Here we check if the command is exit
		if (strcmp(c.arguments[0], "exit") == 0) {

			// We kill all the background processes
			for(int i = 0; i < bgCount; i++){
				kill(bgPids[i], SIGKILL);
			}
			break;
		}
		// We check if the command is cd
		else if (strcmp(c.arguments[0], "cd") == 0) {
			// Assign (e.g cd CS374) to path
			char * path = c.arguments[1];

			// We check if path is NULL if so we assign path to the HOME path
			if(path == NULL){
				path = getenv("HOME");
			}
			// Change dir
			chdir(path);
			continue;

		}
		// We check if the command is status
		else if (strcmp(c.arguments[0], "status") == 0) {
			// We check if terminated properly
			if(WIFEXITED(status)){
				// Print the exited code
				printf("exit value %d\n", WEXITSTATUS(status));
			// We check if terminated by signal
			}else if (WIFSIGNALED(status)) {
				// Print the signal exit code
        			printf("terminated by signal %d\n", WTERMSIG(status));
			}else{
				// Else print exit code 0
				printf("exit value 0\n");
			}
			fflush(stdout);
			continue;
		}
		// Executing other commands
		else{
			// Fork here
			pid_t fork_result = fork();
			
			// Check if fork worked
			if(fork_result == -1){
				perror("fork");
				continue;
			}

			// This is the child process
			if(!fork_result){
				// Creating sigaction structs
				struct sigaction sigint_da = {0};
				struct sigaction sig_process = {0};
				// setting the sa_handler for SIGINT to default behavior
				sigint_da.sa_handler = SIG_DFL;
				sigaction(SIGINT, &sigint_da, NULL);
				
				// We want to ignore the SIGTSTP in all child processes
				sig_process.sa_handler = SIG_IGN;
				sigaction(SIGTSTP, &sig_process, NULL);

				// outputFile not NULL
				if(c.outputFile != NULL){
					// Here we open a file to write-only: create/add giving it 644 as permissions
                                        int fdo = open(c.outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
					// Checking if open worked
					if(fdo == -1){
						perror(c.outputFile);
						exit(1);
					}
					// This is used to redirect STD ouput to fdo
                                        dup2(fdo, STDOUT_FILENO); 
					//closing fdo
					close(fdo);
				}
				// inputFile is not NULL
                                if(c.inputFile != NULL){
					// Here we open a file to read-only
                                        int fdi = open(c.inputFile, O_RDONLY); 
					// Check if open worked
					if (fdi == -1){
						perror(c.inputFile);
						exit(1);

					}
					// Redirect STD input to fdi
					dup2(fdi, STDIN_FILENO); 
					// Closing fdi
					close(fdi);
				}
				
				// If the user specified &
				if(runBackground == 1){
					// We create a sigaction struct
					struct sigaction sigint_ca = {0};
					//We want to ignore SIGINT in the background processes
					sigint_ca.sa_handler = SIG_IGN;
					sigaction(SIGINT, &sigint_ca, NULL);
					
					// Checking inputFile is NULL
					if(c.inputFile == NULL){
						// We open /dev/null in read-only
						int devNullIn = open("/dev/null", O_RDONLY);
						// We check open worked
						if(devNullIn == -1){
							perror("open");
							exit(1);
						}
						// Redirecting STD INPUT to devNullIn
						dup2(devNullIn, STDIN_FILENO);
						// Close devNUllIn
						close(devNullIn);
					}
					// Checking if outputFile is NULL
					if(c.outputFile == NULL){
						// We opne /dev/null in write-only
						int  devNullOut = open("/dev/null", O_WRONLY);
						// We check open worked
						if(devNullOut == -1){
							perror("open");
							exit(1);
						}
						// Redirectind STD Output to devNullout
						dup2(devNullOut, STDOUT_FILENO);
						// Close devNullOut
						close(devNullOut);

					}
				}
				
				// This is used to execute commands and replace the process with specifed command e.g ls
				// execvp takes 2 parameters, 1 is the command 
				// the 2nd is an array e.g "ls", ["ls", "-l", NULL];
				// This with print all files/dirs and their permissions
				execvp(c.arguments[0], c.arguments);
				// Checks if the command is incorrect if so prints an error
				perror(c.arguments[0]);
				// Exit out if execvp fails
				exit(1);

			// This is the parent process
			}else{
				// If background is not 0
				if(runBackground){
					// Then we add the childs pid to the array
					bgPids[bgCount++] = fork_result;
					// Prints
					printf("background pid is %d\n", fork_result);
					// Force output
					fflush(stdout);
				// Foregorund
				}else {
					// We clean the child here (also waiting until the child has completed)
					waitpid(fork_result, &status, 0);
					// We check if foreground child has terminated with a signal
					if(WIFSIGNALED(status)){
						// Prints the terminaled signal
						printf("terminated by signal %d\n", WTERMSIG(status));
						// forces output
						fflush(stdout);
					}
				}
			}
		}
	}

	free(line_of_text);
	return 0;

}


// DO NOT MODIFY ANYTHING BELOW THIS LINE OF CODE
void set_process_limit(int bound) {
	int pipe_fds[2];
	int pipe_result = pipe(pipe_fds);
	if (pipe_result) {
		fprintf(stderr, "Error on pipe() in set_process_limit()\n");
		exit(1);
	}
	
	pid_t fork_result = fork();
	if (fork_result == 0) {
		close(pipe_fds[0]);
		char* user = getenv("USER");
		char command[256];
		sprintf(command, "ps --no-headers auxwwwm | awk '$2 == \"-\" { "
			"print $1 }' | grep %s | wc -l", user);
		dup2(pipe_fds[1], STDOUT_FILENO);
		system(command);
		close(pipe_fds[1]);
		exit(0);
	} else if (fork_result > 0) {
		// Retrieve result of pgrep -u USER -c from the pipe, written by the
		// child process
		close(pipe_fds[1]);
		char captured[256] = {0};
		int read_result;
		size_t total_bytes_read = 0;
		while ((read_result = read(
					pipe_fds[0],
					captured + total_bytes_read,
					255 - total_bytes_read)
				) > 0) {
			total_bytes_read += read_result;
		}
		if (read_result != 0) {
			fprintf(stderr, "Error on read() in set_process_limit()\n");
			exit(1);
		}
		close(pipe_fds[0]);
		int exit_info;
		waitpid(fork_result, &exit_info, 0);
		errno = 0;
		// subtract 5 to account for all the child processes
		int n_cur_procs = strtol(captured, NULL, 10) - 5;
		if (errno) {
			fprintf(stderr, "Error on strtol() in set_process_limit()\n");
			exit(1);
		}

		// Set RLIMIT_NPROC
		int soft_proc_limit = n_cur_procs + bound;
		struct rlimit proc_limit = {0};
		int result = getrlimit(RLIMIT_NPROC, &proc_limit);
		if (result) {
			fprintf(stderr, "Error on getrlimit() in set_process_limit()\n");
			exit(1);
		}
		if (soft_proc_limit < proc_limit.rlim_cur) {
			proc_limit.rlim_cur = soft_proc_limit;
		}
		result = setrlimit(RLIMIT_NPROC, &proc_limit);
		if (result) {
			fprintf(stderr, "Error on setrlimit() in set_process_limit()\n");
			exit(1);
		}
	} else {
		fprintf(stderr, "Error on fork() in set_process_limit()\n");
		exit(1);
	}
}
