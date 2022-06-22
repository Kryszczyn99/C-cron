#include "cron.h"
#include <stdio.h>
#include <string.h>


//COMPILE gcc cron.c main.c log.c -o cron -lpthread -lrt

int main(int argc, char const *argv[]) {

	// VALIDATION OF ARGV

	// ./cron -p or higher (example)
	if (argc < 2) {	
		printf("Cron needs more arguments! You can see more informations on -h command.\n");
		return 1;
	}
	// SECOND ARG SHOULD BE 2 LENGHTH
	if (strlen(argv[1]) != 2) {
		printf("First argument should have length = 2! You can see more informations on -h command.\n");
		return 1;
	}
	//THE FIRST SIGN FOR ARG IS -
	if (argv[1][0] != '-') {
		printf("Wrong argument for cron! You can see more informations on -h command.\n");
		return 1;
	}
	// ./cron -p -r 100 progr args args
	// PROGRAMMABLE TASK
	if(argv[1][1] == 'p') {
		struct info_t msg = { 0 };
		msg.pid = getpid();
		// STD CHECK		
		if (argc < 5) {
			printf("Not enough arguments for this command!\n");
			return 1;
		}
		// RELATIVE CHECK
		if (strlen(argv[2]) == 2 && argv[2][1] == 'r') {
			msg.status = REPEATING_TASK_SIGNAL;
			strcpy(msg.name_of_program, argv[4]);
			// PARSE ARGUMENTS
			for (int i = 5; i < argc; ++i)
			{
				strcat(msg.arguments, argv[i]);
				strcat(msg.arguments, " ");
			}
			// TIME CHECK
			sscanf(argv[3], "%lf", &msg.sec_or_taskID);
		} else {
			msg.status =  DISPOSABLE_TASK_SIGNAL;
			strcpy(msg.name_of_program, argv[3]);
			// PARSE ARGUMENTS
			for (int i = 4; i < argc; ++i)
			{
				strcat(msg.arguments, argv[i]);
				strcat(msg.arguments, " ");
			}
			// TIME CHECK
			sscanf(argv[2], "%lf", &msg.sec_or_taskID);
		}

		int value = transmit_a_message(msg);
		switch(value)
		{
			case 1:
				printf("Cron isn't started!\n");
				break;
			case 0:
				printf("Task has been added to list.\n");
				break;
			default:
				printf("Something is wrong.\n");
		}
	}
	// HELP
	else if(argv[1][1] == 'h') {
		printf("\n");
		printf("Options for CRON-scheduler\n");
		printf("-t  ----> Turn on\n");
		printf("-e  ----> Exit\n");
		printf("-h  ----> Help command\n");
		printf("-d  ----> Display all current planned tasks\n");
		printf("-c <task ID from -g command> ----> Cancel task with specified ID\n");
		printf("-p <-r> <sec> <name_of_program> <arguments> ----> Schedule a task, <-r> for repeating(optional)\n");
		printf("Example: ./cron -p -r 400 ls -la\n");
		printf("\n");
	}
	// DISPLAY TASKS
	else if(argv[1][1] == 'd') {
		int value = display_all_current_tasks();
		if (value == 1) {
			printf("Cron isn't started!\n");
		}
	}
	// TURN ON/START
	else if(argv[1][1] == 't') {
		int value = start_cron_function();
		switch(value) {
			case 1:
				printf("Cron is started already.\n");
				break;
			case 0:
				printf("Cron started.\n");
				break;
			default:
				printf("Cron error. Err: %d\n", value);
		}
	}
	// CANCEL TASK
	else if(argv[1][1] == 'c') {

		struct info_t msg = { 0 };	
		msg.pid = getpid();
		int task_id = -1;
		// ./cron -c <id>
		if (argc != 3) {
			printf("Need more arguments for -c command!\n");
		} 
		else {

			int value = sscanf(argv[2], "%d", &task_id);
			if (value != 1) {
				printf("Need integer argument for this command!\n");
			}
			else if (task_id < 0) {
				printf("Integer argument should be higher or equal 0!\n");
			}
			else {
				msg.status = CANCELING_TASK_SIGNAL;
				msg.sec_or_taskID = task_id;
				value = transmit_a_message(msg);
				switch(value) {
					case 1:
						printf("Cron isn't started.\n");
						break;
					case 0:
						printf("Cancelled a task.\n");
						break;
					default:
						printf("Something is wrong.\n");
				}
			}
		}
	}
	// EXIT CRON/STOP CRON
	else if(argv[1][1] == 'e') {
		struct info_t msg = { 0 };
		msg.pid = getpid();
		msg.status = EXIT_SIGNAL;
		int value = transmit_a_message(msg);
		switch(value) {
			case 1:
				printf("Cron isn't started.\n");
				break;
			case 0:
				printf("Stopped cron.\n");
				break;
			default:
				printf("Cron stopped error. Err: %d\n", value);
		}
	}
	else {
		printf("Invalid command!\n");
	}
	return 0;
}
