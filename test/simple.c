#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

static const char name[] = "testfile";

int main(void) {
	printf("Name address: 0x%x (%d)\n", name, name);

	int fd = open(name, O_RDWR | O_CREAT | O_TRUNC);
	if(fd < 0) {
		return -1;
	}

	char buf[512] = {0};
	write(fd, buf, sizeof(buf));
	
	close(fd);
	return 0;
}

