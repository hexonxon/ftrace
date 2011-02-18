/**
 * task.c
 *
 * Primary perpose of this file is to contain all architecture dependencies in accessing traced task memory and info.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/reg.h>

#include "task.h"
#include "ftrace.h"


ftrace_word_t task_peekword(struct ftrace_task* task, const void* uaddr)
{
	return ptrace(PTRACE_PEEKDATA, task->pid, uaddr, NULL);
}


ftrace_word_t task_peekwordoff(struct ftrace_task* task, const void* uaddr, unsigned offset)
{
	return ptrace(PTRACE_PEEKDATA, task->pid, uaddr + offset, NULL);
}


void task_peekmem(struct ftrace_task* task, const void* uaddr, void* out_buf, unsigned nbytes)
{
	unsigned nwords = nbytes / FTRACE_WORD_SIZE;
	unsigned residue = nbytes % FTRACE_WORD_SIZE;

	while(nwords--) {
		ftrace_word_t word = task_peekword(task, uaddr);
		memcpy(out_buf, &word, sizeof(word));

		out_buf += sizeof(word);
		uaddr += sizeof(word);
	}

	if(residue) {
		ftrace_word_t word = task_peekword(task, uaddr);
		memcpy(out_buf, &word, residue);
	}	
}


unsigned task_peekstr(struct ftrace_task* task, const void* uaddr, char* out_str, unsigned bufsiz)
{
	union {
		ftrace_word_t 	word;
		char		bytes[sizeof(ftrace_word_t)];
	} udata;

	int eos = 0;
	unsigned offset = 0;

	
	while(!eos && (offset < bufsiz) /* -1 byte for NULL-terminator */) {

		memset(&udata, 0, sizeof(udata));
		udata.word = task_peekwordoff(task, uaddr, offset);

		int i;
		for(i = 0; i < sizeof(ftrace_word_t); ++i) {

			//FTRACE_LOG("%c", udata.bytes[i]);
			*out_str++ = udata.bytes[i];
			if(udata.bytes[i] == '\0') {
				eos++;
				break;
			}

		}

		offset += sizeof(udata.bytes);
	}

	//out_str[offset] = '\0';
	
	return offset;
}


int task_syscall_num(struct ftrace_task* task)
{
	return ptrace(PTRACE_PEEKUSER, task->pid, ORIG_RAX * FTRACE_WORD_SIZE, NULL);
}


long int task_syscall_p1(struct ftrace_task* task)
{
	struct user_regs_struct regs;
	ptrace(PTRACE_GETREGS, task->pid, NULL, &regs);
	return regs.rdi;
//	return ptrace(PTRACE_PEEKUSER, task->pid, R9 * __WORDSIZE, NULL);
}


long int task_syscall_p2(struct ftrace_task* task)
{
	return ptrace(PTRACE_PEEKUSER, task->pid, RCX * FTRACE_WORD_SIZE, NULL);
}


long int task_syscall_p3(struct ftrace_task* task)
{
	return ptrace(PTRACE_PEEKUSER, task->pid, RDX * FTRACE_WORD_SIZE, NULL);
}


long int task_syscall_retval(struct ftrace_task* task)
{
	return ptrace(PTRACE_PEEKUSER, task->pid, RAX * FTRACE_WORD_SIZE, NULL);
}



