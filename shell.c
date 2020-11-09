/*
    
ls | cat -n | cat -n | cat -n | cat -n | cat -n | cat -n

tty

ls -l /proc/self/fd | cat | cat | cat | cat

ps axf | grep tty1

sleep 1 | sleep 1 | sleep 1 | sleep 1 | sleep 1 | sleep 1 | sleep 1 | sleep 1 | sleep 1 | sleep 1

*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdbool.h>

#define TOKEN_DELIMITERS = " \t"

bool SHELL_CLOSED = false;

void shell_loop();
void exec_comarg(char* input);
void call_command(char* commandName, char** args, int argc);
bool try_shell_command(char* commandName, char** args, int argc);
void split_into_comargs(char* input, char** outputArr[], int* argc);

void cmd_help();
void cmd_cd(char** args);

int main(int argc, char** argv)
{
	printf("This is a tiny shell. call 'help' for more info.\n");
	printf("> ");

	shell_loop();

	return 0;
}

// Main shell loop function
void shell_loop()
{
	char* str = NULL;
	size_t bufferLen = 0;
	size_t nread;

	while((nread = getline(&str, &bufferLen, stdin)) != -1 && SHELL_CLOSED == false)
	{
		char** output = NULL;
		int outputSize = 0;

		if(strcmp(str, "\n") == 0 || strcmp(str, "\r\n") == 0 || strcmp(str, "") == 0 
		||strcmp(str, " ") == 0)
		{
			printf("> ");
			continue;
		}

		// PREPARING PIPES
		// saving current stdin and stdout
		int in = dup(0); // stdin 
		int out = dup(1); // stdout

		int fdin;
		int fdout;
		
		// parse the line into an array of comargs (commands + arguments)
		split_into_comargs(str, &output, &outputSize);

		for(int i = 0; i < outputSize; i++)
		{
			dup2(fdin, 0);
			close(fdin);

			if(i == outputSize-1)
			{
				// Last command
				fdout=dup(out);
			}
			else
			{
				// Not last command
				int fdpipe[2];
				pipe(fdpipe);
				fdout = fdpipe[1];
				fdin = fdpipe[0];
			}

			dup2(fdout, 1);
			close(fdout);

			exec_comarg(output[i]);
		}		

		// close all pipes, free the memory
		// restore stdin and stdout
		dup2(in, 0);
		dup2(out, 1);
		close(in);
		close(out);
		for(int i = 0; i < outputSize; i++)
		{
			free(output[i]);
		}
		free(output);
		if(SHELL_CLOSED == true)
		{
			break;
		}
		printf("> ");
	}

	free(str);
}

// A function that generates an array of comargs (command + args) strings, that can be passed to exec_comarg() function
// function expects a pointer to the array of strings, and puts the result into this array
// outputSize is set to the size of output array
void split_into_comargs(char* input, char** outputArr[], int* outputSize)
{
	char* token;
	char** resultArr = NULL;
	token = strtok(input, "|");
	while(token != NULL)
	{
		if((*outputSize) == 0)
		{
			resultArr = malloc(sizeof(char*));
		}
		else
		{
			char** newArr = realloc(resultArr, sizeof(char*) * (*outputSize+1));
			if(newArr != NULL)
			{
				resultArr = newArr;
			}
			else
			{
				printf("An error while reallocating memory occurred. %s\n", strerror(errno));
				exit(1);
			}

			if(resultArr == NULL)
			{
				printf("An error while allocating memory occurred. %s\n", strerror(errno));
				exit(1);
			}
		}

		resultArr[(*outputSize)] = malloc(strlen(token)+1);
		token[strcspn(token, "\r\n")] = 0;
		strcpy(resultArr[(*outputSize)], token);	
		(*outputSize)++;
		token = strtok(NULL, "|");	
	}
	*outputArr = resultArr;
}

// A function that splits given command + args string into tokens, and passes tokenized result into call_command()
void exec_comarg(char* input)
{
	// INPUT PROCESSING
	char* token;
	token = strtok(input, " ");
	int argc = 0;
	char** args = NULL;

	//Tokenize input, get array of strings (args)
	while(token != NULL)
	{
		if(argc == 0)
		{
			args = malloc(sizeof(char*));
		}
		else
		{
			char** newArgs = realloc(args, sizeof(char*)*(argc+1));
			if(newArgs != NULL)
			{
				args = newArgs;
			}
			else
			{
				printf("An error while reallocating memory occurred. %s\n", strerror(errno));
				exit(1);
			}
		}

		if(args == NULL)
		{
			printf("An error while allocating memory occurred. %s\n", strerror(errno));
			exit(1);
		}
		
		// strlen ignores null terminator, so adding + 1 to malloc
		args[argc] = malloc(strlen(token) + 1); 
		token[strcspn(token, "\r\n")] = 0;
		strncpy(args[argc], token, strlen(token)+1);
		//strcpy(args[argc], token);
		
		argc++;
		token = strtok(NULL, " ");
	}

	
	///

	//execvp expects last arg in args to be NULL, make sure we have it..

	char** newArgs = realloc(args, sizeof(char*)*(argc+1));
	if(newArgs != NULL)
	{
		args = newArgs;
	}
	else
	{
		printf("An error while reallocating memory occurred. %s\n", strerror(errno));
		exit(1);
	}
	args[argc] = NULL;
	argc++;
	//CALLING THE COMMAND
	call_command(args[0], args, argc);

	for(int i = 0; i < argc; i++)
	{
		free(args[i]);
	}
	free(args);
}

// A function that executes passed command with args
void call_command(char* commandName, char** args, int argc)
{
	// Check if it is a builtin command
	if(try_shell_command(commandName, args, argc) == true)
	{
		return;
	}

	int commandStatus = 0;
	pid_t pid;
	pid = fork();

	if(pid < 0)
	{
		printf("Failed creating new process. %s\n", strerror(errno));
		return;
	}
	else if (pid == 0)
	{	
		if(execvp(commandName, args) == -1)
		{
			printf("Error executing program. %s\n", strerror(errno));
			return;
		}

		for(int i = 0; i < argc+1; i++)
		{
			if(args[i])
				free(args[i]);
		}
		if(args)
			free(args);
	}

	do
	{
		waitpid(pid, &commandStatus, WUNTRACED);
	} while (!WIFEXITED(commandStatus) && !WIFSIGNALED(commandStatus));
}

// Function that checks if given command is shell builtin, and if it is, execute it, then return true
// Returns false if command is not builtin
bool try_shell_command(char* commandName, char** args, int argc)
{
	if(strcmp(commandName, "cd") == 0)
	{
		cmd_cd(args);
		return true;
	}
	else if(strcmp(commandName, "help") == 0)
	{
		cmd_help();
		return true;
	}
	else if (strcmp(commandName, "exit") == 0)
	{
		printf("Bye\n");
		SHELL_CLOSED = true;
		return true;
	}

	return false;
}

// Print help message and exit
void cmd_help()
{
	printf("This shell should accept usual linux shell commands. But there are built in commands too:\n");
	printf("help - print this message.\n");
	printf("cd [directory] - perform chdir command to change directory\n");
	printf("exit - closes the shell\n");
}

// Change directory function
void cmd_cd(char** args)
{
	if(args == NULL || args[1] == NULL)
	{
		printf("Could not change directory, given args are null");
		return;
	}
	
	if(chdir(args[1]) != 0)
	{
		printf("Error: %s\n", strerror(errno));
		return;
	}
}