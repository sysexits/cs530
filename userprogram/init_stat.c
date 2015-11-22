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
#include <dirent.h>
#include <sys/sendfile.h> // target syscall
#include <sys/socket.h> // socket
#include <sys/types.h> // socket type
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <linux/limits.h>

#define __inline __attribute__((always_inline))

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

__inline unsigned long long rdtsc(void)
{
	DECLARE_ARGS(val, low, high);
	asm volatile("rdtsc" : EAX_EDX_RET(val, low, high));
	return EAX_EDX_VAL(val, low, high);
}

__inline unsigned long long rdtsc_ordered(void)
{
	alternative_2("", "mfence", X86_FEATURE_MFENCE_RDTSC,
			  "lfence", X86_FEATURE_LFENCE_RDTSC);
	return rdtsc();
}

__inline int stat_printk(const char *s, struct stat *stats) {
	unsigned long long before, after;
	int result;
	before = rdtsc_ordered();
	result = syscall(547, s, stats);
	after = rdtsc_ordered();
	printf("cs530\tstat\tuser\t%d\t%lld\t\"%s\"\n", result, after-before, s);
	return result;
}

__inline int load_as_file(char *s, struct stat *stats) {
	int stat_result, result;
	int l = strlen(s);

	if(!stat_printk(s, stats)){
		if(S_ISREG(stats->st_mode)) return 0;
	}

	strcat(s, ".js");

	if(!stat_printk(s, stats)){
		if(S_ISREG(stats->st_mode)){
			s[l] = '\0';
			return 0;
		}
	}
	
	strcat(s, "on");
	
	if(!stat_printk(s, stats)){
		if(S_ISREG(stats->st_mode)){
			s[l] = '\0';
			return 0;
		}
	}

	s[l+1] = '\0';
	strcat(s, "node");
	
	if(!stat_printk(s, stats)){
		if(S_ISREG(stats->st_mode)){
			s[l] = '\0';	
			return 0;
		}
	}

	s[l] = '\0';
	return -1;
}

__inline int load_as_directory(char *s, struct stat *stats) {
	int l = strlen(s);
	strcat(s, "/package.json");

	if(!stat_printk(s, stats)){
		if(S_ISREG(stats->st_mode)){
			s[l] = '\0';
			return 0;
		}
	}
	
	s[l+1] = '\0';
	strcat(s, "index.js");

	if(!stat_printk(s, stats)){
		if(S_ISREG(stats->st_mode)){
			s[l] = '\0';
			return 0;
		}
	}

	strcat(s, "on");

	if(!stat_printk(s, stats)){
		if(S_ISREG(stats->st_mode)){
			s[l] = '\0';
			return 0;
		}
	}

	s[l+7] = '\0';
	strcat(s, "node");

	if(!stat_printk(s, stats)){
		if(S_ISREG(stats->st_mode)){
			s[l] = '\0';
			return 0;
		}
	}

	s[l] = '\0';
	return -1;
}

int node_require(const char *cwd, const char *s, struct stat *require_stat) {
	char buffer[256];
	int i, cl = strlen(cwd);

	if(!strcmp(s, "http") || !strcmp(s, "fs")){
		return 0;
	}

	strcpy(buffer, cwd);
	if(s[0] == '/' || s[0] == '.' && (s[1] == '/' || s[1] == '.' && s[2] == '/')){
		strcat(buffer, s);
		
		if(!load_as_file(buffer, require_stat)) return 0;
		if(!load_as_directory(buffer, require_stat)) return 0;
	}

	for(i=cl-1; i>=0; i--){
		if(cwd[i] == '/'){
			buffer[i+1] = '\0';
			strcat(buffer, "node_modules/");
			strcat(buffer, s);
			if(!load_as_file(buffer, require_stat)) return 0;
			if(!load_as_directory(buffer, require_stat)) return 0;
		}
	}

	return -1;
}

__inline void node_require_test(char *cwd, const char *s, struct stat *require_stat){
	int result;
	int cl = strlen(cwd);

	if(cwd[cl-1] != '/'){
		cwd[cl] = '/';
		cwd[cl+1] = '\0';
		cl++;
	}

	result = node_require(cwd, s, require_stat);
	printf("require %s = %d\n", s, result);

	if(!result){
		printf("File size: %zu bytes\n", require_stat->st_size);
	}else{
		printf("File not found\n");
	}
}

int main()
{
	/**
	 * init - server side stat performance measurement
	 */
	
	struct stat file_stat;
	struct timespec ts;
	int i;

	ts.tv_sec = 1;
	ts.tv_nsec = 0;

	printf("init stat() ver.\n");

	for(i=0;i<100;i++){
		node_require_test("/", "./hello", &file_stat);
		node_require_test("/static/foo/bar/", "./hello", &file_stat);
		node_require_test("/static/home/test/", "./hello", &file_stat);
		node_require_test("/static/foo/", "hello", &file_stat);
		node_require_test("/static/foo/", "world", &file_stat);
		node_require_test("/static/foo/", "asdf", &file_stat);
		nanosleep(&ts, NULL);
	}

	return 0;
}
