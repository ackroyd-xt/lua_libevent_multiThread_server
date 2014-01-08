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

		/* 设置所属线程池 */
		void setPool(Threadpool* t) { m_pool = t; };
		Threadpool* getPool() { return m_pool; };

		void setSelf(pthread_t t) { m_self = t; };  
		pthread_t getSelf() { return m_self; };

		/* 设置此线程为空闲线程 */
		void setVacant() { m_clock = time(NULL); }; 
		time_t getVacant() { return m_clock; };

		/* 获取处理的任务个数 */
		long getExp() { return m_exp; }; 
		void setExit() {};

		/* 等待任务 */
		void waitContext();	

		/* 唤醒 */
		bool wakeUp(ThreadContext* t);

		/* 是否完成初始化 */
		bool initDone() { return m_first == false; };

		/* 告知线程池任务完成 */
		bool inform(); 

		/* 删除任务 */
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
		long m_exp; /* 处理的任务包的个数 */

};


_XBASELIB_NAMESPACE_END_
			 
#endif
