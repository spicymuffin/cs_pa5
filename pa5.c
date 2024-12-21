#pragma GCC optimize ("O3")

#include <unistd.h> // read(), write(), close()
#include <fcntl.h>  // open()
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
// use write() to print STDOUT (not printf())

#define DEBUG 1

#define FILE_READ_CHUNK 4096
#define LINE_BUF_SIZE 512

typedef struct word_metadata
{
	int word_start_ptr;
	int word_len;
} word_metadata_t;

int ifd, iret;

// word parser
void word_parse(char* input_buf, int input_len, word_metadata_t* word_mds, int* word_cnt, char delim0, char delim1);

// char utils
char lower(char c);

// write
int write_int(char* dest, int num);
int write_str(char* dest, char* src, int len);
int write_char(char* dest, char c);

// print
void print_int(int num);
void print_str(char* str, int len);
void print_diagnostic(char* str, int val);

// exit
void check_exit(char* buf, int len);

int main(int argc, char** argv)
{
	char input_buf[512];
	int failure_func[512];
	word_metadata_t word_mds[256];

	char text_buf[FILE_READ_CHUNK];
	char line_buf[LINE_BUF_SIZE];

	ifd = open(argv[1], O_RDONLY);
	if (ifd < 0)
	{
		exit(1);
	}

	while (1)
	{
		int idx = 0;
		int case4_flag = 0;

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
			if (c == '*')
			{
				case4_flag = 1;
			}
			if (c != '\n')
			{
				input_buf[idx++] = lower(c);
			}
			else
			{
				break;
			}
		}
		input_buf[idx] = 0;

		// check exit condition
		check_exit(input_buf, idx);

		int word_cnt = 0;

		if (input_buf[0] == '"')
		{
			// case 3 - consecutive words
		}
		else if (case4_flag)
		{
			// case 4 - regex
		}
		else
		{
			if (word_cnt == 1)
			{
				// case 1 - single word
			}
			else
			{
				// case 2 - multiple words
			}
		}

		// reset file offset to beginning for next iteration
		lseek(ifd, 0, SEEK_SET);
	}

	iret = close(ifd);
	if (iret < 0)
	{
		exit(1);
	}

	exit(0);
}




char lower(char c)
{
	return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
}

char tab_to_space(char c)
{
	return c == '\t' ? ' ' : c;
}

void word_parse(char* input_buf, int input_len,
	word_metadata_t* word_mds, int* word_cnt,
	char delim0, char delim1)
{
	int word_start = 0;
	int word_counter = 0;

	for (int i = 0; i <= input_len; i++)
	{
		if (input_buf[i] == delim0 ||
			input_buf[i] == delim1 ||
			input_buf[i] == '\0')
		{
			if (i - word_start > 0)
			{
				word_mds[word_counter].word_start_ptr = word_start;
				word_mds[word_counter].word_len = i - word_start;
				word_counter++;
			}
			word_start = i + 1;
		}
	}

	*word_cnt = word_counter;
}

int write_int(char* dest, int num)
{
	char buf[20];
	char* p = buf + 20 - 1;
	*p = '\0';
	int n = num;
	int negative = n < 0;
	int length = 0;
	if (negative)
	{
		n = -n;
	}

	do
	{
		*--p = (n % 10) + '0';
		n /= 10;
		length++;
	} while (n > 0);

	if (negative)
	{
		*--p = '-';
	}

	for (int i = 0; i < length + (negative ? 1 : 0); i++)
	{
		*dest++ = *p++;
	}

	return length + (negative ? 1 : 0);
}

int write_str(char* dest, char* src, int len)
{
	for (int i = 0; i < len; i++)
	{
		*dest++ = *src++;
	}
	return len;
}

int write_char(char* dest, char c)
{
	*dest = c;
	return 1;
}

void print_int(int num)
{
	char buf[20];
	int len = write_int(buf, num);
	write(STDOUT_FILENO, buf, len);
}

void print_str(char* str, int len)
{
	write(STDOUT_FILENO, str, len);
}

void print_diagnostic(char* str, int val)
{
	for (int i = 0; i < 512; i++)
	{
		if (str[i] == 0)
		{
			break;
		}
		write(STDOUT_FILENO, str + i, 1);
	}
	print_str(": ", 2);
	print_int(val);
	print_str("\n", 1);
}

// if buf is PA5EXIT (case insensitive)
void check_exit(char* buf, int len)
{
	if (len < 7)
	{
		return;
	}

	if (
		lower(buf[0]) == 'p' &&
		lower(buf[1]) == 'a' &&
		lower(buf[2]) == '5' &&
		lower(buf[3]) == 'e' &&
		lower(buf[4]) == 'x' &&
		lower(buf[5]) == 'i' &&
		lower(buf[6]) == 't'
		)
	{
		exit(0);
	}

	return;
}
