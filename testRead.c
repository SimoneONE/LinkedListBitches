#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include "linked_list.h"
#include <stdlib.h>


/** 
 * Set a file descriptor to blocking or non-blocking mode
 * using fcntl library function.
 *
 * @param fd The file descriptor
 * @param blocking 0:non-blocking mode, 1:blocking mode
 *
 * @return 1:success, 0:failure.
 **/

int main( int argc, char *argv[] )
{
    int numBytes = 0;
    
    int filedesc = open("/dev/linked_list0", O_RDWR);
    
    if( argc == 3 ) {
        
        numBytes = atoi(argv[1]);
        
        if(strcmp("p",argv[2]) == 0){
            if( ioctl(filedesc, LL_SET_PACKET_MODE, 0) == 0)
                printf("Set PACKET mode\n");
        }
        else{
            if( ioctl(filedesc, LL_SET_STREAM_MODE, 0) == 0)
                printf("Set STREAM mode\n");
        }
    }
    else {
        printf("Usage: testRead n m, where:\n");
        printf("- n is the number of bytes you want to read.\n");
        printf("- m is the read mode: type 'p' for packet and 's' for string.\n");
        printf("Exiting.\n");
        return 0;
    }

    if (filedesc < 0) {
		printf("There was an error opening linked_list0\n");
        return -1;
    }
 
	char data[numBytes+1];
	//data[numBytes]='\0';
    int res2;
	res2 = read(filedesc, data, numBytes);
	if( res2 < 0 )
		printf("An error occurred in the read.");
	
	//così si dovrebbe risolvere il problema dei caratteri strani
	data[res2]='\0';
	
	printf("Read data:\n%s\n", data);
	//printf("%c\n", data[numBytes]);
	close(filedesc);
 
    return 0;
}
