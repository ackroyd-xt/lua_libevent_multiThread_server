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
	lua_pushlightuserdata(m_conn->lua, m_conn); /*��Conn_infoָ����ջ*/
	t = lua_resume(m_conn->lua, NULL, 1); /*�������Ӵ��ں���״̬��Ҫ֪ͨlua*/

	/* lua���ؽ������ */
	switch(t) {
		case LUA_OK:	/* ������ζ��ر����� */
			if(m_conn->ret != 0) {
				printf("[fd %d] finish, ret=%d errno=%d, closing connection\n", m_conn->fd, m_conn->ret, m_conn->err);
			}else {
				dl("[fd %d] job done, closing connection.\n", m_conn->fd);
			}

			lua_close(m_conn->lua);
			m_conn->lua = NULL;
			m_conn->events = m_conn->revents = 0;
			m_conn->ret = m_conn->err = 0;
			/* ע�⣺������ɾ��lua_State�ٹر���������������ܳ������̹߳ر������������߳̽����������������
			 * ���̷ַ߳��������߳�ɾ��lua_State�����߳̽��յ�lua_State=NULL������  ������� */
			close(m_conn->fd);
			break;
		case LUA_YIELD:	/*yield��Ӧ���κη���ֵ*/
			m_conn->flags |= _FLAG_YIELD_;

			if(m_conn->ret != 0) {	/* ���� */
				printf("[fd %d] job fail, ret=%d errno=%d, closing connection.\n", m_conn->fd, m_conn->ret, m_conn->err);
				lua_close(m_conn->lua);
				m_conn->lua = NULL;
				close(m_conn->fd); 
			}else if((m_conn->flags & _FLAG_CONNECTED_) == 0) { /* �����ر� */
				m_conn->events = m_conn->revents = 0;
				m_conn->ret = m_conn->err = 0;
				close(m_conn->fd);
			}else if(m_conn->events) {	/* ����δ��ɣ��ȴ���һ���¼� */
				m_conn->revents = 0;
				sprintf(buf, "%05d", m_conn->fd);
				write(m_conn->msgfd, buf, 5);//ע���¼�
			}
			break;
		default:
			printf("[fd %d] job returns status %d #%s#, closing connection.\n", m_conn->fd, t, lua_tostring(m_conn->lua, -1));
			lua_close(m_conn->lua);
			m_conn->lua = NULL;
			close(m_conn->fd); /* ������ɾ��lua_State�ٹر������� */
	}
}



#endif

