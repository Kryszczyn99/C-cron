#include "cron.h"
#include "log.h"

//STANDARD LIBS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

//POSIX LIBS
#include <semaphore.h>
#include <mqueue.h>
#include <fcntl.h>

//SYS LIBS
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>



int transmit_a_message(struct info_t message) {

	char receiver_name[LENGTH_FOR_TABLE_MAX]; 
	sprintf(receiver_name, "%s_%d", MQ_CL_FILE, message.pid);

	// SERVER QUEUE CONNECTION
	mqd_t transmitter = mq_open(MQ_SR_FILE, O_WRONLY);
	if (transmitter == -1) {
		return 1;
	}
	// CLIENT QUEUE CREATING
	int value = 0;

	struct mq_attr attributes;
	attributes.mq_flags = 0;
	attributes.mq_maxmsg = MESSAGES_IN_MQ;
	attributes.mq_msgsize = sizeof(struct info_t);
	attributes.mq_curmsgs = 0;

	// OPENING RECEIVER FOR CLIENT
	mqd_t receiver = mq_open(receiver_name, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR, &attributes);
	if (receiver ==  -1) {
		mq_close(transmitter);
		return 2;	
	}
	// SENDING MESSAGE
	value = mq_send(transmitter, (char *)&message, sizeof(struct info_t), 1);
	if (value == -1) {
		// CLOSE ALL MEDIAS
		mq_close(transmitter);
		mq_close(receiver);
		mq_unlink(receiver_name);
		return 3;
	}
	// TRYING TO GET RESPONSE AND WAITING FOR THIS
	value = mq_receive(receiver, (char *)&message, sizeof(struct info_t), NULL);
	if (value == -1) {
		// CLOSE ALL MEDIAS
		mq_close(transmitter);
		mq_close(receiver);
		mq_unlink(receiver_name);
		return 4;
	}
	if (message.status == FAILED) {
		// CLOSE ALL MEDIAS
		mq_close(transmitter);
		mq_close(receiver);
		mq_unlink(receiver_name);
		return 5;
	}
	// CLOSE ALL MEDIAS
	mq_close(transmitter);
	mq_close(receiver);
	mq_unlink(receiver_name);
	return 0;
}
int display_all_current_tasks() {

	int value = 0;
	
	struct info_t message;
	message.status = SHOW_TASK_SIGNAL;
	message.pid = getpid();
	// CREATING NAMES
	char receiver_name[LENGTH_FOR_TABLE_MAX]; 
	sprintf(receiver_name, "%s_%d", MQ_CL_FILE, message.pid);
	// TRYING TO CONNECT TO SERVER
	mqd_t transmitter = mq_open(MQ_SR_FILE, O_WRONLY);
	if (transmitter ==  -1) {
		return 1;
	}
	// CREATING MQUEUE FOR THE CLIENT
	struct mq_attr attributes;
	attributes.mq_flags = 0;
	attributes.mq_maxmsg = MESSAGES_IN_MQ;
	attributes.mq_msgsize = sizeof(struct info_t);
	attributes.mq_curmsgs = 0;

	// OPEN RECEIVER FOR CLIENT
	mqd_t receiver = mq_open(receiver_name, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR, &attributes);
	if (receiver ==  -1) {
		mq_close(transmitter);
		return 2;	
	}
	// SENDING MESSAGE 
	value = mq_send(transmitter, (char *)&message, sizeof(struct info_t), 1);
	if (value == -1) {
		mq_close(transmitter);
		mq_close(receiver);
		mq_unlink(receiver_name);
		return 3;
	}
	int i = 0;
	while(1) {
		if (i == THE_LIMIT_OF_TASKS) break;
		value = mq_receive(receiver, (char *)&message, sizeof(struct info_t), NULL);		
		// GETTING RESPONSE WITH OUR TASKS
		if (value == -1) {
			mq_close(transmitter);
			mq_close(receiver);
			mq_unlink(receiver_name);
			return 4;
		}
		if (message.status == FAILED) {
			return 5;
		}
		if (message.status != SUCCESS) {
			printf("\n[%d]\n", message.pid);
			printf("%s %s\n", message.name_of_program, message.arguments);
			printf("%lf\n", message.sec_or_taskID);
			if(message.status == DISPOSABLE_TASK_SIGNAL) {
				printf("Program runs: 1 time\n");
			}
			else {
				printf("Program runs in repeating mode!\n");
			}
		} else {
			break;
		}
		i++;
	}
	if (i == 0) {
		printf("Cron hasn't got any tasks\n");
	} else {
		printf("\n");
	}
	mq_close(transmitter);
	mq_close(receiver);
	mq_unlink(receiver_name);
	return 0;
}

void main_thread(union sigval arg) {
	struct timer_message_t *task = (struct timer_message_t *)arg.sival_ptr;
	
	pid_t pid, sid;
	pid = fork();
	if (pid > 0) {
		
		log_string("[%s] [%s] executed.\n", task->message.name_of_program, task->message.arguments);
		// IF SIGNAL IS ONLY FOR 1 USAGE THEN CLEAR IT
		if (task->message.status == DISPOSABLE_TASK_SIGNAL) {
			task->message.status = EXIT_SIGNAL;
		}
		return;
	}
	if (pid < 0) {
		return;
	}
	umask(0);
	sid = setsid();
	if (sid < 0) {
		return;
	}

	//DIVIDE ARGS INTO STH USEFUL
	char* arguments[LENGTH_FOR_TABLE_MAX] = {task->message.name_of_program, NULL};
	int i = 1;
	char *string;
	string = strtok(task->message.arguments, " ");
	while(1) {
		if (string == NULL) break;
		arguments[i] = string;
		string = strtok(NULL, " ");
		i++;
	}
	arguments[i] = NULL;

	//EXECUTE PROGRAM
	execvp(task->message.name_of_program, arguments);

	
}


int main_loop_for_cron_server() {
	// TABLE FOR TASKS
	struct timer_message_t tasks[THE_LIMIT_OF_TASKS] = { 0 };
	// STARTING LOGGER FOR LOGS
	if (log_start("logs.txt", tasks, sizeof(struct timer_message_t) * THE_LIMIT_OF_TASKS) != 0) {
		return 1;
	}
	// SET TIER TO MAXIMUM
	log_set_tier(3);

	struct mq_attr attributes;
	attributes.mq_flags = 0;
	attributes.mq_maxmsg = MESSAGES_IN_MQ;
	attributes.mq_msgsize = sizeof(struct info_t);
	attributes.mq_curmsgs = 0;
	// CREATING MQUEUE FOR SERVER
	mqd_t receiver = mq_open(MQ_SR_FILE, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR, &attributes);
	if (receiver == -1) {
		log_string("Creating mqueue for server, failed!");
		log_stop();
		return 2;
	}
	log_string("Cron is ready!");
	// CREATING MESSAGE
	struct info_t first_msg = { 0 };
	static int unique_id = 0;
	while (1) {
		// WAITING 
		if (mq_receive(receiver, (char *)&first_msg, sizeof(struct info_t), NULL) != -1) {
			char tx_name[LENGTH_FOR_TABLE_MAX];
			sprintf(tx_name, "%s_%d", MQ_CL_FILE, first_msg.pid);
			// OPENNING MQUEUE FOR THE CLIENT
			mqd_t transmitter = mq_open(tx_name, O_WRONLY);
			if (transmitter == -1) {
				log_string("Creating mqueue for client, failed.");
				log_stop();
				mq_close(receiver);
				mq_unlink(MQ_SR_FILE);
				return 3;
			}
			// PREPARING 2nd MESSAGE
			struct info_t second_msg = { 0 };
			second_msg.status = SUCCESS;
	
			if (first_msg.status == REPEATING_TASK_SIGNAL || first_msg.status == DISPOSABLE_TASK_SIGNAL) {
				int i = 0;
				while(i < THE_LIMIT_OF_TASKS) {

					if (tasks[i].timer == NULL) break;
					++i;
				}
				if (i < THE_LIMIT_OF_TASKS) {
					struct itimerspec ts = { 0 };
					ts.it_value.tv_sec = first_msg.sec_or_taskID;
					if (first_msg.status == REPEATING_TASK_SIGNAL) {	
						ts.it_interval.tv_sec = first_msg.sec_or_taskID;
					}
					struct sigevent se;
					se.sigev_notify = SIGEV_THREAD;
					se.sigev_value.sival_ptr = &tasks[i];
					se.sigev_notify_function = main_thread;
					se.sigev_notify_attributes = NULL;
					// CREATE TIMER
					int status = timer_create(CLOCK_REALTIME, &se, &tasks[i].timer);
					if (status == -1) {
						log_string("Creating timer, failed.");
						second_msg.status = FAILED;
					}
					status = timer_settime(tasks[i].timer, 0, &ts, 0);
					if (status == -1) {
						log_string("Setting timer, failed.");
						second_msg.status = FAILED;
					}
					tasks[i].message = first_msg;
					tasks[i].message.pid = unique_id++;
					log_string("[%d][%s][%s] in %.0lf seconds.", first_msg.pid, first_msg.name_of_program, first_msg.arguments, first_msg.sec_or_taskID);
				}
				else {
					log_string("Queue is full, can't outsource more tasks.");
					second_msg.status = FAILED;
				}
				mq_send(transmitter, (char *)&second_msg, sizeof(struct info_t), 1);
				mq_close(transmitter);
			}
			else if (first_msg.status == EXIT_SIGNAL) {
				mq_send(transmitter, (char *)&second_msg, sizeof(struct info_t), 1);
				mq_close(transmitter);
				log_string("Cron has stopped working!");
				mq_close(receiver);
				mq_unlink(MQ_SR_FILE);
				log_stop();
				return 0;
			}
			else if (first_msg.status == SHOW_TASK_SIGNAL) {
				for (int i = 0; i < THE_LIMIT_OF_TASKS; ++i) {
					if (tasks[i].timer != NULL) {
						mq_send(transmitter, (char *)&tasks[i].message, sizeof(struct info_t), 1);
					}
				}
				log_string("[%d] got list of tasks.", first_msg.pid);
				mq_send(transmitter, (char *)&second_msg, sizeof(struct info_t), 1);
				mq_close(transmitter);
			}
			else if (first_msg.status == CANCELING_TASK_SIGNAL) {
				for (int i = 0; i < THE_LIMIT_OF_TASKS; ++i) {
					if (tasks[i].timer != NULL && tasks[i].message.pid == first_msg.sec_or_taskID) {
						struct itimerspec ts = { 0 };
						int status = timer_settime(tasks[i].timer, 0, &ts, 0);
						if (status == -1) {
							log_string("Stopping timer, failed!");
							second_msg.status = FAILED;
						}
						log_string("[%d] has stopped a task: %.0lf", first_msg.pid, first_msg.sec_or_taskID);
						tasks[i].timer = NULL;
					}
				}
				mq_send(transmitter, (char *)&second_msg, sizeof(struct info_t), 1);
				mq_close(transmitter);
			}
		}
	}
	return 0;
}

// CHECKING IF FILE IN /dev/mqueue already exist
int get_cron_current_status() {
	mqd_t value = mq_open(MQ_SR_FILE, O_RDONLY);
	if (value !=  -1) {
		mq_close(value);
		return 1;
	}
	return 0;
}

int start_cron_function() {

	// IF TRUE THEN CRON IS CURRENTLY RUNNING
	if (get_cron_current_status() == 1) {
		return 1;
	}
	// TURNING ON CRON
	if (fork_cron_function() != 0) {
		return 2;
	}
	return 0;
}

int fork_cron_function() {

	pid_t pid, sid;
	// MAIN AND OTHER THREAD
	pid = fork();
	// IF FORK GIVES LESS THAN 0 then error
	if (pid < 0) {
		return 1;
	}
	// GOT PID FOR NEW THREAD
	if (pid > 0) {
		
		while (get_cron_current_status() == 0) {
			//WAITING FOR CRON TO BE STARTED
			usleep(50);
		}
		printf("Current cron pid: %d\n", pid);
		return 0;
	}
	// PERMISSIONS FOR FILES
	umask(0);

	// TERMINAL CONNECTION
	sid = setsid();
	if (sid < 0) {
		return 1;
	}

	// CLOSING ALL CONNECTIONS
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	
	return main_loop_for_cron_server();
}