/**
 * ftrace.h
 *
 * Common ftrace definitions.
 */

#ifndef FTRACE_FTRACE_H
#define FTRACE_FTRACE_H


#ifdef FTRACE_DEBUG
# include <assert.h>
# include <stdio.h>
# define FTRACE_ASSERT(pred) 		assert((pred))
# define FTRACE_LOG(format, args...) 	fprintf(stderr, format, ## args)
#else
# define FTRACE_ASSERT(pred)		
# define FTRACE_LOG(format, args...)	
#endif


// Tracer result codes.
typedef enum {
	FTRACE_SUCCESS = 0,
	FTRACE_NO_MEMORY,
	FTRACE_INVALID_PROCESS,
} ftrace_result_t;


#endif

