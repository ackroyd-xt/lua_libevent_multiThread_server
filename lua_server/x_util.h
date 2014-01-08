#ifndef _X_UTIL_
#define _X_UTIL_

#include <unistd.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define _XBASELIB_NAMESPACE_ namespace xBaseLib{
#define _XBASELIB_NAMESPACE_END_ }
#define _USING_XBASELIB_ using namespace xBaseLib;

_XBASELIB_NAMESPACE_

double x_time();

void x_sleep(double k);

void x_daemonlize(const char* filename);

int x_connect(const char* ip, int port);

int x_bind(int port);

void x_signal(int signal, void (*f)(int));

void x_sigblock(int sig);

void x_sigunblock(int sig);

_XBASELIB_NAMESPACE_END_

#endif

