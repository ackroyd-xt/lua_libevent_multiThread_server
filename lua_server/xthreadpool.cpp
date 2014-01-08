#include <unistd.h>

#include "xthreadpool.h"

_XBASELIB_NAMESPACE_

void* surveillance(void* t)
{
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
	Threadpool* p = (Threadpool*)t;
	while(1) {
		pthread_testcancel();/* 添加取消点 */
		if(!p->flush()) {
			return NULL;
		}
		pthread_testcancel();/* 添加取消点 */
		sleep(p->getChkVacant());
	}
	return NULL;
}

void Threadpool::init()
{
	m_maxsize = 5;
	m_currsize = 0;
	m_timeout = -1;
	m_maxvacant = -1;
	m_sync = 0;
	m_chkvacant = -1;
	m_hasmonitor = false;
	m_list.clear();
}

Threadpool::Threadpool(int maxthread)
{
	init();
	m_maxsize = maxthread;
}

Threadpool::~Threadpool()
{
	int k;
	collectAll();
	if(m_hasmonitor) {
		if((k = pthread_cancel(m_monitor)))
			return;
		if((k = pthread_join(m_monitor, NULL)))
			return;
	}

}

void Threadpool::collectAll()
{
	int sz, asz;
	std::list<Thread*>::iterator ite;

	/* 进入此流程后，不再接收业务请求 */
	m_list_lock.lock();
	asz = m_list.size();
	m_list_lock.unlock();

	m_vacant_cond.lock();
	while(1) {
		m_list_lock.lock();
		asz = m_list.size();
		m_list_lock.unlock();
		if(m_vacant.size() != asz) {
			m_vacant_cond.wait();
		}else
			break;
	}
	m_vacant.clear();
	m_vacant_cond.unlock();

	m_list_lock.lock();
	printf("********************************************\n");
	sz = 0;
	for(ite = m_list.begin(); ite != m_list.end(); ++ite) {
		printf("thread [%d] exp = %ld\n", (*ite)->getSelf(), (*ite)->getExp());
		sz += (*ite)->getExp();
		/*此处停止线程*/
		if(!((*ite)->wakeUp(NULL))) {
			printf("close thread [%d] fail.\n", (*ite)->getSelf());
		}
		pthread_join((*ite)->getSelf(), NULL);
		delete *ite;
	}
	printf("********************************************\n");
	printf("total exp = %d\n", sz);
	printf("********************************************\n");
	m_list.clear();
	/* 如果正处于接受业务请求的流程中，则由于每次
	 * 接收请求均会调用flush(), 其他业务线程会更改
	 * m_currsize 的值；仅当退出此流程后，其他线程
	 * 不再自动更改此值，也就实质上不需要锁。
	 */
	m_currsize = 0;
	m_list_lock.unlock();


}

int Threadpool::addThread(ThreadContext* t)
{
	int k;
	Thread* thd;
	thd = new Thread(t);
	thd->setPool(this);
	if(k = thd->start()) {
		printf("thread start fail:%d\n", k);
		return k;
	}
	m_list_lock.lock();
	m_list.push_back(thd);
	m_currsize += 1;
	m_list_lock.unlock();

	return 0;
}

int Threadpool::blockTask(ThreadContext* t)
{
	m_vacant_cond.lock();
	if(m_vacant.size() == 0) {
		m_vacant_cond.wait();
	}

	//printf("now wakeUp thread %d\n", m_vacant.front()->getSelf());
	if(m_vacant.front()->wakeUp(t)) {
		m_vacant.pop_front();
		m_vacant_cond.unlock();
	}else {
		//获取任务失败
		m_vacant_cond.unlock();
		printf("wakeUp thread fail.\n");
		return 1;
	}
	return 0;
}

int Threadpool::unblockTask(ThreadContext* t)
{
	m_vacant_cond.lock();
	if(m_vacant.size() > 0) {
		if(m_vacant.front()->wakeUp(t)) {
			m_vacant.pop_front();
			m_vacant_cond.unlock();
			return 0;
		}else {
			m_vacant_cond.unlock();
			printf("wakeUp thread fail.\n");
			return 1;
		}
	}

	m_task.push_back(t);
	m_vacant_cond.unlock();
	return 0;
}

bool Threadpool::add(ThreadContext* t, int sync)
{
	int k;

	/* 业务线程调用del()只能减少m_currsize, 也就是说
	 * m_currsize只会越来越小，因此此处取值不需要lock
	 */

	if(m_currsize < m_maxsize) {
		//printf("creating new thread.\n");
		return (addThread(t) == 0);
	}

	/*所有线程均已启用*/

	if((sync==0)||((sync==-1)&&(m_sync==0))) {
		if(blockTask(t))
			return false;
	}else{
		if(unblockTask(t))
			return false;
	}

	//return flush();
	return true;

}

void Threadpool::del(Thread* t)
{
	/* 业务线程自动更改 m_currsize, 需要锁*/
	m_list_lock.lock();
	m_list.remove(t);
	m_currsize -= 1;
	m_list_lock.unlock();
}

bool Threadpool::inform(Thread* t) 
{
	int k;
	/* m_list仅在创建线程时维护 */
	m_vacant_cond.lock();
	if(m_task.size() > 0) {
		t->setContext(m_task.front());
		m_task.pop_front();
		m_vacant_cond.unlock();
		return true;
	}
		
	t->setVacant();
	m_vacant.push_back(t);

	/* 若主线程在等待空闲线程，则唤醒主线程 */
	m_vacant_cond.signal_unlock();
	return true;
}

bool Threadpool::flush()
{
	int cnt = 0, vacant;
	std::list<Thread*>::iterator ite, itf;
	time_t t = time(NULL);


	m_vacant_cond.lock();

	if(m_timeout != -1) {
		for(ite = m_vacant.begin(); ite != m_vacant.end();) {
			if(t - (*ite)->getVacant() > m_timeout) {
				if((*ite)->wakeUp(NULL) == false) {
					m_vacant_cond.unlock();
					return false;
				}
				pthread_join((*ite)->getSelf(), NULL);
				itf = m_vacant.erase(ite);
				del(*ite);
				ite = itf;
				++cnt;
			}else {
				/*m_vacant是先进先出*/
				break;
			}
		}
	}

	vacant = m_vacant.size() - m_maxvacant;;
	if(m_maxvacant < 0)
		vacant = -1;

	for(ite = m_vacant.begin(); (ite != m_vacant.end()) &&(vacant >0);) {
		if((*ite)->wakeUp(NULL) == false) {
			m_vacant_cond.unlock();
			return false;
		}
		pthread_join((*ite)->getSelf(), NULL);
		itf = m_vacant.erase(ite);
		del(*ite);
		ite = itf;
		++cnt;
		--vacant;
	}
	m_vacant_cond.unlock();

	if(cnt > 0) 
		printf(" flush: %d thread timeout.\n", cnt);

	return true;
}

bool Threadpool::startSurveillance()
{
	int k;
	if(m_chkvacant == -1)
		return true;

	if((k = pthread_create(&m_monitor, NULL, surveillance, this))) {
		printf("pthread_create fail:%d\n", k);
		return false;
	}
	m_hasmonitor = true;
	return true;
}


_XBASELIB_NAMESPACE_END_





