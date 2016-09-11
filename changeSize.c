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
    
    if (filedesc < 0) {
	printf("There was an error opening linked_list0\n");
        return -1;
    }
    
    if(argc == 3) {
	if(!strncmp(argv[1],"-s",2)){
		numBytes = atoi(argv[2]);
       		if( ioctl(filedesc, LL_SET_MAX_SIZE , &numBytes) == 0){
                	printf("Set buffer size = %d.\n", numBytes);
            		if( ioctl(filedesc, LL_GET_MAX_SIZE	, &numBytes) == 0)
                		printf("New buffer size is %d.\n", numBytes);
      		}
	}
	else if(!strncmp(argv[1],"-m",2)){
		numBytes = atoi(argv[2]);
		if( ioctl(filedesc, LL_SET_PACK_MIN_SIZE , &numBytes) == 0){
            		printf("Set pack min size= %d.\n", numBytes);
            		if( ioctl(filedesc, LL_GET_PACK_MIN_SIZE , &numBytes) == 0)
                		printf("New pack min size is %d.\n", numBytes);
       		}
	}
	else if(!strncmp(argv[1],"-M",2)){
		numBytes = atoi(argv[2]);
		if( ioctl(filedesc, LL_SET_PACK_MAX_SIZE , &numBytes) == 0){
            		printf("Set pack max size = %d.\n", numBytes);
            		if( ioctl(filedesc, LL_GET_PACK_MAX_SIZE , &numBytes) == 0)
                		printf("New pack max size is %d.\n", numBytes);
       		}
	}
	else {
        	printf("Usage: %s [-s | -m | -M] , where:\n",argv[0]);
        	printf("- s is the of the buffer.\n");
        	printf("- min is minimum size of the packet .\n");
        	printf("- max is the maximum size of the packet.\n");
        	printf("Exiting.\n");
    	}
    }
    else {
        printf("Usage: %s [-s] [-m] [-M] , where:\n",argv[0]);
        printf("- s is the of the buffer.\n");
        printf("- min is minimum size of the packet .\n");
        printf("- max is the maximum size of the packet.\n");
        printf("Exiting.\n");
    }
    
    return 0;
}
