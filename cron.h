#ifndef _CRON_H_
#define _CRON_H_


//STANDARD LIBS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

//POSIX LIBS
#include <mqueue.h>
#include <semaphore.h>
#include <fcntl.h>

//SYS LIBS
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>


#define THE_LIMIT_OF_TASKS 40
#define LENGTH_FOR_TABLE_MAX 512
#define MESSAGES_IN_MQ 4

#define MQ_CL_FILE "/cron_client"
#define MQ_SR_FILE "/cron_server"


enum action_enum_t {
	EXIT_SIGNAL = 0,
	DISPOSABLE_TASK_SIGNAL = 1,
	REPEATING_TASK_SIGNAL = 2,
	SHOW_TASK_SIGNAL = 3,
	CANCELING_TASK_SIGNAL = 4,
	FAILED = 5,
	SUCCESS = 6
};

struct info_t {
	int pid;
	double sec_or_taskID;
	enum action_enum_t status;
	char name_of_program[LENGTH_FOR_TABLE_MAX];
	char arguments[LENGTH_FOR_TABLE_MAX];
};

struct timer_message_t {
	struct info_t message;
	timer_t timer;
};


int transmit_a_message(struct info_t message);
int main_loop_for_cron_server();
int display_all_current_tasks();
int get_cron_current_status();
int start_cron_function();
int fork_cron_function();
#endif