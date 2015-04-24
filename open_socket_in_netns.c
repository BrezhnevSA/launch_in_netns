#include <sys/param.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#ifndef NETNS_RUN_DIR
#define NETNS_RUN_DIR "/var/run/netns"
#endif

#define CLONE_NEWNET 0x40000000	

#define PART1_GLOBAL_NETNS_PATH "/proc/" 
#define PART2_GLOBAL_NETNS_PATH "/ns/net"

static int netns_change(const char *nsname, int fd_global_netns) {
/* If you need to change current netns to global netns:
 * set the first argument "NULL", the second argument 
 * is file descriptor of global netns (it must be saved
 * before first call of this function)  
 * If you need to change current netns to another netns:
 * set the first argument as netns which you need,
 * the second argument may be any
*/  
	char netns_path[MAXPATHLEN];
	int fd_netns;

 /*  Join to global network namespace
  *  if (network namespace "nsname" == NULL && 
  *  file descriptor of global network namespace
  *  "fd_global_netns" >-1)
  *  else exit function with error -1
  *  if (file descriptor of global network namespace
  *  "fd_global_netns" < 0) 
 */
	if (nsname == NULL) {
		fprintf(stdout,"*** fd_global_netns is:%d\n",fd_global_netns);
		if (setns(fd_global_netns, CLONE_NEWNET) == -1) {			
			return -1;
		}
		return 0;
	}

/* Get path for network namespace "nsname" */	
	snprintf(netns_path, sizeof(netns_path), "%s/%s", NETNS_RUN_DIR, nsname);

/* Create the base netns directory if it doesn't exist */
	mkdir(NETNS_RUN_DIR, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);

/* Create(or open) netns file */
	fd_netns = open(netns_path, O_RDONLY|O_CREAT|O_EXCL, 0);
	if (fd_netns < 0) {		
		return -2;
	}
	close(fd_netns);

/* Join to network namespace "nsname" */
	if (unshare(CLONE_NEWNET) < 0) {		
		return -1;
	}
	 
	return 0;
}

int open_socket_in_netns(const char *nsname, int domain, int type, int protocol) {
	int fd_global_netns; /* File descriptor of global network namespace */
	int fd_socket; /* File descriptor of socket which openning in netns "nsname" */
	int exit_stat; /* Exit status of function netns_change() */
	static char path1[MAXPATHLEN] = {0}; /* Path to global netns of this process */
    static char pid_[30] = {0}; /* Pid the current process */
	
/* Get path to the global network namespace of this process */  
	strcat(path1,PART1_GLOBAL_NETNS_PATH);
	sprintf(pid_, "%ld", (long)getpid());
	strcat(path1,pid_);
	strcat(path1,PART2_GLOBAL_NETNS_PATH);
	
/* Save file descriptor of global network namespace */
	fd_global_netns = open(path1, O_RDONLY, 0);
	fprintf(stdout,"*** path1 is:%s, fd_global_netns is:%d\n",path1,fd_global_netns);
	if (fd_global_netns < 0) {
		fprintf(stderr, "*** Cannot get file descriptor of global namespace file : %s\n",
				strerror(errno));
		return -1;
	}	
	
/* Change netns to "nsname" */	
	exit_stat = netns_change(nsname, fd_global_netns);
	if (exit_stat == -1) {
		fprintf(stderr, "*** Failed to create(or set) a new network namespace \"%s\": %s\n",
			nsname, strerror(errno));
		return -1;
	}
	else if (exit_stat == -2) {
		fprintf(stderr, "*** Cannot create network namespace(\"%s\") file : %s\n",
			nsname, strerror(errno));
		return -2;
	}

/* Open socket in netns "nsname" */	
	fd_socket = socket(domain, type, protocol);
    if (fd_socket < 0) {
        fprintf(stderr, "*** Cannot open socket in network namespace \"%s\": %s\n",
				nsname, strerror(errno));
		return -2;
	}
	
/* Return to global netns */	
	netns_change(NULL, fd_global_netns);
	if (exit_stat == -1) {
		fprintf(stderr, "*** Failed to set a global network namespace : %s\n",
				strerror(errno));
		return -1;
	}

/* Close file descriptor of global network namespace */	
	close(fd_global_netns);
	
/*Return file descriptor of socket which openning in netns "nsname" */	
	return fd_socket;
}
