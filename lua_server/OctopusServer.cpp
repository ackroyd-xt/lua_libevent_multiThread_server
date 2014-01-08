#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/types.h>

#include "xthreadpool.h"
#include "xNetContext.h"

_USING_XBASELIB_

#define MAXFD 1024

//const char* server_script = "./server.lua";
const char* server_script = NULL;
struct Conn_info conns[MAXFD];
Threadpool *pool;

struct timeval tv1, tv2;
long cnt;
double t;
int g_sig;

void luaRegister(lua_State *l)
{
	lua_register(l, "wait4Read", wait4Read);
	lua_register(l, "wait4Write", wait4Write);
	lua_register(l, "luaRead", luaRead);
	lua_register(l, "luaWrite", luaWrite);
	lua_register(l, "luaClose", luaClose);
	lua_register(l, "luaCheckConnection", luaCheckConnection);
	lua_register(l, "wait4Timeout", wait4Timeout);
	lua_register(l, "luaCheckTimeout", luaCheckTimeout);
	lua_register(l, "luaUnblockWrite", luaUnblockWrite);
}

void sig(int signum)
{
	printf("signal %d received\n", signum);
	if(pool) {
		pool->collectAll();
	}
	exit(0);
}

xNetContext* BuildCtx(int fd)
{
	int t;
	lua_State **ptr =&(conns[fd].lua);
	if(*ptr == NULL) {
		if((*ptr = luaL_newstate()) == NULL) {
			printf("[fd %d] alloc lua_State fail.\n", fd);
			return NULL;
		}
		luaL_openlibs(*ptr);
		luaRegister(*ptr);

		if((t = luaL_dofile(*ptr, server_script)) != LUA_OK) {
			printf("[fd %d] lua_loadfile fail, ret=%d\n", fd, t);
			lua_close(*ptr);
			*ptr = NULL;
			return NULL;
		}
	}
	t = lua_status(*ptr);
	dl("luaState fd=%d stack=%d state=%d %s\n", fd, lua_gettop(*ptr), t, t==LUA_OK? "LUA_OK":(t==LUA_YIELD ? "LUA_YIELD":"else"));
	if(t!= LUA_OK  && t != LUA_YIELD) {
		printf("[fd %d] invalid luaState status [%d] [%s] !\n", fd, t, lua_tostring(*ptr, -1));
		lua_close(*ptr);
		*ptr = NULL;
		return NULL;
	}

	return new xNetContext(conns + fd);
}

/* 处理accept */
void socket_handler(evutil_socket_t listener, short event, void *arg)
{
	int k;
	xNetContext *ctx;
	struct event_base *base = (struct event_base *)arg;
	evutil_socket_t fd;

	if(event == EV_TIMEOUT) 
		return;

	if((fd = accept(listener, NULL, NULL)) == -1) {
		printf("accept fail, errno=%d\n", errno);
		return;
	}

	if(++cnt > 1000) {
		gettimeofday(&tv2, 0);
		t = tv2.tv_sec - tv1.tv_sec + (float(tv2.tv_usec-tv1.tv_usec))/1000000;
		printf("time_interval=%2.10f  conn=%ld, avg=%f, speed=%f\n", t, cnt, t/cnt, cnt/t);
		cnt =0;
		gettimeofday(&tv1, 0);
	}

	conns[fd].base = base;
	conns[fd].fd = fd;
	conns[fd].events = 0;
	conns[fd].revents = 0;
	conns[fd].flags = _FLAG_CONNECTED_;
	conns[fd].ret = 0;

	if(ctx = BuildCtx(fd)) {
		pool->add(ctx);
	}else {
		printf("[fd %d] Build Context fail.\n", fd);
		close(fd);
	}

}

/* 处理lua等待的事件 */
void event_handler(evutil_socket_t fd, short event, void *arg)
{
	int n, mode, t;
	char buf[1024] = {0};
	xNetContext *ctx;

	conns[fd].revents = event;
	event_del(conns[fd].evt);
	event_free(conns[fd].evt);
	conns[fd].evt = NULL;

	if(ctx = BuildCtx(fd)) {
		pool->add(ctx);
	}else {
		printf("[fd %d] build context fail\n", fd);
		close(fd);
	}
}

/* 处理lua的事件注册申请 */
void msg_handler(evutil_socket_t fd, short event, void* arg)
{
	int nfd;
	char buf[10] = {0};
	if(event != EV_READ) {
		printf("unexpected err: pipe return event %d\n", event);
		return;
	}

	read(fd, buf, 5);
	nfd = atoi(buf);
	conns[nfd].revents = 0;

	/* 只要注册了事件就表明lua非空 */
	conns[nfd].evt = event_new(conns[nfd].base, nfd, conns[nfd].events, event_handler, (void*)(conns + nfd));
	if(event_add(conns[nfd].evt, (conns[nfd].events & EV_TIMEOUT) ? &(conns[nfd].tv) : NULL)) {
		printf("[fd %d] event_add fail\n", nfd);
		lua_close(conns[nfd].lua);
		conns[nfd].lua = NULL;
		close(nfd);
	}
}

int main(int argc, char* argv[])
{
	int listener, nfd, i, k, ret, event;
	long timeout = 100;
	double ta, tb;
	long conn;
	char ip[20] = {0}, buf[8192] = {0};
	string str;
	struct sockaddr_in cdr;
	socklen_t sz = sizeof(cdr);
	struct timeval tv;
	int pipefd[2];

	if(argc < 4) {
		printf("usage: %s <port> <threadnum> <script>\n", argv[0]);
		return 0;
	}
	server_script = argv[3];

	if(pipe(pipefd)) {
		printf("pipe fail\n");
		return -1;
	}

	for(i=0; i<MAXFD; ++i) {
		conns[i].lua = NULL;
		conns[i].base = NULL;
		conns[i].evt = NULL;
		conns[i].fd = i;
		conns[i].flags = 0;
		conns[i].msgfd = pipefd[1];
		conns[i].ret = 0;
	}

	//x_sigblock(13);//SIGPIPE
	x_signal(13, SIG_IGN);
	x_signal(2, sig);
	x_signal(15, sig);

	pool = new Threadpool(atoi(argv[2]));

	if((listener= x_bind(atoi(argv[1]))) == -1) {
		printf("bind fail. errno=%d\n", errno);
		return 0;
	}
	listen(listener, 150);

	//event_enable_debug_mode();
	evutil_make_listen_socket_reuseable(listener);
	//evutil_make_socket_nonblocking(listener);//todo: 好像这里设置非阻塞的话accept后的连接都是非阻塞的
	struct event_base *base = event_base_new();
	struct event *soc_evt, *msg_evt;

	tv.tv_sec = 1;
	tv.tv_usec = 0;

	soc_evt = event_new(base, listener, EV_READ|EV_PERSIST, socket_handler, (void*)base);
	if(event_add(soc_evt, &tv)) {
		printf("event_add socket_handler fail\n");
		return -1;
	}

	msg_evt = event_new(base, pipefd[0], EV_READ|EV_PERSIST, msg_handler, NULL);
	if(event_add(msg_evt, NULL)) {
		printf("event_add message_handler fail\n");
		return -1;
	}
	
	gettimeofday(&tv1, 0);
	event_base_dispatch(base);

	printf("The End.");
	delete pool;
	return 0;

}





