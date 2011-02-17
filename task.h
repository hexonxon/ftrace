/**
 * task.h - syscall tracer interface.
 *
 * Syscall tracer is initialized by creating and running a new process (ftrace_create) or attaching to the existing (ftrace_attach).
 */

#ifndef FTRACE_TASK_H
#define FTRACE_TASK_H


#ifdef FTRACE_DEBUG
# include <assert.h>
# include <stdio.h>
# define FTRACE_ASSERT(pred) 		assert((pred))
# define FTRACE_LOG(format, args...) 	fprintf(stderr, format, ## args)
#else
# define FTRACE_ASSERT(pred)		
# define FTRACE_LOG(format, args...)	
#endif


/// Tracer result codes.
typedef enum {
	kFTrace_Success = 0,
	kFTrace_NoMemory,
	kFTrace_InvalidProcess,
} ftrace_result_t;


/**
 * Trace task.
 * Single execution thread to trace syscall entering/exiting. 
 * Tracer will keep track of all threads in traced process and its children one task for each thread.
 */
struct ftrace_task {
	int			tid;		// target thread id.
	int			entered;	

	struct ftrace_task*	next;		// next task.
};


/**
 * Tracer context.
 * Keeps track of tasks to trace and their status.
 */
struct tracer_context {
	struct ftrace_task*	tasks;		// task list.
	struct ftrace_task*	current;	// current stopped task.
};


/**
 * Run new task and trace it.
 * 
 * @name	Executable image path.
 * @argv	argv value to pass to executable.
 * 
 * @return	kFTraceInvalidProcess if executable path is invalid.
 */
ftrace_result_t ftrace_create(const char* name, char** argv);


/**
 * Attach to running process and trace it.
 *
 * @pid		Process id to attach to.
 *
 * @return	kFTraceInvalidProcess if pid is invalid.
 */
ftrace_result_t ftrace_attach(int pid);


#endif

