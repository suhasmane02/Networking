#ifndef TEST_H
#define TEST_H

#define MY_MAGIC        'G'
#define READ_IOCTL      _IOR(MY_MAGIC, 0, int)
#define WRITE_IOCTL     _IOR(MY_MAGIC, 1, int)

#define ECHO_REQ        1
#define ECHO_REPLY      2

#define TEST_DEV        "/dev/testdev"
#define ETH_P_TEST	0xCAFE

typedef struct test_struct {
        int count;
        int type;
        char data[100];
} str_test;


#endif
