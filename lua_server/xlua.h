#ifndef _XLUA_H_
#define _XLUA_H_

#include <errno.h>
#include "xthread.h"
#include "event2/event.h"
#include "event2/bufferevent.h"
#include "event.h"
#include <string>
using std::string;

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

_USING_XBASELIB_

#ifdef _DEBUG
#define dl(fmt, ...)  printf("[debug]"fmt, ##__VA_ARGS__)
#else
#define dl
#endif
 
/* Conn_info.flags */
#define _FLAG_CONNECTED_ 0x00000001	/*表示是否已连接*/
#define _FLAG_YIELD_	 0x00000002	/*表示coroutine是否处于yield状态*/
#define _FLAG_TIMEOUT_	 0x00000004	/*表示是否发生超时事件*/
#define _FLAG_ERROR_	 0x00000008	/*表示是否存在错误 */

/* lua return value */

/* 一个xNetContext表示一个连接处理中的某个步骤，具体步骤由lua脚本表示。对每个客户端连接，其处理可能有多个步骤，这些步骤可能由多个线程执行。进程维护一个所有coroutine状态表，当有socket事件时创建的xNetContext放入事件队列，由空闲线程获取该xNetContext，并使用lua来resume该连接的处理脚本 */

typedef int luaF(lua_State*);

luaF wait4Read;
luaF wait4Write;
luaF luaRead;
luaF luaWrite;
luaF luaClose;
luaF luaCheckConnection;
luaF wait4Timeout;
luaF luaCheckTimeout;
luaF luaUnblockWrite;

struct Conn_info{
	struct event_base* base;
	struct lua_State* lua;
	struct event* evt;
	int fd;
	int msgfd;
	int events;	//等待的事件
	int revents;//返回的事件
	int flags; 	//目前用于表示是否处于等待中\连接状态\是否超时
	struct timeval tv;		//等待超时或返回的超时剩余时间
	int ret;				//读或写的长度,或者表示lua执行结果,0表示连接终止
	int err;
};

#endif

