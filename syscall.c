/**
 * syscall.c
 */

ftrace_word_t syscall_getword(int pid, const void* addr)
{
	return ptrace(PTRACE_PEEKDATA, pid, addr, NULL);
}


ftrace_word_t syscall_getword2(int pid, const void* addr, unsigned offset)
{
	return ptrace(PTRACE_PEEKDATA, pid, addr + offset, NULL);
}


void syscall_getmem(const ftrace_word_t* uaddr, ftrace_word_t* out_buf, unsigned nbytes)
{
	unsigned nwords = nbytes / FTRACE_WORD_SIZE;
	unsigned residue = nbytes % FTRACE_WORD_SIZE;
	
	unsigned i;
	for(i = 0; i < nwords; ++i, ++uaddr) {
		*out_buf++ = syscall_getword(uaddr);
	}

	if(residue) {
		ftrace_word_t word = syscall_getword(uaddr);
		memcpy(uaddr, (char*)&word, residue);
	}		
}


unsigned syscall_getstr(const void* uaddr, char* out_buf, unsigned bufsize)
{
	union {
		ftrace_word_t 	word;
		char		bytes[FTRACE_WORD_SIZE];
	} udata;

	int eos = 0;
	unsigned nleft = bufsize;

	while(!eos && nleft) {
		udata.word = syscall_getword(uaddr);
		
		int i;
		for(i = FTRACE_WORD_SIZE; i >= 0; --i) {
			eos = (udata.bytes[i] == '\0') && break;
		}

		memcpy(out_buf, udata.bytes, sizeof(udata.bytes));
		out_buf += sizeof(udata.bytes);
		nleft -= sizeof(udata.bytes);
		++uaddr;
	}

	unsigned ntotal = bufsize - nleft;
	out_buf[ntotal] = '\0';
	
	return ntotal;
}
		

ftrace_word_t syscall_tag(int pid) 
{
	return ptrace(PTRACE_PEEKUSER, pid, FTRACE_WORD_SIZE * ORIG_EAX, NULL);
}


ftrace_word_t syscall_retval(int pid)
{

}


