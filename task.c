
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "task.h"

ftrace_result_t start(struct tracer_context* ctx);
void trace_task(struct ftrace_task* task);

// Set ptrace options for new processes.
static void setup_ptrace_options(int pid)
{
	ptrace(PTRACE_SETOPTIONS, pid, PTRACE_O_TRACEFORK | PTRACE_O_TRACECLONE); 
}


// Add new task entry into list.
// Return 0 on success, -1 if memory alloc failed.
static int add_task(struct tracer_context* ctx, int pid) 
{
	struct ftrace_task* task = malloc(sizeof(struct ftrace_task));
	if(!task) {
		return -1;
	}

	memset(task, 0, sizeof(*task));
	task->tid = pid;

	task->next = ctx->tasks;
	ctx->tasks = task;

	return 0;	
}	


static struct ftrace_task* find_task(struct tracer_context* ctx, int pid)
{
	struct ftrace_task* task = ctx->tasks;

	while(task) {
		if(task->tid == pid) {
			return task;
		}

		task = task->next;
	}

	return NULL;
}


static void remove_task(struct tracer_context* ctx, struct ftrace_task* task)
{
	if(task == ctx->tasks) {
		ctx->tasks = task->next;
	} else {
		struct ftrace_task* prev = ctx->tasks;
		while(prev && (prev->next != task)) {
			prev = prev->next;
		}

		FTRACE_ASSERT(prev && (prev->next == task));
		prev->next = task->next;
	}

	free(task);
}


ftrace_result_t ftrace_create(const char* name, char** argv)
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
		return kFTrace_InvalidProcess;
	}

	// Init trace context and add new task.
	struct tracer_context ctx;
	memset(&ctx, 0, sizeof(&ctx));
	
	if(0 != add_task(&ctx, pid)) {
		return kFTrace_NoMemory;
	}

	setup_ptrace_options(pid);
	ptrace(PTRACE_SYSCALL, pid, NULL, NULL);

	FTRACE_LOG("Starting trace for pid %d\n", pid);

	return start(&ctx);
}


ftrace_result_t ftrace_attach(int pid)
{
	return kFTrace_InvalidProcess;
}


/********************************************************************
 *
 * 	Tracing.
 *
 ********************************************************************/


ftrace_result_t start(struct tracer_context* ctx) 
{
	int status;

	// wait on all child processes.
	while(1) {
		int pid = waitpid(-1, &status, WUNTRACED);
		if(pid < 0) {
			FTRACE_LOG("waitpid failed with %d:%s\n", errno, strerror(errno));
			break;
		}

		int signo = 0;

		ctx->current = find_task(ctx, pid);
		FTRACE_ASSERT(ctx->current != NULL);

		if(WIFEXITED(status) || WIFSIGNALED(status)) {

			// task exited, need to remove it from trace list.
			FTRACE_LOG("task %d terminated\n", pid);
			remove_task(ctx, ctx->current);
		
		} else if(WIFSTOPPED(status)) {
		
			// task is stopped.
			// if signal is SIGSTOP - thats for us.
			signo = WSTOPSIG(status);
			FTRACE_LOG("Task %d stopped by signal %d\n", pid, signo);
			if(signo == SIGSTOP) {
				trace_task(ctx->current);
			}

		} else {
			FTRACE_LOG("Unknown task %d status\n", pid);
		}

		ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
	}


	// stop tracing.
	return kFTrace_Success;
}


void trace_task(struct ftrace_task* task)
{
	
}


