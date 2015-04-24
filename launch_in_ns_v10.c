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

static int netns_change(const char *nsname, int fd_global_netns)
{
/* If you need to change current netns to global netns:
 * set the first argument "NULL", the second argument 
 * is file descriptor of global netns (it must be saved
 * before first call of this function)  
 * If you need to change current netns to another netns:
 * set the first argument as netns which you need,
 * the second argument may be any
*/  
	char netns_path[MAXPATHLEN];
	int fd;

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
			fprintf(stderr, "*** Failed to set a global network namespace : %s\n",
				strerror(errno));
			return -2;
		}
		return 0;
	}

/* Get path for network namespace "nsname" */	
	snprintf(netns_path, sizeof(netns_path), "%s/%s", NETNS_RUN_DIR, nsname);

/* Create the base netns directory if it doesn't exist */
	mkdir(NETNS_RUN_DIR, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);

/* Create the filesystem state */
	fd = open(netns_path, O_RDONLY|O_CREAT|O_EXCL, 0);
	if (fd < 0) {
		fprintf(stderr, "*** Cannot create namespace file \"%s\": %s\n",
			netns_path, strerror(errno));
		return -1;
	}
	close(fd);

/* Join to network namespace "nsname" */
	if (unshare(CLONE_NEWNET) < 0) {
		fprintf(stderr, "*** Failed to create a new network namespace \"%s\": %s\n",
			nsname, strerror(errno));
		return -1;
	}
	 
	return 0;
}

int main(int argc, char** argv)
{
  int fd_global_netns; /* File descriptor of global network namespace */
  char *nsname1 = "NS1", *nsname2 = "NS2";
  static char path1[MAXPATHLEN] = {0}; /* Path to the global network namespace of this process */
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
  
/* Testing function netns_change() */
  fprintf(stdout,"*** ---------------------------------\n");
  fprintf(stdout,"*** interfaces in global namespace:\n");
  system("ifconfig -a");
  
  fprintf(stdout,"*** ---------------------------------\n");
  fprintf(stdout,"*** interfaces in namespace %s:\n", nsname1);
  netns_change(nsname1, fd_global_netns);
  system("ip tuntap add mode tap tapNS1");
/* shows lo and tapNS1 interface */
  system("ifconfig -a");
  
  fprintf(stdout,"*** ---------------------------------\n");
  fprintf(stdout,"*** interfaces in global namespace:\n");
  netns_change(NULL, fd_global_netns);
  system("ifconfig -a");
  
  fprintf(stdout,"*** ---------------------------------\n");
  fprintf(stdout,"*** interfaces in namespace %s:\n", nsname2);
  netns_change(nsname2, fd_global_netns);
  system("ip tuntap add mode tap tapNS2");
/* shows lo and tapNS2 interface */
  system("ifconfig -a");
  
  fprintf(stdout,"*** ---------------------------------\n");
  fprintf(stdout,"*** interfaces in global namespace:\n");
  netns_change(NULL, fd_global_netns);
  system("ifconfig -a");
  
  close(fd_global_netns);
  return 0; 
}
