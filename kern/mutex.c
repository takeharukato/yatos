/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  mutex routines                                                    */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/kern_types.h>
#include <kern/assert.h>
#include <kern/kprintf.h>
#include <kern/string.h>
#include <kern/errno.h>
#include <kern/spinlock.h>
#include <kern/thread.h>
#include <kern/thread-sync.h>
#include <kern/thread-sync.h>
#include <kern/sched.h>
#include <kern/mutex.h>

void
mutex_init(mutex *mtx, mutex_flags mtx_flags){

	kassert( mtx != NULL );

	spinlock_init( &mtx->lock );
	sync_init_object( &mtx->mutex_waiter, SYNC_WAKE_FLAG_ALL, THR_TSTATE_WAIT);
	mtx->counter = 0;
	mtx->owner = NULL;
	mtx->mtx_flags = mtx_flags;
}

void
mutex_destroy(mutex *mtx){

	kassert( mtx != NULL );
	sync_wake( &mtx->mutex_waiter, SYNC_OBJ_DESTROYED);
	mtx->counter = 0;
	mtx->owner = NULL;
}

bool
mutex_locked_by_self(mutex *mtx){
	intrflags flags;
	bool         rc;
	
	spinlock_lock_disable_intr( &mtx->lock , &flags );
	rc = ( mtx->owner == current );
	spinlock_unlock_restore_intr( &mtx->lock, &flags);

	return rc;
}

bool
mutex_lock(mutex *mtx){
	intrflags flags;
	sync_reason rc;
	bool       ret;

	kassert( mtx != NULL );

	spinlock_lock_disable_intr( &mtx->lock , &flags );
	while(1) {
		if ( mtx->counter == 0 ) {

			++mtx->counter;
			mtx->owner = current;
			goto success_out;
		}

		if ( mtx->counter > 0 ){
		
			if ( ( mtx->owner == current ) &&
			    (mtx->mtx_flags & MTX_FLAG_RECURSIVE ) ) {
				++mtx->counter;
				goto success_out;
			}

			rc = sync_wait(&mtx->mutex_waiter, &mtx->lock);
			if ( rc == SYNC_OBJ_DESTROYED ) {

				ret = false;
				goto unlock_out;
			}
		}
	}

success_out:
	ret = true;

unlock_out:
	spinlock_unlock_restore_intr(&mtx->lock, &flags);

	return ret;
}

void
mutex_unlock(mutex *mtx) {
	intrflags flags;

	kassert( mtx != NULL );

	spinlock_lock_disable_intr( &mtx->lock , &flags );

	kassert( mtx->counter > 0);
	kassert( mtx->owner == current );
	--mtx->counter;
	if ( mtx->counter == 0 ) {
		
		mtx->owner = NULL;
		sync_wake( &mtx->mutex_waiter, SYNC_WAI_RELEASED);
	}

	spinlock_unlock_restore_intr( &mtx->lock, &flags);

	return;
}

