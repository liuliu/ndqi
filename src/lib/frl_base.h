#ifndef GUARD_frl_base_h
#define GUARD_frl_base_h

#include "apr.h"
#include "apr_portable.h"
#include "apr_atomic.h"
#include "apr_thread_rwlock.h"
#include "apr_thread_mutex.h"
#include "apr_thread_cond.h"
#include "apr_poll.h"
#include "apr_queue.h"

#define FRL_PROGRESS_INTERRUPT (APR_EOF)
#define FRL_PROGRESS_COMLETE (APR_SUCCESS)
#define FRL_PROGRESS_WAIT_SIGNAL (APR_INCOMPLETE)
#define FRL_PROGRESS_CONTINUE (APR_TIMEUP)
#define FRL_PROGRESS_RESTART (APR_EINIT)
#define FRL_PROGRESS_IS_INTERRUPT(status) (FRL_PROGRESS_INTERRUPT == status)
#define FRL_PROGRESS_IS_COMLETE(status) (FRL_PROGRESS_COMLETE == status)
#define FRL_PROGRESS_IS_WAIT_SIGNAL(status) (FRL_PROGRESS_WAIT_SIGNAL == status)
#define FRL_PROGRESS_IS_CONTINUE(status) (FRL_PROGRESS_CONTINUE == status)
#define FRL_PROGRESS_IS_RESTART(status) (FRL_PROGRESS_RESTART == status)

enum frl_lock_u
{
	FRL_LOCK_WITH,
	FRL_LOCK_FREE
};

enum frl_insert_u
{
	FRL_INSERT_BEFORE,
	FRL_INSERT_AFTER
};

enum frl_memory_u
{
	FRL_MEMORY_SELF,
	FRL_MEMORY_GLOBAL
};

enum frl_thread_model_u
{
	FRL_THREAD_LEADER_FOLLOWER,
	FRL_THREAD_CONSUMER_PRODUCER
};

enum frl_level_u
{
	FRL_LEVEL_ERROR,
	FRL_LEVEL_WARNING,
	FRL_LEVEL_INFO
};

const apr_uint32_t SIZEOF_POINTER = sizeof(void*);
const apr_uint32_t SIZEOF_APR_UINT32_T = sizeof(apr_uint32_t);

#endif
