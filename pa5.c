#include <unistd.h>	//read(), write(), close()
#include <fcntl.h>		//open()
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
//use wirte() to print STDOUT  (not printf())

int ifd, iret;
char lower(char c);
void find(char* buf, int len);

int main(int argc, char** argv)
{
	char buf[512];

	ifd = open(argv[1], O_RDONLY);
	if (ifd < 0)
		exit(1);

	while (1)
	{
		int idx = 0;
		while (1)
		{
			char c;
			ssize_t ret = read(STDIN_FILENO, &c, 1);
			if (ret == 0)
			{
				exit(0);
			}
			if (ret < 0)
			{
				exit(1);
			}

			if (c != '\n')
				buf[idx++] = lower(c);
			else
				break;
		}
		buf[idx] = 0;

		// decide case:
	}

	iret = close(ifd);
	if (iret < 0)
		exit(1);
	
	exit(0);
}

char lower(char c)
{
	if (c >= 'A' && c <= 'Z')
	{
		return c + ('a' - 'A');
	}
	return c;
}

// modify find function !! 
void find(char* buf, int len)
{
	return;
}

// find single word locations
void case1_handler(char* buf, int len)
{

}

void case2_handler(char* buf, int len)
{

}

void case3_handler(char* buf, int len)
{

}

void case4_handler(char* buf, int len)
{

}

void knuth_morris_pratt(char* buf, int ilen, char* match, int mlen)
{

}