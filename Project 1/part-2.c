/*
 * file:        part-2.c
 * description: Part 2, CS5600 Project 1, Spring 2022
 */

/* NO OTHER INCLUDE FILES */
#include "elf64.h"
#include "sysdefs.h"

extern void *vector[];
#define BUFFER_SIZE 201
char BUF[200];
char *argv[10];
int argc;

/* ---------- */

/* write these functions
 */
int read(int fd, void *ptr, int len);
int write(int fd, void *ptr, int len);
void exit(int err);
int open(char *path, int flags);
int close(int fd);
int lseek(int fd, int offset, int flag);
void *mmap(void *addr, int len, int prot, int flags, int fd, int offset);
int munmap(void *addr, int len);
int split(char **argv, int max_argc, char *line);

// len: number of bytes to read
// return: 1 for success
int read(int fd, void *ptr, int len)
{
	if (len < 1)
	{
		return -1;
	}

	syscall(__NR_read, fd, ptr, len);
	return 1;
}

int write(int fd, void *ptr, int len)
{
	if (len < 1)
	{
		return -1;
	}
	syscall(__NR_write, fd, ptr, len);
	return 1;
}

void exit(int err)
{
	syscall(__NR_exit, err);
}

int open(char *path, int flags)
{
	return syscall(__NR_open, path, flags);
}

int close(int fd)
{
	return syscall(__NR_close, fd);
}

int lseek(int fd, int offset, int flag)
{
	return syscall(__NR_lseek, fd, offset, flag);
}

void *mmap(void *addr, int len, int prot, int flags, int fd, int offset)
{
	return (void *)syscall(__NR_mmap, addr, len, prot, flags, fd, offset);
}

int munmap(void *addr, int len)
{
	return syscall(__NR_munmap, addr, len);
}

/* print a string to stdout (file descriptor 1) */
void print(char *buffer)
{
	for (int i = 0; i < BUFFER_SIZE; i++)
	{
		write(1, &buffer[i], 1);
		if (buffer[i] == '\0')
		{
			break;
		}
	}
}

int strcmp(const char *str1, const char *str2)
{
	for (int i = 0; i < 4; i++)
	{
		if (*str1 && (*str1 == *str2))
		{
			str1++;
			str2++;
		}
	}
	if (str2[0] == '\0')
	{
		return 0;
	}
	return (*str1 > *str2) - (*str1 < *str2);
}

/* read one line from stdin (file descriptor 0) into a buffer: */
void readline(char *buffer)
{

	for (int i = 0; i < BUFFER_SIZE; i++)
	{
		read(0, &buffer[i], 1);
		if (buffer[i] == '\n')
		{
			buffer[++i] = '\0';
			break;
		}
	}
}

/* ---------- */

/* the three 'system call' functions - readline, print, getarg
 * hints:
 *  - read() or write() one byte at a time. It's OK to be slow.
 *  - stdin is file desc. 0, stdout is file descriptor 1
 *  - use global variables for getarg
 */

void do_readline(char *buf, int len)
{
	int i;
	for (i = 0; i < len; i++)
	{
		read(0, &buf[i], 1);
		if (buf[i] == '\n')
		{
			buf[++i] = '\0';
			break;
		}
	}
}

void do_print(char *buf)
{
	int i;
	for (i = 0; i < BUFFER_SIZE; i++)
	{
		write(1, &buf[i], 1);
		if (buf[i] == '\0')
		{
			break;
		}
	}
}
char *do_getarg(int i)
{
	int argc = split(argv, 10, BUF);
	if (i >= argc)
	{
		return 0;
	}
	return argv[i];
}

/* ---------- */

/* the guts of part 2
 *   read the ELF header
 *   for each section, if b_type == PT_LOAD:
 *     create mmap region
 *     read from file into region
 *   function call to hdr.e_entry
 *   munmap each mmap'ed region so we don't crash the 2nd time
 */

/* your code here */

/* ---------- */

/* simple function to split a line:
 *   char buffer[200];
 *   <read line into 'buffer'>
 *   char *argv[10];
 *   int argc = split(argv, 10, buffer);
 *   ... pointers to words are in argv[0], ... argv[argc-1]
 */
int split(char **argv, int max_argc, char *line)
{
	int i = 0;
	char *p = line;

	while (i < max_argc)
	{
		while (*p != 0 && (*p == ' ' || *p == '\t' || *p == '\n'))
			*p++ = 0;
		if (*p == 0)
			return i;
		argv[i++] = p;
		while (*p != 0 && *p != ' ' && *p != '\t' && *p != '\n')
			p++;
	}
	return i;
}

/* ---------- */

void main(void)
{

	vector[0] = do_readline;
	vector[1] = do_print;
	vector[2] = do_getarg;
	// do_readline(param);

	char *exit_str = "quit";

	while (1)
	{
		print("> ");
		readline(BUF);
		if (strcmp(BUF, exit_str) == 0)
		{
			exit(1);
		}

		char *filename = do_getarg(0); /* I should really check argc first... */
		int fd;
		/* YOUR CODE HERE */
		if ((fd = open(filename, O_RDONLY)) < 0)
			exit(1); /* failure code */
		// fd now points to where the executable file is

		/* read the main header (offset 0) */
		struct elf64_ehdr hdr;
		read(fd, &hdr, sizeof(hdr));
		// store the executable's info in hdr -> elf header
		// do_print("entry point address: " + str(hdr.e_entry));

		/* read program headers (offset 'hdr.e_phoff') */
		int i, n = hdr.e_phnum;
		struct elf64_phdr phdrs[n];
		lseek(fd, hdr.e_phoff, SEEK_SET);
		// there are a lof of info in ELF hearder, we need to locate
		// program header table. Lseek is to move the fd to program header table in ELF header
		read(fd, phdrs, sizeof(phdrs));
		// read the program header table from the executable and stores it
		// in memory at address phdrs (an array)

		// ----------------------determine the value of M_offset------------------------------
		// multiple of 4096 = 64 * 4096
		int M_offset = 64 * 4096;
		char *buf[n]; // array of pointer
		/* look at each section in program headers */
		for (i = 0; i < hdr.e_phnum; i++)
		{
			if (phdrs[i].p_type == PT_LOAD)
			{
				// not all segments should be loaded in memory, only those with p_type PT_LOAD
				int len = ROUND_UP(phdrs[i].p_memsz, 4096);
				long addrp = ROUND_DOWN((long)phdrs[i].p_vaddr, 4096); // starting address of the page

				buf[i] = mmap((void *)(addrp + M_offset), len, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
				if (buf == MAP_FAILED)
				{
					do_print("mmap failed\n");
					exit(0);
				}
				lseek(fd, (int)phdrs[i].p_offset, SEEK_SET); // on disk, fd pointer to disk
				read(fd, phdrs[i].p_vaddr + M_offset, (int)phdrs[i].p_filesz);
			}
		}
		void (*f)();
		f = hdr.e_entry + M_offset;
		f();
		// for loop to mumap()

		for (i = 0; i < hdr.e_phnum; i++)
		{
			if (phdrs[i].p_type == PT_LOAD)
			{
				// not all segments should be loaded in memory, only those with p_type PT_LOAD
				int len = ROUND_UP(phdrs[i].p_memsz, 4096);

				int value = munmap((void *)(buf[i]), len);
				if (value < 0)
				{
					do_print("munmap failed\n");
					exit(0);
				}
			}
		}

		close(fd);
	}
}
