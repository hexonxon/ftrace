/**
 * ftrace.c
 *
 * Parses parameters and initializes tracing.
 *
 * We keep track of the tasks to trace in a simple linked list for each process' threads and its childrens' threads.
 * Syscall mechanism is syncronous for each execution thread (i.e. a single thread enter and exits the syscall syncroniously).
 * We rely on it to store valid syscall context for each task.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>

#include "task.h"
#include "ftrace.h"


/*********************************************************
 *
 * Task list management.
 * We are currently single threaded so no locking is required.
 *
 *********************************************************/

static struct ftrace_task* 	g_task_list;	// traced tasks in a linked list.
static struct ftrace_task*	g_current;	// currently traced task.


// Add new task entry into list.
static int add_task(int pid) 
{
	struct ftrace_task* task = malloc(sizeof(struct ftrace_task));
	if(!task) {
		return -1;
	}

	memset(task, 0, sizeof(*task));
	task->pid = pid;

	task->next = g_task_list;
	g_task_list = task;

	return 0;	
}	


// Find task by pid.
static struct ftrace_task* find_task(int pid)
{
	struct ftrace_task* task = g_task_list;

	while(task) {
		if(task->pid == pid) {
			return task;
		}

		task = task->next;
	}

	return NULL;
}


// Remove task from list
static void remove_task(struct ftrace_task* task)
{
	if(task == g_task_list) {
		g_task_list = task->next;
	} else {
		struct ftrace_task* prev = g_task_list;
		while(prev && (prev->next != task)) {
			prev = prev->next;
		}

		FTRACE_ASSERT(prev && (prev->next == task));
		prev->next = task->next;
	}

	free(task);
}



/*********************************************************
 *
 * Tracing.
 *
 *********************************************************/

static ftrace_result_t start(void);
static void trace_task(struct ftrace_task* task);


// Extract ptrace event from wait status code.
#define FTRACE_EVENT(status) ( (status) >> 16 )


// Set ptrace options for new processes.
static void setup_ptrace_options(int pid)
{
	ptrace(PTRACE_SETOPTIONS, pid, PTRACE_O_TRACEFORK | PTRACE_O_TRACECLONE); 
}


// Run new process and trace it.
static ftrace_result_t ftrace_create(const char* name, char** argv)
{
	int pid = vfork();
	if(pid < 0) {
		FTRACE_LOG("Error: vfork failed\n");
		return -1;
	}

	if(pid == 0) {
		
		/* Child. */

		long rc = ptrace(PTRACE_TRACEME, 0, NULL, NULL);
		if(rc < 0) {
			FTRACE_LOG("Error: ptrace failed\n");
			return -1;
		}

		execvp(name, argv);

		/* Should not be reachable if ok. */
		FTRACE_LOG("Error: exec failed\n");
		exit(0);
	}

	/* Parent. Wait for child to exec and start tracing. */
	
	int status = 0;
	waitpid(pid, &status, WUNTRACED);
	
	if(WIFEXITED(status)) {
		FTRACE_LOG("Error: child process failed to execute\n");
		return FTRACE_INVALID_PROCESS;
	}

	// Add first task.
	if(0 != add_task(pid)) {
		return FTRACE_NO_MEMORY;
	}

	setup_ptrace_options(pid);
	ptrace(PTRACE_SYSCALL, pid, NULL, NULL);

	FTRACE_LOG("Starting trace for pid %d\n", pid);

	return start();
}


// Attach to existing process and trace it.
static ftrace_result_t ftrace_attach(int pid)
{
	return FTRACE_INVALID_PROCESS;
}


// Start tracing of created or attached process.
static ftrace_result_t start(void) 
{
	int status;
	int pid;
	int signo;

	// wait on all child processes.
	while((pid = waitpid(-1, &status, 0)) != -1) {

		signo = 0;
		struct ftrace_task* task = find_task(pid);

		// Precondition: stopped task is known.
		FTRACE_ASSERT(task != NULL);

		// task exited, need to remove it from trace list.
		if(WIFEXITED(status) || WIFSIGNALED(status)) {

			FTRACE_LOG("task %d terminated\n", pid);
			remove_task(task);
		
		// task is stopped
		} else if(WIFSTOPPED(status)) {
		
			signo = WSTOPSIG(status);
		
			// SIGSTOP/SIGTRAP is our cue.
			if((signo == SIGTRAP) || (signo == SIGSTOP)) {

				// We stop process in case of cloning/forking and when it is entering/exiting syscalls.
				// To deffirentiate we use ptrace event code in process status. 
				switch(FTRACE_EVENT(status)) {
				case PTRACE_EVENT_FORK: 	/* fallthru */
				case PTRACE_EVENT_VFORK:	/* fallthru */
				case PTRACE_EVENT_CLONE: {
					// task cloned/forked - add another pid context.
					int new_pid;
					ptrace(PTRACE_GETEVENTMSG, task->pid, NULL, &new_pid);
					add_task(new_pid);
					FTRACE_LOG("task %d cloned, new pid is %d\n", task->pid, new_pid);
					break;
				}
				
				case 0: {
					// syscall trap
					trace_task(task);
				}

				default: break;
				};
			}

		} else {
			FTRACE_LOG("Unknown task %d status\n", pid);
		}

		ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
	}

	// stop tracing.
	return FTRACE_SUCCESS;
}


static const char* syscall_name(int syscall)
{
	switch(syscall) {
	case SYS_open: return "open";
	case SYS_close: return "close";
	case SYS_creat: return "creat";
	case SYS_rename: return "rename";
	case SYS_read: return "read";
	case SYS_pread64: return "pread";
	case SYS_write: return "write";
	case SYS_pwrite64: return "pwrite";
	default: return "unknown syscall";
	};
}


// Process stopped traced task.
static void trace_task(struct ftrace_task* task)
{
	int sysnum = task_syscall_num(task);

	if(task->in_syscall) {
		FTRACE_LOG("task %d is returning from syscall \"%s\" (%d)\n", task->pid, syscall_name(sysnum), sysnum);
		task->in_syscall = 0;
	} else {
		FTRACE_LOG("task %d is entering syscall \"%s\" (%d)\n", task->pid, syscall_name(sysnum), sysnum);
		if(sysnum == SYS_open) {
			char name[PATH_MAX] = {0};
			long int nameaddr = task_syscall_p1(task);
			unsigned namelen = task_peekstr(task, nameaddr, name, PATH_MAX);
			FTRACE_LOG("task %d opens file \"%s\"\n", task->pid, name);
		}

		task->in_syscall = 1;
	}
		
}



/*********************************************************
 *
 * 	Main.
 *
 *********************************************************/


static void usage() 
{
	printf(	"ftrace - Trace process file i/o:\n"
		"usage: ftrace command [args...]\n"
		" command [args...]	- run command with optional arguments.\n"
	);
}


int main(int argc, char** argv) 
{
	if(argc < 2) {
		usage();
		return EXIT_FAILURE;
	}

	const char* cmd = argv[1];
	char** args = &argv[1];

	return ftrace_create(cmd, args);
}


