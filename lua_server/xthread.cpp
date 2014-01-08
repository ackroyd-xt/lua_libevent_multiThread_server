#include "xthread.h"
#include "xthreadpool.h"

_XBASELIB_NAMESPACE_

void* process(void* t)
{
	Thread* thd = (Thread*)t;
	thd->setSelf(pthread_self());

	pthread_detach(pthread_self());

	sigset_t set, oset;

	pthread_sigmask(0, NULL, &set);
	sigaddset(&set, SIGTERM);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTSTP);
	pthread_sigmask(SIG_BLOCK, &set, &oset);

	while(1) {
		thd->waitContext();
		if(thd->getContext() == NULL) {
			//无context的唤醒表示终止
			break;
		}
		thd->run();
		if(thd->getContext()->needDelete()) 
			thd->deleteContext();
		thd->setContext(NULL);
		thd->inform();
	}

	//线程退出
	thd->setExit();
	
	return 0;
}

void Thread::init()
{
	m_first = true;
	m_exp = 0;

	m_firstcond.lock();
}
	
Thread::Thread()
{
	init();
	m_context = NULL;
}

Thread::Thread(ThreadContext* t)
{
	init();
	m_context = t;
}

Thread::~Thread()
{
	if(m_context && (m_context->needDelete()))
		delete m_context;
}

bool Thread::inform()
{
	if(!m_pool)
		return false;
	
	return m_pool->inform(this);
}

void Thread::waitContext()
{
	/* 新建线程，第一次调用时尚未被放入线程池，不会被抢夺锁
	 * 若非第一次调用，则每次从pthread_cond_wait醒来后均已获得锁 
	 * 在被唤醒后的处理中均未释放锁
	 */
	int k;

	if(m_first) {
		m_cond.lock();
		m_firstcond.lock();
		m_first = false;
		m_firstcond.signal_unlock();

	}

	if(m_context) {
		++m_exp;
		return;
	}

	/* 获得锁后，调用pthread_cond_wait原子的释放锁、将本线程插入等待队列
	 * 当被唤醒时再次获得锁，并进入业务处理
	 */
	m_cond.wait();

}

bool Thread::trySetContext(ThreadContext* t)
{
	int k;
	/* 整个线程的生命期中，仅在两个时间段锁未被占用：
	 * 1. 线程被创建但还未放入线程池
	 * 2. 线程调用pthread_cond_wait休眠
	 * 此次尝试获取锁是在第二个时间段时执行，若未获得所表示
	 * 其他任务已抢占该线程，则返回失败
	 */

	/*分析为什么trylock会失败？
	 * -----因为线程是在调用inform将本线程放到空闲
	 *  线程队列m_vacant后，再调用pthread_cond_wait
	 *  释放锁并休眠的;如果在插入队列和释放锁之间主
	 *  线程给该线程分配任务，则会因无法获取锁而失败。
	 */
	/*
	if((k = pthread_mutex_trylock(&m_mutex)) != 0) {
		printf("pthread_mutex_trylock thread %d fail:%d.\n", getSelf(), k);
		return false;
	}
	*/

	if(k = m_cond.lock()) {
		printf("pthread_mutex_lock fail:%d\n", k);
		return false;
	}

	setContext(t);
	++m_exp;
	return true;
}

bool Thread::wakeUp(ThreadContext* t) 
{
	if(!trySetContext(t))
		return false;

	/* 锁已获取并设置了context，然后唤醒线程，线程会因无法获取锁而阻塞
	 * 唤醒后解锁，于是线程会获取到锁并开始业务处理
	 * 若获取锁后、解锁前有其他任务想要获取锁，由于调用的是pthread_mutex_trylock,会立即返回失败，
	 * 这样可以保证解锁后只有线程获取到锁
	 */
	m_cond.signal_unlock();

	return true;
}

int Thread::start()
{
	int k;
	pthread_t pth;
	if(k = pthread_create(&pth, NULL, process, this)) {
		printf("pthread_create fail:%d\n", k);
		return k;
	}
	m_firstcond.wait();

	return 0;
}

_XBASELIB_NAMESPACE_END_
