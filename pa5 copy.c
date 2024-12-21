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

// case handlers
void handle_case1(char* input_buf, int idx, int* failure_func, char* text_buf, char* line_buf);
void handle_case2(char* input_buf, int word_cnt, word_metadata_t* word_mds, int* failure_func, char* text_buf, char* line_buf);
void handle_case3(char* input_buf, int idx, int word_cnt, word_metadata_t* word_mds, int* failure_func, char* text_buf, char* line_buf);
void handle_case4(char* input_buf, int idx, int word_cnt, word_metadata_t* word_mds);

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

// kmp
void kmp_find_print(int line_number, int* first, char* text, int text_len, char* pattern, int pattern_len, int* failure_func);
int kmp_check_presence(char* text, int text_len, char* pattern, int pattern_len, int* failure_func);

void compute_fail_function(int* failure_func, char* pattern, int pattern_len);

// exit
void check_exit(char* buf, int len);

// --------------------
//     MAIN
// --------------------
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

		// Read a line from stdin
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
			// case 3
			handle_case3(input_buf, idx, word_cnt, word_mds, failure_func, text_buf, line_buf);
		}
		else if (case4_flag)
		{
			// case 4
			word_parse(input_buf, idx, word_mds, &word_cnt, '*', '*');
			// word count guaranteed to be 2
			handle_case4(input_buf, idx, word_cnt, word_mds);
		}
		else
		{
			// parse with spaces/tabs as delimiters
			word_parse(input_buf, idx, word_mds, &word_cnt, ' ', '\t');

			if (word_cnt == 1)
			{
				// case 1
				handle_case1(input_buf, idx, failure_func, text_buf, line_buf);
			}
			else
			{
				// case 2
				handle_case2(input_buf, word_cnt, word_mds, failure_func, text_buf, line_buf);
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

// --------------------
//  CASE HANDLERS
// --------------------

// case1: single word
void handle_case1
(
	char* input_buf,
	int idx,
	int* failure_func,
	char* text_buf,
	char* line_buf
)
{
	compute_fail_function(failure_func, input_buf, idx);

	int line_no = 1;
	int bytes_read = 0;

	int text_buf_idx = 0;
	int line_buf_idx = 0;

	int first = 1;

	do
	{
		bytes_read = read(ifd, text_buf, FILE_READ_CHUNK);

		while (text_buf_idx < bytes_read)
		{
			while (text_buf_idx < bytes_read && text_buf[text_buf_idx] != '\n')
			{
				line_buf[line_buf_idx++] = text_buf[text_buf_idx++];
			}

			if (text_buf_idx < bytes_read)
			{
				line_buf[line_buf_idx] = 0;

				if (line_buf_idx > 0)
				{
					kmp_find_print(line_no, &first,
						line_buf, line_buf_idx,
						input_buf, idx,
						failure_func);
				}
				line_buf_idx = 0;
				line_no++;
			}
			// skip the \n character
			text_buf_idx++;
		}

		text_buf_idx = 0;
	} while (bytes_read > 0);

	// last line in file didn't have a \n
	if (line_buf_idx > 0)
	{
		line_buf[line_buf_idx] = '\0';
		kmp_find_print(line_no, &first,
			line_buf, line_buf_idx,
			input_buf, idx,
			failure_func);
	}

	print_str("\n", 1);
}

// case2: multiple words
void handle_case2
(
	char* input_buf,
	int word_cnt,
	word_metadata_t* word_mds,
	int* failure_func,
	char* text_buf,
	char* line_buf
)
{
	// compute failure functions for each word
	for (int i = 0; i < word_cnt; i++)
	{
		compute_fail_function(failure_func + word_mds[i].word_start_ptr,
			input_buf + word_mds[i].word_start_ptr,
			word_mds[i].word_len);
	}

	int line_no = 1;
	int bytes_read = 0;

	int text_buf_idx = 0;
	int line_buf_idx = 0;

	int first = 1;

	do
	{
		bytes_read = read(ifd, text_buf, FILE_READ_CHUNK);

		while (text_buf_idx < bytes_read)
		{
			while (text_buf_idx < bytes_read && text_buf[text_buf_idx] != '\n')
			{
				line_buf[line_buf_idx++] = text_buf[text_buf_idx++];
			}

			if (text_buf_idx < bytes_read)
			{
				line_buf[line_buf_idx] = 0;

				if (line_buf_idx > 0)
				{
					int all_present = 1;

					// check if all words are present
					for (int i = 0; i < word_cnt; i++)
					{
						if (kmp_check_presence(
							line_buf,
							line_buf_idx,
							input_buf + word_mds[i].word_start_ptr,
							word_mds[i].word_len,
							failure_func + word_mds[i].word_start_ptr) == 0)
						{
							all_present = 0;
							break;
						}
					}

					if (all_present)
					{
						// prepare output buffer
						char buf[512]; // arbitrary size
						int buf_idx = 0;

						if (!first)
						{
							buf_idx += write_char(buf_idx + buf, ' ');
						}
						else
						{
							first = 0;
						}

						buf_idx += write_int(buf_idx + buf, line_no);
						print_str(buf, buf_idx);
					}
				}

				line_buf_idx = 0;
				line_no++;
			}
			// skip the \n character
			text_buf_idx++;
		}

		text_buf_idx = 0;
	} while (bytes_read > 0);

	// last line in file didn't have a \n
	if (line_buf_idx > 0)
	{
		line_buf[line_buf_idx] = '\0';

		int all_present = 1;
		for (int i = 0; i < word_cnt; i++)
		{
			if (kmp_check_presence(line_buf, line_buf_idx,
				input_buf + word_mds[i].word_start_ptr,
				word_mds[i].word_len,
				failure_func + word_mds[i].word_start_ptr) == 0)
			{
				all_present = 0;
				break;
			}
		}

		if (all_present)
		{
			char buf[512]; // arbitrary size
			int buf_idx = 0;

			if (!first)
			{
				buf_idx += write_char(buf_idx + buf, ' ');
			}
			else
			{
				first = 0;
			}

			buf_idx += write_int(buf_idx + buf, line_no);
			print_str(buf, buf_idx);
		}
	}
	print_str("\n", 1);
}

// case3: double quotes, find words that are consecutive
void handle_case3
(
	char* input_buf,
	int idx,
	int word_cnt,
	word_metadata_t* word_mds,
	int* failure_func,
	char* text_buf,
	char* line_buf
)
{
	// skip the opening quote
	input_buf++;
	// replace the closing quote with null terminator
	input_buf[idx - 1] = 0;
	// decrement idx by two to account for the two quotes
	idx -= 2;

	// parse with spaces/tabs as delimiters
	word_parse(input_buf, idx, word_mds, &word_cnt, ' ', '\t');

	// compute failure functions for each word
	for (int i = 0; i < word_cnt; i++)
	{
		compute_fail_function(failure_func + word_mds[i].word_start_ptr,
			input_buf + word_mds[i].word_start_ptr,
			word_mds[i].word_len);
	}

	int line_no = 1;
	int bytes_read = 0;

	int text_buf_idx = 0;
	int line_buf_idx = 0;

	int first = 1;

	do
	{
		bytes_read = read(ifd, text_buf, FILE_READ_CHUNK);

		while (text_buf_idx < bytes_read)
		{
			while (text_buf_idx < bytes_read && text_buf[text_buf_idx] != '\n')
			{
				line_buf[line_buf_idx++] = text_buf[text_buf_idx++];
			}

			if (text_buf_idx < bytes_read)
			{
				line_buf[line_buf_idx] = 0;

				if (line_buf_idx > 0)
				{
					int start = 0;
					int end = 0;

					// check that all words are present in order
					for (int i = 0; i < word_cnt; i++)
					{
						// search from the
						kmp_find_first(
							line_buf,
							line_buf_idx,
							input_buf + word_mds[i].word_start_ptr,
							word_mds[i].word_len,
							failure_func + word_mds[i].word_start_ptr,
							&start,
							&end
						);

						if (start == -1)
						{
							break;
						}
						if (i == 0)
						{
							first = 1;
						}
						else
						{
							if (start != end)
							{
								first = 0;
							}
						}
					}
				}

				line_buf_idx = 0;
				line_no++;
			}
			// skip the \n character
			text_buf_idx++;
		}

		text_buf_idx = 0;
	} while (bytes_read > 0);

	// last line in file didn't have a \n
	if (line_buf_idx > 0)
	{
		line_buf[line_buf_idx] = '\0';


	}
	print_str("\n", 1);
}

// case4: asterisks
void handle_case4(char* input_buf, int idx, int word_cnt, word_metadata_t* word_mds)
{
	// case 4
	// word_parse already done in main
	// word count guaranteed to be 2
	// The original snippet had no further logic for case 4, so we leave it empty.
	(void)input_buf;
	(void)idx;
	(void)word_cnt;
	(void)word_mds;
}

// --------------------
//   HELPER FUNCTIONS
// --------------------
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

// use knuth-morris-pratt and print the locations of all instances of pattern in text
void kmp_find_print(
	int line_number,
	int* first,
	char* text,
	int text_len,
	char* pattern,
	int pattern_len,
	int* failure_func)
{
	int text_idx = 0;
	int pattern_idx = 0;

	while (text_idx < text_len)
	{
		if (lower(text[text_idx]) == lower(pattern[pattern_idx]))
		{
			text_idx++;
			pattern_idx++;
			if (pattern_idx == pattern_len)
			{
				char buf[512]; // arbitrary size
				int buf_idx = 0;

				if (!(*first))
				{
					buf_idx += write_char(buf + buf_idx, ' ');
				}
				else
				{
					*first = 0;
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
int kmp_check_presence
(
	char* text,
	int text_len,
	char* pattern,
	int pattern_len,
	int* failure_func
)
{
	int text_idx = 0;
	int pattern_idx = 0;

	while (text_idx < text_len)
	{
		if (lower(text[text_idx]) == lower(pattern[pattern_idx]))
		{
			text_idx++;
			pattern_idx++;
			if (pattern_idx == pattern_len)
			{
				return 1; // pattern found
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
	return 0;
}

// find the first instance of pattern in text, return into start and end the indices of the first instance
// if pattern is not found, start and end are set to -1
void kmp_find_first(char* text, int text_len, char* pattern, int pattern_len, int* failure_func, int* start, int* end)
{
	int text_idx = 0;
	int pattern_idx = 0;

	*start = -1;
	*end = -1;

	while (text_idx < text_len)
	{
		if (lower(text[text_idx]) == lower(pattern[pattern_idx]))
		{
			text_idx++;
			pattern_idx++;
			if (pattern_idx == pattern_len)
			{
				*start = text_idx - pattern_len;
				*end = text_idx;
				return;
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

void compute_fail_function(int* failure_func, char* pattern, int pattern_len)
{
	failure_func[0] = 0;
	for (int i = 1; i < pattern_len; i++)
	{
		int j = failure_func[i - 1];
		while (j > 0 && pattern[i] != pattern[j])
		{
			j = failure_func[j - 1];
		}
		if (pattern[i] == pattern[j])
		{
			j++;
		}
		failure_func[i] = j;
	}
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
