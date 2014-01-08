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

		/* ��������̸߳��� */
		void setMaxSize(int t) { m_maxsize = t; };
		int getMaxSize() { return m_maxsize; };

		/* ��ȡ��ǰ�̸߳��� */
		int getCurrSize(){ return m_currsize; };

		/* ���ÿ����̳߳�ʱʱ�� */
		void setTimeout(int t) { m_timeout = t; };
		int getTimeout() { return m_timeout; };

		/* �����������̸߳��� */
		void setMaxVacant(int t){ m_maxvacant = t; };
		int getMaxVacant(){ return m_maxvacant; };

		/* �����Ƿ����������̶߳�ʱ��������߳� */
		void setChkVacant(int t) { m_chkvacant = t; };
		int getChkVacant() { return m_chkvacant; };

		/* ����������ͬ��ִ�л����첽ִ�� */
		void setSync(int t) { m_sync = t; };
		bool isSync() { return m_sync == 0; };

		/* todo: ״̬����get���� 
		int getCurrSize();
		int getCurrVacant();
		int getCurrTask();
		*/

		/* sync=0:����  1:������(blocking queue) Ĭ������ */
		bool add(ThreadContext* t, int sync = -1);

		bool inform(Thread* t);
		void del(Thread* t);

		/* ��������߳� */
		bool flush();

		void collectAll();

		/* ���������߳� */
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
