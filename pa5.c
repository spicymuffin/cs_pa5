#pragma GCC optimize ("O2")

#include <unistd.h>	//read(), write(), close()
#include <fcntl.h> //open()
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
char lower(char c);
void word_parse(char* input_buf, int input_len, word_metadata_t* word_mds, int* word_cnt, char delim);
void kmp_find_print(int line_number, char* text, int text_len, char* pattern, int pattern_len, int* failure_func);
int write_int(char* dest, int num);
int write_str(char* dest, char* src, int len);
int write_char(char* dest, char c);
void print_int(int num);
void print_str(char* str, int len);
void compute_fail_function(int* failure_func, char* pattern, int pattern_len);
void check_exit(char* buf, int len);
void case1_handler(char* text, int text_len, char* pattern, int pattern_len);
void case2_handler(char* text, int text_len, char* words, word_metadata_t* word_mds, int word_cnt);
void case3_handler(char* buf, int len);
void case4_handler(char* buf, int len);

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

		// input_buf is not null

		// check exit condition
		check_exit(input_buf, idx);

		int word_cnt = 0;

		// determine case
		if (input_buf[0] == '"')
		{
			// case 3
		}
		else if (case4_flag)
		{
			// case 4
			word_parse(input_buf, idx, word_mds, &word_cnt, '*');
			// word count guaranteed to be 2

		}
		else
		{
			print_int(idx);
			// case 1, 2
			word_parse(input_buf, idx, word_mds, &word_cnt, ' ');

			if (word_cnt == 1)
			{

				#if DEBUG
				print_str("working...", 10);
				#endif
				// case 1
				// compute failure function
				compute_fail_function(failure_func, input_buf, idx);

				// read 1 line into line_buf
				int line_no = 1;
				int bytes_read = 0;

				int text_buf_idx = 0;
				int line_buf_idx = 0;

				do
				{
					bytes_read = read(ifd, text_buf, FILE_READ_CHUNK);

					while (text_buf_idx < bytes_read)
					{
						while (line_buf[line_buf_idx] != '\n' && text_buf_idx < bytes_read)
						{
							line_buf[line_buf_idx++] = text_buf[text_buf_idx++];
						}

						// if we exited the while loop because of \n, we finished reading a line,
						// so we can start processing it
						if (line_buf[line_buf_idx] == '\n')
						{
							line_buf[line_buf_idx] = 0;
							kmp_find_print(line_no, line_buf, line_buf_idx, input_buf, idx, failure_func);
							line_buf_idx = 0;
						}
						// else we exited the while loop because we reached the end of the read buffer
						// part of the line is already in the line buffer, so we need to copy the rest
						// into the line buffer in the next iteration

						// skip the \n character
						text_buf_idx++;
					}

					text_buf_idx = 0;

				} while (bytes_read > 0);

				// last line in file didn't have a \n
				if (line_buf_idx > 0)
				{
					line_buf[line_buf_idx] = '\0';
					kmp_find_print(line_no, line_buf, line_buf_idx, input_buf, idx, failure_func);
				}
			}
			else
			{
				// case 2
				// case2_handler(input_buf, idx, );
			}
		}
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

// find single word locations
void case1_handler(char* text, int text_len, char* pattern, int pattern_len)
{

}

void case2_handler(char* text, int text_len, char* words, word_metadata_t* word_mds, int word_cnt)
{
	for (int i = 0; i < word_cnt; i++)
	{
		// find word in text
	}
}

void case3_handler(char* buf, int len)
{

}

void case4_handler(char* buf, int len)
{

}

void word_parse(char* input_buf, int input_len, word_metadata_t* word_mds, int* word_cnt, char delim)
{
	int word_start_ptr = 0;
	int word_counter = 0;

	input_len += 1; // to account for the null terminator

	for (int i = 0; i < input_len; i++)
	{
		if (input_buf[i] == '\0')
		{
			// end of input
			word_counter++;
			break;
		}
		else if (input_buf[i] == delim)
		{
			// word found
			word_mds[word_counter].word_start_ptr = word_start_ptr;
			word_mds[word_counter].word_len = i - word_start_ptr;
			word_counter++;

			if (input_buf[i + 1] == '\0')
			{
				// end of input
				break;
			}
			else
			{
				word_start_ptr = i + 1;
			}
		}
	}
	print_int(word_counter);
	*word_cnt = word_counter;
}

// use knuth-morris-pratt and print the locations of all instances of pattern in text
void kmp_find_print(int line_number, char* text, int text_len, char* pattern, int pattern_len, int* failure_func)
{
	int text_idx = 0;
	int pattern_idx = 0;
	int first = 0;

	while (text_idx < text_len)
	{
		if (text[text_idx] == pattern[pattern_idx])
		{
			text_idx++;
			pattern_idx++;
			if (pattern_idx == pattern_len)
			{
				// pattern found
				// prepare output buffer
				char buf[512]; // arbitrart size
				int buf_idx = 0;

				// if not first then print space
				if (!first)
				{
					buf_idx += write_char(buf, ' ');
				}
				else
				{
					first = 1;
				}
				buf_idx += write_int(buf + buf_idx, line_number);
				buf_idx += write_char(buf + buf_idx, ':');
				buf_idx += write_int(buf + buf_idx, text_idx - pattern_len);
				print_str(buf, buf_idx);
				pattern_idx = failure_func[pattern_idx - 1];
			}
		}
		else
		{
			if (pattern_idx != 0)
			{
				pattern_idx = failure_func[pattern_idx - 1];
			}
			else
			{
				text_idx++;
			}
		}
	}
}

// use knuth-morris-pratt and return 1 if pattern is present in text else 0
int kmp_check_presence()
{

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

void compute_fail_function(int* failure_func, char* pattern, int pattern_len)
{
	failure_func[0] = 0;
	for (int i = 0; i < pattern_len; i++)
	{
		int j = failure_func[i - 1];
		while (pattern[i] != pattern[j] && j >= 1)
		{

			j = failure_func[j - 1];
			if (pattern[i] == pattern[j])
			{
				failure_func[i] = j + 1;
			}
			else
			{
				failure_func[i] = 0;
			}
		}
	}
}

// if buf is PA5EXIT (case insinsitive)
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