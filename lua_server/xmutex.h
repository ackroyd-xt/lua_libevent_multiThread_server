#ifndef _X_MUTEX_H_
#define _X_MUTEX_H_
#include <pthread.h>
#include "x_util.h"

_XBASELIB_NAMESPACE_

class MutexLock{
	public:

		MutexLock(const pthread_mutexattr_t* attr = NULL) { pthread_mutex_init(&m_mutex, attr); };

		~MutexLock() { pthread_mutex_destroy(&m_mutex); };

		int lock() { return pthread_mutex_lock(&m_mutex); };

		int unlock() { return pthread_mutex_unlock(&m_mutex); };

		int trylock() { return pthread_mutex_trylock(&m_mutex); };

	protected:

		pthread_mutex_t m_mutex;
};

class MutexCond:public MutexLock{
	public:
		MutexCond(const pthread_condattr_t* attr = NULL, const pthread_mutexattr_t* mattr = NULL):MutexLock(mattr) {
			pthread_cond_init(&m_cond, attr);
		}

		~MutexCond() { pthread_cond_destroy(&m_cond); };

		int wait(struct timespec* timeout = NULL) { 
			if(timeout == NULL)
				return pthread_cond_wait(&m_cond, &m_mutex);
			return pthread_cond_timedwait(&m_cond, &m_mutex, timeout);
		};

		/* 一般情况下，先获取锁再陷入等待 */
		int lock_wait(struct timespec* timeout = NULL) { return (lock() == 0) && wait(timeout); };

		
		/* 一般情况下，先获取锁，更改条件变量，然后唤醒各线程 */
		int signal_unlock() { return (pthread_cond_signal(&m_cond) == 0) && unlock(); };

		/* 一般情况下，先获取锁，更改条件变量，然后唤醒各线程 */
		int broadcast_unlock() { return (pthread_cond_broadcast(&m_cond) == 0) && unlock(); };
		
	private:
		pthread_cond_t m_cond;
};

class RWLock {
	public:
		RWLock(const pthread_rwlockattr_t* attr = NULL) { pthread_rwlock_init(&m_lock, attr); };
		~RWLock() { pthread_rwlock_destroy(&m_lock); };

		int rlock() { return pthread_rwlock_rdlock(&m_lock); };

		int wlock() { return pthread_rwlock_wrlock(&m_lock); };

		int try_rlock() { return pthread_rwlock_tryrdlock(&m_lock); };

		int try_wlock() { return pthread_rwlock_trywrlock(&m_lock); };

		int unlock() { return pthread_rwlock_unlock(&m_lock); };

	private:

		pthread_rwlock_t m_lock;

};

_XBASELIB_NAMESPACE_END_

#endif






	
