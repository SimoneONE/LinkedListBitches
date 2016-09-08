#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

//MODIFICA SARA
#include "linked_list.h"

void set_read_mode(int fd , int argv)
{
	int res;
	if(argv==LL_PACKET_MODE){
		if(ioctl(fd, LL_SET_PACKET_MODE, &res) == -1)
		{
			perror("ll ioctl set read mode\n");
		}
		else
		{
			printf("Read mode set to: %d\n", res);
		}
	}
	else if (argv==LL_STREAM_MODE){
		if(ioctl(fd, LL_SET_STREAM_MODE, &res) == -1)
		{
			perror("ll ioctl set read mode\n");
		}
		else
		{
			printf("Read mode set to: %d\n", res);
		}
	}
	
	else {
		printf("Invalid argument");
	}
}

void set_blocking_flag(int fd)
{
	int res;
	if (ioctl(fd, LL_SET_BLOCKING, &res) == -1)
	{
		perror("ll ioctl set blocking flag\n");
	}
	else
	{
		printf("Blocking flag setted\n");
	}
}

void set_buff_size(int fd, int argv){
	if (ioctl(fd, SET_BUFF_SIZE, &argv) == -1){
		perror("set buffer size \n");
	}
	else{
		printf("Trying to change buffer size\n");
	}
}

void reset_blocking_flag(int fd)
{
	int res;
	if (ioctl(fd, LL_SET_NONBLOCKING, &res) == -1)
	{
		perror("ll ioctl reset blocking flag\n");
	}
	else
	{
		printf("Blocking flag resetted\n");
	}
}

int main(int argc, char const *argv[])
{
	if (argc < 4)
		goto help;

	const char * minor = argv[1];
	int mode_stream = 0;
	int mode_packet = 0;
	int blocking = 0;
	int not_blocking = 0;
	
	int buff_size = atoi(argv[2]);

	int n = 3;

	while ( n < argc )
	{
		if ( !strncmp(argv[n], "-s", 2) || !strncmp(argv[n], "-S", 2) )
		{
			mode_stream = 1;
		}

		if ( !strncmp(argv[n], "-p", 2) || !strncmp(argv[n], "-P", 2) )
		{
			mode_packet = 1;
		}

		if ( !strncmp(argv[n], "-b", 2) || !strncmp(argv[n], "-B", 2) )
		{
			blocking = 1;
		}

		if ( !strncmp(argv[n], "-nb", 3) || !strncmp(argv[n], "-NB", 3) )
		{
			not_blocking = 1;
		}

		++n;
	}

	if (mode_stream && mode_packet)
		goto help;

	if (blocking && not_blocking)
		goto help;

	/*queste parentesi sono per compilare
	 * http://stackoverflow.com/questions/20654191/c-stack-memory-goto-and-jump-into-scope-of-identifier-with-variably-modified */ 
	{char *file_name = "/dev/ll";
	char new_file_name[strlen(file_name) + 1];
	int fd;

	strcpy(new_file_name, file_name);
	strcat(new_file_name, minor);

	fd = open(new_file_name, O_RDWR);

	if (fd == -1)
	{
		perror("ioctl open error\n");
		return 2;
	}
	

	if (mode_stream)
		set_read_mode(fd, LL_STREAM_MODE);

	if (mode_packet)
		set_read_mode(fd, LL_PACKET_MODE);

	if(blocking)
		set_blocking_flag(fd);

	if(not_blocking)
		reset_blocking_flag(fd);
		
	/* set buffer size */
	if(buff_size!=0 ){
		set_buff_size(fd, buff_size);
	}

	return 0;}

help:
	fprintf(stderr, "Usage: %s minor [-s | -p] [-b | -nb]\n", argv[0]);
	return 1;
}
