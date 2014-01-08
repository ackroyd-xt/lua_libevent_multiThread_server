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
#define _FLAG_CONNECTED_ 0x00000001	/*��ʾ�Ƿ�������*/
#define _FLAG_YIELD_	 0x00000002	/*��ʾcoroutine�Ƿ���yield״̬*/
#define _FLAG_TIMEOUT_	 0x00000004	/*��ʾ�Ƿ�����ʱ�¼�*/
#define _FLAG_ERROR_	 0x00000008	/*��ʾ�Ƿ���ڴ��� */

/* lua return value */

/* һ��xNetContext��ʾһ�����Ӵ����е�ĳ�����裬���岽����lua�ű���ʾ����ÿ���ͻ������ӣ��䴦������ж�����裬��Щ��������ɶ���߳�ִ�С�����ά��һ������coroutine״̬������socket�¼�ʱ������xNetContext�����¼����У��ɿ����̻߳�ȡ��xNetContext����ʹ��lua��resume�����ӵĴ���ű� */

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
	int events;	//�ȴ����¼�
	int revents;//���ص��¼�
	int flags; 	//Ŀǰ���ڱ�ʾ�Ƿ��ڵȴ���\����״̬\�Ƿ�ʱ
	struct timeval tv;		//�ȴ���ʱ�򷵻صĳ�ʱʣ��ʱ��
	int ret;				//����д�ĳ���,���߱�ʾluaִ�н��,0��ʾ������ֹ
	int err;
};

#endif

