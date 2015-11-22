/** 
 *	User-level performance measuring program 
 *	Description: init is a server side simple web server program
 *		     it simulates apache, nginx sendfile behaviour.	
 *
 *	Author: Jaehyun Jang, Jimin Park
 */
#include <stdio.h>
#include <fcntl.h> // O_WRONLY
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/sendfile.h> // target syscall
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <linux/limits.h>
#include <signal.h>

#define msg_cs530(syscall, function, format) \
	"cs530\t" syscall "\t" function "\t" "%" format "\t" "%llu\n" \

#define DECLARE_ARGS(val, low, high) unsigned long low, high
#define EAX_EDX_VAL(val, low, high) ((low) | (high) << 32)
#define EAX_EDX_RET(val, low, high) "=a" (low), "=d" (high)
#define __stringify_1(x)	#x
#define __stringify(x)		__stringify_1(x)

#define b_replacement(num)	"664"#num
#define e_replacement(num)	"665"#num

#define alt_rlen(num)		e_replacement(num)"f-"b_replacement(num)"f"
#define alt_max_short(a, b)	"((" a ") ^ (((" a ") ^ (" b ")) & -(-((" a ") - (" b ")))))"

#define alt_slen		"662b-661b"
#define alt_end_marker		"663"
#define alt_pad_len		alt_end_marker"b-662b"
#define alt_total_slen		alt_end_marker"b-661b"
#define OLDINSTR_2(oldinstr, num1, num2) \
	"661:\n\t" oldinstr "\n662:\n"								\
	".skip -((" alt_max_short(alt_rlen(num1), alt_rlen(num2)) " - (" alt_slen ")) > 0) * "	\
		"(" alt_max_short(alt_rlen(num1), alt_rlen(num2)) " - (" alt_slen ")), 0x90\n"	\
	alt_end_marker ":\n"

#define ALTINSTR_ENTRY(feature, num)					      \
	" .long 661b - .\n"				/* label           */ \
	" .long " b_replacement(num)"f - .\n"		/* new instruction */ \
	" .word " __stringify(feature) "\n"		/* feature bit     */ \
	" .byte " alt_total_slen "\n"			/* source len      */ \
	" .byte " alt_rlen(num) "\n"			/* replacement len */ \
	" .byte " alt_pad_len "\n"			/* pad len */

#define ALTINSTR_REPLACEMENT(newinstr, feature, num)	/* replacement */     \
	b_replacement(num)":\n\t" newinstr "\n" e_replacement(num) ":\n\t"

#define ALTERNATIVE_2(oldinstr, newinstr1, feature1, newinstr2, feature2)\
	OLDINSTR_2(oldinstr, 1, 2)					\
	".pushsection .altinstructions,\"a\"\n"				\
	ALTINSTR_ENTRY(feature1, 1)					\
	ALTINSTR_ENTRY(feature2, 2)					\
	".popsection\n"							\
	".pushsection .altinstr_replacement, \"ax\"\n"			\
	ALTINSTR_REPLACEMENT(newinstr1, feature1, 1)			\
	ALTINSTR_REPLACEMENT(newinstr2, feature2, 2)			\
	".popsection"


#define alternative_2(oldinstr, newinstr1, feature1, newinstr2, feature2) \
	asm volatile(ALTERNATIVE_2(oldinstr, newinstr1, feature1, newinstr2, feature2) ::: "memory")
#define X86_FEATURE_MFENCE_RDTSC ( 3*32+17) /* "" Mfence synchronizes RDTSC */
#define X86_FEATURE_LFENCE_RDTSC ( 3*32+18) /* "" Lfence synchronizes RDTSC */

int sock;	// Socket descriptor

void int_handler(int);

inline unsigned long long rdtsc(void)
{
	DECLARE_ARGS(val, low, high);
	asm volatile("rdtsc" : EAX_EDX_RET(val, low, high));
	return EAX_EDX_VAL(val, low, high);
}

inline unsigned long long rdtsc_ordered(void)
{
	alternative_2("", "mfence", X86_FEATURE_MFENCE_RDTSC,
			  "lfence", X86_FEATURE_LFENCE_RDTSC);
	return rdtsc();
}

int main(int argc, char **argv)
{
	/**
	 * init - server side sendfile performance measurement
	 */
	int port = 1234;
	
	int desc;	// A file descriptor for socket
	int fd;	// A file descriptor to send	

	struct sockaddr_in addr; // socket parameters for bind
	struct sockaddr_in addr1; // socket parameters for accept

	int addrlen;	// argument to accept
	struct stat stat_buf; // argument to fstat
	off_t offset = 0; // file offset

	int rc;	// holds return code of system calls

	if (argc == 2) {
		port = atoi(argv[1]);
		if (port <= 0) {
			fprintf(stderr, "invalid port: %s\n", argv[1]);
			exit(1);
		}
	} else if(argc != 1) {
		fprintf(stderr, "usage: %s [port]\n", argv[0]);
		exit(1);
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		fprintf(stderr, "unable to create socket: %s\n", strerror(errno));
		goto end;	  
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	rc =  bind(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (rc == -1) {
		fprintf(stderr, "unable to bind to socket: %s\n", strerror(errno)); goto end;
	}

	rc = listen(sock, 1);
	if (rc == -1) {
		fprintf(stderr, "listen failed: %s\n", strerror(errno)); goto end;
	}

	signal(SIGINT, int_handler);

	while(1)
	{
	const char *filename = "./static/index.html\0";
	desc = accept(sock, (struct sockaddr *)  &addr1, &addrlen);
	if (desc == -1) {
		fprintf(stderr, "accept failed: %s\n", strerror(errno)); goto end;
	}

	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "unable to open '%s': %s\n", filename, strerror(errno)); goto end;
	}
	fstat(fd, &stat_buf);

	offset = 0;
	int remains;
	for(remains = stat_buf.st_size; remains > 0; )
	{
		unsigned long long before = rdtsc_ordered();
		rc = syscall(546, desc, fd, &offset, remains);
		unsigned long long after = rdtsc_ordered();
		printf(msg_cs530("sendfile", "user", "d"), rc, after-before);

		if(rc > 0) {
			offset += rc;
			remains -= rc;
		}
		else if(rc == -1)
			break;
	}
	close(desc);	
	close(fd);
	}
end:
	close(sock);
	return 0;
}

void int_handler(int sig)
{
	close(sock);
	exit(1);
}
