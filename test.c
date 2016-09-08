#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include "linked_list.h"

/** 
 * Set a file descriptor to blocking or non-blocking mode
 * using fcntl library function.
 *
 * @param fd The file descriptor
 * @param blocking 0:non-blocking mode, 1:blocking mode
 *
 * @return 1:success, 0:failure.
 **/
int fd_set_blocking(int fd, int blocking) {
    /* Save the current flags */
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return 0;

    if (blocking)
        flags &= ~O_NONBLOCK;
    else
        flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags) != -1;
}
 
int main(void)
{
    int filedesc = open("/dev/ll0", O_RDWR);
 
    if (filedesc < 0) {
		printf("There was an error opening ll0\n");
        return -1;
    }
 
	printf("Testing write and stream, non-blocking read...\n");
	
	int wrote = 0;
    if ((wrote = write(filedesc, "This will be output to ll0\n", 31)) != 31) {
        printf("There was an error writing to ll0; wrote: %d\n", wrote);
        return -1;
    }
    
    char data[128];
 
    /*if(read(filedesc, data, 128) < 0)
		printf("An error occurred in the read.\n");
		
	printf("Read data:\n%s", data);*/

	/* TEST BY ROB */
	printf("Testing now blocking mode on read stream...\n");
	if( ioctl(filedesc, LL_SET_NONBLOCKING, 0) == 0)
		printf("Set NON BLOCKING mode\n");

	/*int res;
	res = read(filedesc, data, 128);
	if( res < 0 )
		printf("An error occurred in the read.");
	if( res == 0 )
		printf("res is equal to 0 => (probably) we called read in non blocking mode.");*/

	/* TEST BY SARA */
 
	printf("Testing now non blocking mode on read packet...\n");
	if( ioctl(filedesc, LL_SET_PACKET_MODE, 0) == 0)
		printf("Set PACKET mode\n");
		
	int res2;
	res2 = read(filedesc, data, 128);
	if( res2 < 0 )
		printf("An error occurred in the read.");
	if( res2 == 0 )
		printf("res is equal to 0 => (probably) we called read in non blocking mode.");
	
	printf("Read data:\n%s\n", data);
	
	printf("Testing now non blocking mode on read stream...\n");
	if( ioctl(filedesc, LL_SET_STREAM_MODE, 0) == 0)
		printf("Set STREAM mode\n");
		
		
	int res3;
	res3 = read(filedesc, data, 3);
	if( res3 < 0 )
		printf("An error occurred in the read.");
	if( res3 == 0 )
		printf("res is equal to 0 => (probably) we called read in non blocking mode.");
	
	printf("Read data:\n%s\n", data);
	
	printf("Testing now non blocking mode on read packet...\n");
	if( ioctl(filedesc, LL_SET_PACKET_MODE, 0) == 0)
		printf("Set PACKET mode\n");
		
	int res4;
	res4 = read(filedesc, data, 128);
	if( res4 < 0 )
		printf("An error occurred in the read.");
	if( res4 == 0 )
		printf("res is equal to 0 => (probably) we called read in non blocking mode.");
	
	printf("Read data:\n%s\n", data);
	
	int wrote2 = 0;
    if ((wrote2 = write(filedesc, "and it does ok", 14)) != 14) {
        printf("There was an error writing to ll0; wrote: %d\n", wrote2);
        return -1;
    }
    
    printf("Testing now non blocking mode on read stream...\n");
	if( ioctl(filedesc, LL_SET_STREAM_MODE, 0) == 0)
		printf("Set STREAM mode\n");
		
    int res5;
	res5 = read(filedesc, data, 42);
	if( res5 < 0 )
		printf("An error occurred in the read.");
	if( res5 == 0 )
		printf("res is equal to 0 => (probably) we called read in non blocking mode.");
	
	printf("Read data:\n%s\n", data);
	
	/* TEST BY FABIO
	printf("Testing now blocking mode on read stream...\n");
	if (fd_set_blocking(filedesc, 0))
		printf("Set NON BLOCKING mode\n");
	if(read(filedesc, data, 128) < 0)
		printf("An error occurred in the read.\n");
	*/
	
	close(filedesc);
 
    return 0;
}
