#ifndef _X_THREADCTX_H_
#define _X_THREADCTX_H_

_XBASELIB_NAMESPACE_

class ThreadContext {
	public:
		ThreadContext(){};
		virtual ~ThreadContext(){};
		virtual void run() = 0;
		virtual bool needDelete() const { return true; };
};


_XBASELIB_NAMESPACE_END_

#endif
