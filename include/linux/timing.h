#ifndef _FS_TIMING_H
#define _FS_TIMING_H

#include <linux/module.h>

struct stat_timestamp_pair {
	unsigned long long before, after;
	int value;
};

struct sendfile_timestamp_pair {
	unsigned long long before, after, diff, value;
	int order;
};

struct stat_timestamp {
	int pathlookupat_count,
	    pathwalk_loop_count,
	    lookupat_loop_count;
	struct stat_timestamp_pair
		lookupat_pathinit,
		lookupat_pathwalk_hash[3],
		lookupat_pathwalk_np[3],
		lookupat_pathwalk_component[3],
		lookupat_pathwalk_loop[3],
		lookupat_loop_pathwalk,
		lookupat_last_component_fast,
		component_slow_lock,
		component_slow_hash_cache,
		real_lookup,
		d_alloc,
		component_slow_hash_real,
		component_slow_hash,
		component_slow_unlock,
		lookupat_last_component_slow,
		lookupat_loop_last,
		lookupat_loop,
		lookupat_complete,
		lookupat_terminate,
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
		in_rw_verify_area,
		out_rw_verify_area,
		file_start_write,
		dsd_rw_verify_area,
		dsd_splice_direct_to_actor,
		dsd_sdta_do_splice_to,
		dsd_sdta_do_splice_from,
		dsd_sdta_dsf_splice_from_pipe,
		dsd_sdta_dsf_sfp_spinlock,
		dsd_sdta_dsf_sfp_internal,
		dsd_sdta_dsf_sfp_write_pipe_buf_spinlock,
		dsd_sdta_dsf_sfp_write_pipe_buf_internal,
		dsd_sdta_dsf_sfp_kernel_write,
		dsd_sdta_dsf_sfp_kw_write,
		dsd_sdta_dsf_sfp_kw_new_sync_write,
		dsd_sdta_dsf_sfp_kw_nsw_write_iter,
		vfs_iter_write,
		write_iter,
		sock_write_iter,
		do_splice_direct,
		file_end_write,
		splice_from_pipe,
		generic_file_write_iter,
		kernel_sendpage,
		sendpage,
		ks_sendpage
		;
	int order;
};


__inline static void stat_tp_time(struct stat_timestamp_pair *pair){
	pair->before = rdtsc_ordered();
}

__inline static void stat_tp_timeEnd(struct stat_timestamp_pair *pair, int value){
	pair->after = rdtsc_ordered();
	pair->value = value;
}

__inline static void sendfile_tp_time(struct sendfile_timestamp_pair *pair, int order){
	pair->before = rdtsc_ordered();
	pair->order = order;
}

__inline static void sendfile_tp_timeEnd(struct sendfile_timestamp_pair *pair, unsigned long long value){
	pair->after = rdtsc_ordered();
	pair->diff += pair->after - pair->before;
	pair->value = value;
}

__inline static void sendfile_tp_hwPerf(struct sendfile_timestamp_pair *pair){
}

#endif
