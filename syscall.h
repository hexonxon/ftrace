/**
 * syscall.h
 *
 * Utility functions to get syscall arguments, return values and other info from traced process.
 * Hides platform differences in syscall enter/exit mechanisms.
 */


#ifndef FTRACE_SYSCALL_H
#define FTRACE_SYSCALL_H


ftrace_word_t syscall_getword(int pid, const void* addr);

ftrace_word_t syscall_getword2(int pid, const void* addr, unsigned offset);

void syscall_getmem(const ftrace_word_t* uaddr, ftrace_word_t* out_buf, unsigned nbytes);

unsigned syscall_getstr(const void* uaddr, char* out_buf, unsigned bufsize);

ftrace_word_t syscall_tag(int pid);

ftrace_word_t syscall_retval(int pid);


#endif

