#ifndef _X_THREAD_H_
#define _X_THREAD_H_

#include "x_util.h"
#include "xthreadctx.h"
#include "xmutex.h"

_XBASELIB_NAMESPACE_

class Threadpool; 

class Thread {
	public:
		Thread();
		Thread(ThreadContext* t);
		~Thread();

		void init();
		int start();
		void setContext(ThreadContext* t) { m_context = t; };
		const ThreadContext * getContext() { return m_context; };

		/* ���������̳߳� */
		void setPool(Threadpool* t) { m_pool = t; };
		Threadpool* getPool() { return m_pool; };

		void setSelf(pthread_t t) { m_self = t; };  
		pthread_t getSelf() { return m_self; };

		/* ���ô��߳�Ϊ�����߳� */
		void setVacant() { m_clock = time(NULL); }; 
		time_t getVacant() { return m_clock; };

		/* ��ȡ������������ */
		long getExp() { return m_exp; }; 
		void setExit() {};

		/* �ȴ����� */
		void waitContext();	

		/* ���� */
		bool wakeUp(ThreadContext* t);

		/* �Ƿ���ɳ�ʼ�� */
		bool initDone() { return m_first == false; };

		/* ��֪�̳߳�������� */
		bool inform(); 

		/* ɾ������ */
		void deleteContext() { if(m_context) { delete m_context; m_context = NULL; } };

		void run() { if(m_context) m_context->run(); };
		
	private:
		bool trySetContext(ThreadContext* t);

	private:
		MutexCond m_cond;
		
		MutexCond m_firstcond;
		
	private:
		bool m_first;
		ThreadContext* m_context;
		pthread_t m_self;
		Threadpool* m_pool;
		time_t m_clock;
		long m_exp; /* �����������ĸ��� */

};


_XBASELIB_NAMESPACE_END_
			 
#endif
