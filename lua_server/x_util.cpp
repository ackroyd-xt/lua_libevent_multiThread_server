#include "x_util.h"

_XBASELIB_NAMESPACE_

double x_time()
{
	struct timeval tv;
	gettimeofday(&tv,0);
	return tv.tv_sec + (double)tv.tv_usec/1000000;
}

void x_sleep(double k)
{
	struct timeval tv;
	tv.tv_sec = (int)k;
	tv.tv_usec = (k - tv.tv_sec)*1000000;
	select(0,0,0,0,&tv);
}

void x_daemonlize(const char* filename)
{
	int fd;
	if(filename[0]==0){
		printf("filename invalid\n");
		exit(0);
	}
	if(fork())
		exit(0) ;
	setsid() ;
	struct sigaction act ;
	act.sa_handler = SIG_IGN ;
	sigaction(SIGHUP,&act,NULL) ;
	if(fork())
		exit(0) ;
	if((fd = open(filename,O_RDWR|O_CREAT|O_APPEND|O_EXCL, 0644))==-1)
		if((fd = open(filename, O_RDWR|O_APPEND))==-1) {
			printf("cannot open/create %s.\n",filename);
			exit(0);
		}
	dup2(fd, STDIN_FILENO);
	dup2(fd, STDOUT_FILENO);
	dup2(fd, STDERR_FILENO);
	FILE* fl = fdopen(fd, "w");
	setlinebuf(fl);
}

int x_connect(const char* ip, int port)
{
	int fd ;
	double delay;
	struct timeval tv1, tv2;
	if((fd=socket(AF_INET, SOCK_STREAM, 0))==-1)
		return -1;
	struct sockaddr_in adr ;
	if(inet_pton(AF_INET, ip, &((adr.sin_addr).s_addr))!=1)
		return -1;
	adr.sin_port = htons(port) ;
	adr.sin_family = AF_INET ;
	gettimeofday(&tv1, 0);
	if(connect(fd, (struct sockaddr*)&adr, sizeof(adr))==-1) {
		printf("connect fail, errno=%d\n", errno);
		if(errno == EAGAIN) {
			gettimeofday(&tv2, 0);
			delay = tv2.tv_sec - tv1.tv_sec + (float(tv2.tv_usec-tv1.tv_usec))/1000000;
			printf("connect fail at EAGAIN, delay=%f\n", delay);
		}else
			exit(0);
		return -1;
	}
	return fd;
}

int x_bind(int port)
{
	int sfd;
	struct sockaddr_in sdr;

	sfd = socket(AF_INET, SOCK_STREAM, 0);
	sdr.sin_port = htons(port);
	sdr.sin_addr.s_addr  =htonl(INADDR_ANY);
	sdr.sin_family = AF_INET;

	return (bind(sfd, (struct sockaddr*)&sdr, sizeof(sdr))==0) ? sfd : -1;
}

void x_signal(int sig, void (*f)(int))
{
	struct sigaction act, oact;
	act.sa_flags=0;
	act.sa_handler = f;
	sigaction(sig, &act, &oact);
}

void x_sigblock(int sig)
{
	sigset_t set, oset;
	sigprocmask(0, NULL, &oset);
	set = oset;
	sigaddset(&set, sig);
	sigprocmask(SIG_BLOCK, &set, &oset);
}

void x_sigunblock(int sig)
{
	sigset_t set, oset;
	sigprocmask(0, NULL, &oset);
	set = oset;
	sigdelset(&set, sig);
	sigprocmask(SIG_BLOCK, &set, &oset);
}

_XBASELIB_NAMESPACE_END_

