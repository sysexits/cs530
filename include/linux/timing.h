#ifndef _FS_TIMING_H
#define _FS_TIMING_H

#include<linux/module.h>

struct stat_timestamp_pair {
	unsigned long long before, after;
	int value;
};

struct sendfile_timestamp_pair {
	unsigned long long before, after, value;
};

struct stat_timestamp {
	struct stat_timestamp_pair
		vfs_fstatat_userpath_lookup_setnameidata,
		vfs_fstatat_userpath_lookup_pathlookupat,
		vfs_fstatat_userpath_lookup_auditinode,
		vfs_fstatat_userpath_lookup_putname,
		vfs_fstatat_userpath,
		vfs_fstatat_getattr_getattr,
		vfs_fstatat_getattr_fillattr,
		vfs_fstatat_getattr,
		vfs_stat,
		cp_stat
		;      
};

struct sendfile_timestamp {
	struct sendfile_timestamp_pair
		do_sendfile,
		in_fdget,
		out_fdget,
		rw_verify_area,
		file_start_write,
		dsd_rw_verify_area,
		dsd_splice_direct_to_actor,
		do_splice_direct,
		file_end_write;
};

__inline static void stat_tp_time(struct stat_timestamp_pair *pair){
	pair->before = rdtsc_ordered();
}

__inline static void stat_tp_timeEnd(struct stat_timestamp_pair *pair, int value){
	pair->after = rdtsc_ordered();
	pair->value = value;
}

__inline static void sendfile_tp_time(struct sendfile_timestamp_pair *pair){
	pair->before = rdtsc_ordered();
}

__inline static void sendfile_tp_timeEnd(struct sendfile_timestamp_pair *pair, unsigned long long value){
	pair->after = rdtsc_ordered();
	pair->value = value;
}

#endif
