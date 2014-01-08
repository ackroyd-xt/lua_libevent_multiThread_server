#ifndef _X_NETCONTEXT_
#define _X_NETCONTEXT_

#include "xlua.h"

class xNetContext:public ThreadContext {
	public:
		xNetContext(struct Conn_info* conn) { m_conn = conn; };
		~xNetContext(){};
		void run();// lua_resume
		bool needDelete() const { return true; };	
	private:
		struct Conn_info* m_conn;
};

void xNetContext::run()
{
	int t;
	char buf[10] = {0};

	lua_getglobal(m_conn->lua, "server");
	lua_pushlightuserdata(m_conn->lua, m_conn); /*将Conn_info指针入栈*/
	t = lua_resume(m_conn->lua, NULL, 1); /*无论连接处于何种状态都要通知lua*/

	/* lua返回结果处理 */
	switch(t) {
		case LUA_OK:	/* 无论如何都关闭连接 */
			if(m_conn->ret != 0) {
				printf("[fd %d] finish, ret=%d errno=%d, closing connection\n", m_conn->fd, m_conn->ret, m_conn->err);
			}else {
				dl("[fd %d] job done, closing connection.\n", m_conn->fd);
			}

			lua_close(m_conn->lua);
			m_conn->lua = NULL;
			m_conn->events = m_conn->revents = 0;
			m_conn->ret = m_conn->err = 0;
			/* 注意：必须先删除lua_State再关闭描述符，否则可能出现子线程关闭描述符、主线程接受请求打开描述符、
			 * 主线程分发请求、子线程删除lua_State、子线程接收到lua_State=NULL的请求  这种情况 */
			close(m_conn->fd);
			break;
		case LUA_YIELD:	/*yield不应有任何返回值*/
			m_conn->flags |= _FLAG_YIELD_;

			if(m_conn->ret != 0) {	/* 出错 */
				printf("[fd %d] job fail, ret=%d errno=%d, closing connection.\n", m_conn->fd, m_conn->ret, m_conn->err);
				lua_close(m_conn->lua);
				m_conn->lua = NULL;
				close(m_conn->fd); 
			}else if((m_conn->flags & _FLAG_CONNECTED_) == 0) { /* 正常关闭 */
				m_conn->events = m_conn->revents = 0;
				m_conn->ret = m_conn->err = 0;
				close(m_conn->fd);
			}else if(m_conn->events) {	/* 处理未完成，等待下一个事件 */
				m_conn->revents = 0;
				sprintf(buf, "%05d", m_conn->fd);
				write(m_conn->msgfd, buf, 5);//注册事件
			}
			break;
		default:
			printf("[fd %d] job returns status %d #%s#, closing connection.\n", m_conn->fd, t, lua_tostring(m_conn->lua, -1));
			lua_close(m_conn->lua);
			m_conn->lua = NULL;
			close(m_conn->fd); /* 必须先删除lua_State再关闭描述符 */
	}
}



#endif

