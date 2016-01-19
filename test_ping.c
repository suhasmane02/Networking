#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include "test.h"

/*
  Sample application which sends layer 2 echo request 
  and receives the same as echo reply.
  This send echo request from user space to kernel module using ioctl.
  And kernel module sends it to target machine of-course within LAN.
  pass command line parameters as:
  ./test 10 "testing layer 2 ping"
   10 is packet count
   and double quote string is echo string.
*/
int main(int argc, char *argv[])
{
	str_test test_data;
	void *p;
	int val = 0;
	int fd1 = -1;
	int fd2 = -1;
	char temp_buf[100] = {0};
	int count = 0;
	int len = 100;
	int i32Read = 0;

	/* number of echo requests to send */
	count = atoi(argv[1]);

	/* get the major number to create character special file */
	system("cat /proc/devices | grep test_dev | cut -d\" \" -f 1 > test.txt");

	fd1 = open("test.txt", O_RDONLY, S_IRUSR);
	if(fd1 == -1) {
		printf("test.txt failed to open\n");
		return -1;
	}

	i32Read = lseek(fd1, 0, SEEK_END);
	p = mmap(0, i32Read, PROT_READ, MAP_SHARED, fd1, 0);
	if(p == MAP_FAILED) {
		printf("mmap failed\n");
		return -1;
	}

	sscanf((char*)p, "%d", &val);

	printf("val = %d\n", val);
	sprintf(temp_buf, "mknod %s c %d 0", TEST_DEV, val);
	system(temp_buf);

	close(fd1);
	unlink("test.txt");

	/* open TEST_DEV */
	if((fd2 = open(TEST_DEV, O_RDWR)) < 0) {
		printf("open failed\n");
		return -1;
	}

	/* copy echo buffer to temporary buffer */
	memset(temp_buf, 0, 100);
	strncpy(temp_buf, argv[2], strlen(argv[2]));

	while(count--) {
		memset(&test_data, 0, sizeof(str_test));
		test_data.count = count;
		test_data.type = ECHO_REQ;
		strncpy(test_data.data, temp_buf, strlen(temp_buf));

		/* send echo request */
		if(ioctl(fd2, WRITE_IOCTL, &test_data) < 0)
			perror("write ioctl");
		printf("sending 100 bytes | count = %d\n", test_data.count);

		sleep(2);

		/* receive echo reply */
		memset(&test_data, 0, sizeof(str_test));
		if(ioctl(fd2, READ_IOCTL, &test_data) < 0)
			perror("read ioctl");
		printf("received 100 bytes | count = %d\n", test_data.count);
		printf("                      data = %s\n\n", test_data.data);
	}

	unlink(TEST_DEV);
	return 0;
}
