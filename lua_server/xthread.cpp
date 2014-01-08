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
			//��context�Ļ��ѱ�ʾ��ֹ
			break;
		}
		thd->run();
		if(thd->getContext()->needDelete()) 
			thd->deleteContext();
		thd->setContext(NULL);
		thd->inform();
	}

	//�߳��˳�
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
	/* �½��̣߳���һ�ε���ʱ��δ�������̳߳أ����ᱻ������
	 * ���ǵ�һ�ε��ã���ÿ�δ�pthread_cond_wait��������ѻ���� 
	 * �ڱ����Ѻ�Ĵ����о�δ�ͷ���
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

	/* ������󣬵���pthread_cond_waitԭ�ӵ��ͷ����������̲߳���ȴ�����
	 * ��������ʱ�ٴλ������������ҵ����
	 */
	m_cond.wait();

}

bool Thread::trySetContext(ThreadContext* t)
{
	int k;
	/* �����̵߳��������У���������ʱ�����δ��ռ�ã�
	 * 1. �̱߳���������δ�����̳߳�
	 * 2. �̵߳���pthread_cond_wait����
	 * �˴γ��Ի�ȡ�����ڵڶ���ʱ���ʱִ�У���δ�������ʾ
	 * ������������ռ���̣߳��򷵻�ʧ��
	 */

	/*����Ϊʲôtrylock��ʧ�ܣ�
	 * -----��Ϊ�߳����ڵ���inform�����̷߳ŵ�����
	 *  �̶߳���m_vacant���ٵ���pthread_cond_wait
	 *  �ͷ��������ߵ�;����ڲ�����к��ͷ���֮����
	 *  �̸߳����̷߳�������������޷���ȡ����ʧ�ܡ�
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

	/* ���ѻ�ȡ��������context��Ȼ�����̣߳��̻߳����޷���ȡ��������
	 * ���Ѻ�����������̻߳��ȡ��������ʼҵ����
	 * ����ȡ���󡢽���ǰ������������Ҫ��ȡ�������ڵ��õ���pthread_mutex_trylock,����������ʧ�ܣ�
	 * �������Ա�֤������ֻ���̻߳�ȡ����
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
