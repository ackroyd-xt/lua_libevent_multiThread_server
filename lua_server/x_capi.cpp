#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "xlua.h"

//wait4Read(info, timeout)
int wait4Read(lua_State *L)
{
	struct Conn_info *info = (struct Conn_info*)lua_touserdata(L, 1);
	int timeout = (int)lua_tonumber(L, 2);

	if(!info)
		return 0;

	info->events = EV_READ;
	if(timeout > 0) {
		info->tv.tv_sec = timeout/1000000;
		info->tv.tv_usec = timeout%1000000;
		info->events |= EV_TIMEOUT;
	}

	return 0;
}

//wait4Write(info, timeout)
int wait4Write(lua_State *L)
{
	struct Conn_info *info = (struct Conn_info*)lua_touserdata(L, 1);
	int timeout = (int)lua_tonumber(L, 2);
	
	if(!info)
		return 0;

	info->events = EV_WRITE;
	if(timeout > 0) {
		info->tv.tv_sec = timeout/1000000;
		info->tv.tv_usec = timeout%1000000;
		info->events |= EV_TIMEOUT;
	}
	
	return 0;
}

//luaRead(info)
int luaRead(lua_State *L)
{
	int tmp;
	char buf[1024] = {0};
	struct Conn_info *info = (struct Conn_info*)lua_touserdata(L, 1);
	if(!info)
		return 0;

	errno = 0;
	do {
		tmp = read(info->fd, buf, sizeof(buf)-1);
	}while (errno == EINTR);

	info->err = errno;
	if(tmp <= 0)
		info->flags &= ~_FLAG_CONNECTED_;
	info->ret = (tmp<0) ? (errno == ECONNRESET ? info->ret : tmp) : (info->ret);

	lua_pushstring(L, buf);
	return 1;
}

//luaWrite(info, str)
int luaWrite(lua_State *L)
{
	int tmp;
	struct Conn_info *info = (struct Conn_info*)lua_touserdata(L, 1);
	char *x = (char*)lua_tostring(L, 2);

	errno = 0;
	do {
		tmp = write(info->fd, x, strlen(x));
	}while (errno == EINTR);

	info->err = errno;
	if(errno == EPIPE)
		info->flags &= ~_FLAG_CONNECTED_;
	info->ret = (tmp < 0) ? (errno == EPIPE ? info->ret : tmp) : (info->ret);

	lua_pushnumber(L, tmp);
	return 1;
}

//luaClose(info)
int luaClose(lua_State *L)
{
	struct Conn_info *info = (struct Conn_info*)lua_touserdata(L, 1);
	info->events = 0;
	info->revents = 0;	/* 注意这里要清除，否则下个相同fd的连接可能接收到过期的事件 */
	info->flags &= ~_FLAG_CONNECTED_;

	info->ret = 0;
	info->err = 0;
	return 0;
}

//luaCheckConnection(info)
int luaCheckConnection(lua_State *L)
{
	struct Conn_info *info = (struct Conn_info*)lua_touserdata(L, 1);
	lua_pushnumber(L, (info->flags & _FLAG_CONNECTED_) ? 1:0);
	return 1;
}

//wait4Timeout(info, timeout)
int wait4Timeout(lua_State *L)
{
	struct Conn_info *info = (struct Conn_info*)lua_touserdata(L, 1);
	int timeout = (int)lua_tonumber(L, 2);

	if(!info)
		return 0;
	info->events = EV_TIMEOUT;
	info->tv.tv_sec = timeout/1000000;
	info->tv.tv_usec = timeout%1000000;
	return 0;
}

//luaCheckTimeout(info)
int luaCheckTimeout(lua_State *L)
{
	struct Conn_info *info = (struct Conn_info*)lua_touserdata(L, 1);
	lua_pushnumber(L, ((info->revents) & EV_TIMEOUT) ? 1:0);
	return 1;
}

int luaUnblockWrite(lua_State *L)
{
	int mode, ret;
	struct Conn_info *info = (struct Conn_info*)lua_touserdata(L, 1);
	char *x = (char*)lua_tostring(L, 2);

	mode = fcntl(info->fd, F_GETFL);
	fcntl(info->fd, F_SETFL, mode|O_NONBLOCK);

	do {
		ret = write(info->fd, x, strlen(x));
	}while (errno == EINTR);

	info->err = errno;
	if(errno == EPIPE)
		info->flags &= ~_FLAG_CONNECTED_;
	info->ret = (ret < 0) ? (errno == EPIPE ? info->ret : ret) : (info->ret);

	fcntl(info->fd, F_SETFL, mode);
	lua_pushnumber(L, ((ret < 0) && (errno == EAGAIN)) ? 0 : ret);

	return 1;
}





