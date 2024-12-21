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
#define INPUT_BUF_SIZE 512
#define LINE_BUF_SIZE 1024

typedef struct word_metadata
{
	int word_start_ptr;
	int word_len;
} word_metadata_t;

int ifd, iret;

// word parser
void word_parse(char* input_buf, int input_len, word_metadata_t* word_mds, int* word_cnt, char delim);

// char utils
char lower(char c);
char tab_to_space(char c);

// write
int write_int(char* dest, int num);
int write_str(char* dest, char* src, int len);
int write_char(char* dest, char c);

// print
void print_int(int num);
void print_str(char* str, int len);
void print_diagnostic(char* str, int val);

// case handlers
void case1(char* input_buf, int input_len, word_metadata_t* input_word_mds, int input_word_cnt);
void case2(char* input_buf, int input_len, word_metadata_t* input_word_mds, int input_word_cnt);
void case3(char* input_buf, int input_len, word_metadata_t* input_word_mds, int input_word_cnt);
void case4(char* input_buf, int input_len, word_metadata_t* input_word_mds, int input_word_cnt);

// exit
void check_exit(char* buf, int len);

char read_buf[FILE_READ_CHUNK];
int read_buf_idx = 0;
int bytes_read = 0;
int line_no = 1;
int first = 1;

/*
 * reads one line from `ifd` into `dest_buf`, storing *all* characters
 * (including delimiters) in `dest_buf`.  the function also parses tokens
 * on-the-fly, storing them in `word_mds`. zero-length words are recorded
 * for consecutive delimiters or delimiters at the start/end.
 *
 * returns the number of characters written into dest_buf (excluding the
 * terminating '\0').
 */
int read_parse_line(
	char* dest_buf,
	int* ln,
	int* eof,
	word_metadata_t* word_mds,
	int* word_cnt,
	char delim
)
{
	int dest_buf_idx = 0;        // Where we're writing in dest_buf
	int word_start_offset = 0;   // Start offset of the current word in dest_buf
	int word_counter = 0;        // How many words parsed so far

	// First read if we haven't yet
	if (first)
	{
		bytes_read = read(ifd, read_buf, sizeof(read_buf));
		first = 0;
	}

	while (1)
	{
		// if no more data in read_buf => check EOF
		if (bytes_read == 0)
		{
			// we are done reading the file
			// finalize the last word (which could be zero-length).
			// only do this if we want to treat the partial line at EOF as valid.
			if (dest_buf_idx >= 0)
			{
				// finalize the last word
				word_mds[word_counter].word_start_ptr = word_start_offset;
				word_mds[word_counter].word_len = dest_buf_idx - word_start_offset;
				word_counter++;
			}

			// null-terminate the line
			dest_buf[dest_buf_idx] = '\0';

			// this is effectively the end
			*ln = line_no;
			*eof = 1;
			*word_cnt = word_counter;

			// return how many bytes we wrote to dest_buf
			return dest_buf_idx;
		}

		// process the current chunk until we see a newline or exhaust read_buf
		while (read_buf_idx < bytes_read && read_buf[read_buf_idx] != '\n')
		{
			char c = read_buf[read_buf_idx++];
			c = tab_to_space(c);

			// check if this character is our delimiter
			if (c == delim)
			{
				// finalize the current word (could be zero-length)
				word_mds[word_counter].word_start_ptr = word_start_offset;
				word_mds[word_counter].word_len = dest_buf_idx - word_start_offset;
				word_counter++;

				// store the delimiter in dest_buf
				dest_buf[dest_buf_idx++] = c;

				// the new word starts after the delimiter
				word_start_offset = dest_buf_idx;
			}
			else
			{
				// not a delimiter => part of a word
				dest_buf[dest_buf_idx++] = c;
			}
		}

		// did we exit because we found a newline?
		if (read_buf_idx < bytes_read)
		{
			// we found a newline
			read_buf_idx++; // skip the newline character

			// finalize the last word in this line
			word_mds[word_counter].word_start_ptr = word_start_offset;
			word_mds[word_counter].word_len = dest_buf_idx - word_start_offset;
			word_counter++;

			// null-terminate the line
			dest_buf[dest_buf_idx] = '\0';

			// we got a full line => update line number
			*ln = line_no++;
			*eof = 0;
			*word_cnt = word_counter;

			// return how many bytes were written (excluding '\0')
			return dest_buf_idx;
		}
		else
		{
			// we used up everything in read_buf but did NOT see a newline
			// => read next chunk
			bytes_read = read(ifd, read_buf, sizeof(read_buf));
			read_buf_idx = 0;
		}
	}
}

void reset_read()
{
	read_buf_idx = 0;
	bytes_read = 0;
	line_no = 1;
	first = 1;
	lseek(ifd, 0, SEEK_SET);
}

int main(int argc, char** argv)
{
	char input_buf[INPUT_BUF_SIZE];
	word_metadata_t input_word_mds[INPUT_BUF_SIZE];

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
			// remove double quotes, null-terminate
			input_buf[idx - 1] = 0;
			word_parse(input_buf + 1, idx - 2, input_word_mds, &word_cnt, ' ');

			#if DEBUG

			print_str("input_buf: ", 11);
			print_str(input_buf, idx);
			print_str("\n", 1);

			print_str("word_cnt: ", 10);
			print_int(word_cnt);
			print_str("\n", 1);

			for (int i = 0; i < word_cnt; i++)
			{
				print_str("word: ", 6);
				print_int(i);
				print_str(" start: ", 8);
				print_int(input_word_mds[i].word_start_ptr);
				print_str(" len: ", 6);
				print_int(input_word_mds[i].word_len);
				print_str("\n", 1);
			}

			#endif

			case3(input_buf + 1, idx - 2, input_word_mds, word_cnt);
		}
		else if (case4_flag)
		{
			// case 4 - regex
			word_parse(input_buf, idx, input_word_mds, &word_cnt, '*');

			// word count guaranteed to be 2
			case4(input_buf, idx, input_word_mds, word_cnt);
		}
		else
		{
			word_parse(input_buf, idx, input_word_mds, &word_cnt, ' ');

			if (word_cnt == 1)
			{
				// case 1 - single word
				case1(input_buf, idx, input_word_mds, word_cnt);
			}
			else
			{
				// case 2 - multiple words
				case2(input_buf, idx, input_word_mds, word_cnt);
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

int wordmatch(char* word1, char* word2, int word1_len, int word2_len)
{
	if (word1_len != word2_len)
	{
		return 0;
	}

	for (int i = 0; i < word1_len; i++)
	{
		if (lower(word1[i]) != lower(word2[i]))
		{
			return 0;
		}
	}

	return 1;
}

// case1: single word
void case1(char* input_buf, int input_len, word_metadata_t* input_word_mds, int input_word_cnt)
{
	int ln = 0;
	int eof = 0;

	int line_word_cnt = 0;
	char line_buf[LINE_BUF_SIZE];
	word_metadata_t line_word_mds[LINE_BUF_SIZE];

	do
	{
		int line_len = read_parse_line(line_buf, &ln, &eof, line_word_mds, &line_word_cnt, ' ');

		if (line_len == 0)
		{
			continue;
		}

		for (int i = 0; i < line_word_cnt; i++)
		{
			if (wordmatch(
				line_buf + line_word_mds[i].word_start_ptr,
				input_buf + input_word_mds[0].word_start_ptr,
				line_word_mds[i].word_len,
				input_word_mds[0].word_len))
			{
				print_int(ln);
				print_str(":", 1);
				print_int(line_word_mds[i].word_start_ptr);
				print_str(" ", 1);
			}
		}

	} while (!eof);

	print_str("\n", 1);

	reset_read();
}

// case2: check that all words are present
void case2(char* input_buf, int input_len, word_metadata_t* input_word_mds, int input_word_cnt)
{
	int ln = 0;
	int eof = 0;

	int line_word_cnt = 0;
	char line_buf[LINE_BUF_SIZE];
	word_metadata_t line_word_mds[LINE_BUF_SIZE];

	do
	{
		int line_len = read_parse_line(line_buf, &ln, &eof, line_word_mds, &line_word_cnt, ' ');

		if (line_len == 0)
		{
			continue;
		}

		int all_present = 1;
		for (int i = 0; i < input_word_cnt; i++)
		{
			int present = 0;
			for (int j = 0; j < line_word_cnt; j++)
			{
				if (wordmatch(
					line_buf + line_word_mds[j].word_start_ptr,
					input_buf + input_word_mds[i].word_start_ptr,
					line_word_mds[j].word_len,
					input_word_mds[i].word_len))
				{
					present = 1;
					break;
				}
			}

			if (!present)
			{
				all_present = 0;
				break;
			}
		}

		if (all_present)
		{
			print_int(ln);
			print_str(" ", 1);
		}

	} while (!eof);

	print_str("\n", 1);

	reset_read();
}

// case3: double quotes, find words that are consecutive
void case3(char* input_buf, int input_len, word_metadata_t* input_word_mds, int input_word_cnt)
{
	int ln = 0;
	int eof = 0;

	int line_word_cnt = 0;
	char line_buf[LINE_BUF_SIZE];
	word_metadata_t line_word_mds[LINE_BUF_SIZE];

	do
	{
		int line_len = read_parse_line(line_buf, &ln, &eof, line_word_mds, &line_word_cnt, ' ');

		if (line_len == 0)
		{
			continue;
		}

		if (line_word_cnt < input_word_cnt)
		{
			continue;
		}

		// plus 1 becasue strict sign <
		for (int i = 0; i < line_word_cnt - input_word_cnt + 1; i++)
		{
			int all_present = 1;
			for (int j = 0; j < input_word_cnt; j++)
			{
				if (!wordmatch(
					line_buf + line_word_mds[i + j].word_start_ptr,
					input_buf + input_word_mds[j].word_start_ptr,
					line_word_mds[i + j].word_len,
					input_word_mds[j].word_len))
				{
					all_present = 0;
					break;
				}
			}

			if (all_present)
			{
				print_int(ln);
				print_str(":", 1);
				print_int(line_word_mds[i].word_start_ptr);
				print_str(" ", 1);
			}
		}

	} while (!eof);

	print_str("\n", 1);

	reset_read();
}

// case4: regex, find patterns of word1[delim]{at least 1 word}[delim]word2
void case4(char* input_buf, int input_len, word_metadata_t* input_word_mds, int input_word_cnt)
{
	int ln = 0;
	int eof = 0;

	int line_word_cnt = 0;
	char line_buf[LINE_BUF_SIZE];
	word_metadata_t line_word_mds[LINE_BUF_SIZE];

	do
	{
		int line_len = read_parse_line(line_buf, &ln, &eof, line_word_mds, &line_word_cnt, ' ');

		if (line_len == 0)
		{
			continue;
		}

		// must have at least 3 words
		if (line_word_cnt < 3)
		{
			continue;
		}

		for (int i = 0; i < line_word_cnt - 3 + 1; i++)
		{
			if (!wordmatch(
				line_buf + line_word_mds[i].word_start_ptr,
				input_buf + input_word_mds[0].word_start_ptr,
				line_word_mds[i].word_len,
				input_word_mds[0].word_len))
			{
				continue;
			}
			else
			{
				// = number of words after word1 before word2
				int j = 0;

				while (!wordmatch(
					line_buf + line_word_mds[i + j + 1].word_start_ptr,
					input_buf + input_word_mds[1].word_start_ptr,
					line_word_mds[i + j + 1].word_len,
					input_word_mds[1].word_len))
				{
					j++;
					if (i + j + 1 >= line_word_cnt)
					{
						break;
					}
				}

				// if we reached the end of the line without finding word2
				if (i + j + 1 >= line_word_cnt)
				{
					continue;
				}
				// else we found word2
				else
				{
					// if there is a word between word1 and word2
					if (j > 0)
					{
						print_int(ln);
						print_str(" ", 1);
						break;
					}
				}
			}
		}

	} while (!eof);

	print_str("\n", 1);

	reset_read();
}

char lower(char c)
{
	return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
}

char tab_to_space(char c)
{
	return c == '\t' ? ' ' : c;
}

void word_parse(char* input_buf, int input_len, word_metadata_t* word_mds, int* word_cnt, char delim)
{
	int word_start = 0;
	int word_counter = 0;

	for (int i = 0; i <= input_len; i++)
	{
		if (tab_to_space(input_buf[i]) == delim || input_buf[i] == '\0')
		{

			word_mds[word_counter].word_start_ptr = word_start;
			word_mds[word_counter].word_len = i - word_start;

			word_counter++;
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
