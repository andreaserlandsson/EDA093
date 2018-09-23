/* 
 * Main source code file for lsh shell program
 *
 * You are free to add functions to this file.
 * If you want to add functions in a separate file 
 * you will need to modify Makefile to compile
 * your additional functions.
 *
 * Add appropriate comments in your code to make it
 * easier for us while grading your assignment.
 *
 * Submit the entire lab1 folder as a tar archive (.tgz).
 * Command to create submission archive: 
      $> tar cvf lab1.tgz lab1/
 *
 * All the best 
 */


#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "parse.h"
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

/*
 * Function declarations
 */

void PrintCommand(int, Command *);
void PrintPgm(Pgm *);
void stripwhite(char *);
void get_pgm(Command *c);
void execute_command(Command *c);
void init_command(Command *c);
int backround_process(Command *c);
void execute_pipes(Command *c);
void signal_handler();

/* When non-zero, this global means the user is done using this program. */
int done = 0;

/*
 * Name: main
 *
 * Description: Gets the ball rolling...
 *
 */
int main(void) {
	Command cmd;
	int n;

	signal(SIGINT, signal_handler);
	while (!done) {
		char *line;
		line = readline(">>> ");

		if (!line) {
		/* Encountered EOF at top level */
		done = 1;
		}
		else {
	/*
	* Remove leading and trailing whitespace from the line
	* Then, if there is anything left, add it to the history list
	* and execute it.
	*/
		stripwhite(line);

		if(*line) {
			add_history(line);
			/* execute it */
			n = parse(line, &cmd);
			PrintCommand(n, &cmd);
			init_command(&cmd);
		}
    }
    
	if(line) {
		free(line);
    	}
	}
	return 0;
}

/*
 * Name: PrintCommand
 *
 * Description: Prints a Command structure as returned by parse on stdout.
 *
 */
void PrintCommand (int n, Command *cmd) {
	printf("Parse returned %d:\n", n);
	printf("   stdin : %s\n", cmd->rstdin  ? cmd->rstdin  : "<none>" );
	printf("   stdout: %s\n", cmd->rstdout ? cmd->rstdout : "<none>" );
	printf("   bg    : %s\n", cmd->bakground ? "yes" : "no");
	PrintPgm(cmd->pgm);
}

/*
 * Name: PrintPgm
 *
 * Description: Prints a list of Pgm:s
 *
 */
void PrintPgm (Pgm *p) {
	if (p == NULL) {
		return;
	}
	else {
		char **pl = p->pgmlist;

		/* The list is in reversed order so print
		* it reversed to get right	
		* */
		PrintPgm(p->next);
		printf("    [");
	while (*pl) {
		printf("%s ", *pl++);
		
	}
		printf("]\n");
	}
}

//int backround_process(Command c) {
//}

void init_command(Command *c) {

	execute_command(c);

}

void execute_pipes(Command *c) {
	int pipefd[2];
	int pid;
	int status;
	Pgm *p = c->pgm;
	
	pipe(pipefd);
	pid = fork();
	
	if (pid == 0) {
	
		close(pipefd[0]);
		dup2(pipefd[1], 1);
		p = p->next;
	
		if (p->next) {
			execute_pipes(c);
		}	
		else {
			execvp(*p->pgmlist, p->pgmlist);
			exit(0);
		}
	}
	else {
		close(pipefd[1]);
		dup2(pipefd[0], 0);
		waitpid(pid, &status, 0);
		execvp(*p->pgmlist, p->pgmlist);
	}
}

void signal_handler() {
	printf("\n");
}

void execute_command(Command *c) {
	int pid;	

	Pgm *p = c->pgm;	

	if (p == NULL) {
		return;
	}
		// Returns 0 if identical, thus the !
		if(!strcmp(*p->pgmlist, "exit")) {
			exit(0);
		}
		else if(!strcmp(*p->pgmlist, "cd")) {
			chdir(p->pgmlist[1]);
			return;
		}
		pid = fork();

		if (pid < 0) {
			printf("Error!\n");
		}
		else if (pid > 0 && c->bakground == 1) {
			signal(SIGCHLD, SIG_IGN);
		}
		else if (pid > 0) {
			waitpid(pid, 0, NULL);
		}
		else {
			if(c->rstdout) {
			FILE *f = freopen(c->rstdout, "w", stdout);
			}
			if(c->rstdin) {
				int fd = open(c->rstdin, 0);
				dup2(fd, 0);
			}
			
			if(p->next) {
				execute_pipes(c);
			}
			execvp(*p->pgmlist, p->pgmlist);	
			exit(0);
		}
}

/*
 * Name: stripwhite
 *
 * Description: Strip whitespace from the start and end of STRING.
 */
void stripwhite (char *string) {
	register int i = 0;

	while (isspace( string[i] )) {
		i++;
	}
  
	if (i) {
		strcpy (string, string + i);
	}

	i = strlen( string ) - 1;
	while (i> 0 && isspace (string[i])) {
		i--;
	}

	string [++i] = '\0';
}
