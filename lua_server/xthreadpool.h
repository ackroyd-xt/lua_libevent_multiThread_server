#ifndef _X_THREADPOOL_H_
#define _X_THREADPOOL_H_

#include <list>
#include <iterator>
#include <stdio.h>
#include "xthread.h"

_XBASELIB_NAMESPACE_

class Threadpool {
	public:
		Threadpool(int maxthread);
		~Threadpool();

		/* 设置最大线程个数 */
		void setMaxSize(int t) { m_maxsize = t; };
		int getMaxSize() { return m_maxsize; };

		/* 获取当前线程个数 */
		int getCurrSize(){ return m_currsize; };

		/* 设置空闲线程超时时间 */
		void setTimeout(int t) { m_timeout = t; };
		int getTimeout() { return m_timeout; };

		/* 设置最大空闲线程个数 */
		void setMaxVacant(int t){ m_maxvacant = t; };
		int getMaxVacant(){ return m_maxvacant; };

		/* 设置是否启动监视线程定时清理空闲线程 */
		void setChkVacant(int t) { m_chkvacant = t; };
		int getChkVacant() { return m_chkvacant; };

		/* 设置任务是同步执行还是异步执行 */
		void setSync(int t) { m_sync = t; };
		bool isSync() { return m_sync == 0; };

		/* todo: 状态报告get方法 
		int getCurrSize();
		int getCurrVacant();
		int getCurrTask();
		*/

		/* sync=0:阻塞  1:非阻塞(blocking queue) 默认阻塞 */
		bool add(ThreadContext* t, int sync = -1);

		bool inform(Thread* t);
		void del(Thread* t);

		/* 清理空闲线程 */
		bool flush();

		void collectAll();

		/* 启动监视线程 */
		bool startSurveillance();
		
	private:

		void init();
		int addThread(ThreadContext* t);
		int blockTask(ThreadContext* t);
		int unblockTask(ThreadContext* t);


		int lock_vacant() { return m_vacant_cond.lock(); };
		int unlock_vacant() { return m_vacant_cond.unlock(); };

	private:
		std::list<Thread*> m_list;
		std::list<Thread*> m_vacant;
		std::list<ThreadContext*> m_task;

		MutexLock m_list_lock;
		MutexCond m_vacant_cond;

		pthread_t m_monitor;
		bool m_hasmonitor;

		int m_maxsize;
		int m_currsize;
		int m_maxvacant;
		int m_timeout;
		int m_chkvacant;
		int m_sync;



};

_XBASELIB_NAMESPACE_END_

#endif
