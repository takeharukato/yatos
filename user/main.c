
//#define CRASH_ME

#if defined(CRASH_ME)
#include <stdint.h>
#endif  /*  CRASH_ME  */

#include <ulib/libyatos.h>
#include <hal/rdtsc.h>

#define LOOP_TSC (2000000000ULL)

static int data_bss;
static int data=0x8000;

int
new_thread(void *arg) {
	uint64_t  tsc1, tsc2;

	yatos_printf("[%d]: new_thread(arg=%p)\n",
	    yatos_thread_getid(), arg);
	yatos_printf("[%d]: consume user cpu resources, please wait\n", yatos_thread_getid());
	tsc1 = rdtsc();
	do{
		tsc2 = rdtsc();
	}while( (tsc2 - tsc1) < (uint64_t)( 5*LOOP_TSC ));
	yatos_printf("[%d]: return 0\n",
	    yatos_thread_getid(), arg);

	return 0;
}

void
user_handler(event_id  id, evinfo *inf, void *ctx) {
	
	yatos_printf("[%d]: user_handler(%d, %p, %p), data=%p and return\n",
	    yatos_thread_getid(), id, inf, ctx, inf->data);
}

void
user_stack_test(void) {
	uint64_t array[8*1024];

	memset(&array[0], 0, sizeof(array));

	return;
}

int
main(int argc, char *argv[]){
	int                i;
	int               rc;
	double          fval;
	void       *old_heap;
	tid            newid;
	tid           chldid;
	exit_code       code;
	uint64_t  tsc1, tsc2;
	thread_resource tres;

#if defined(CRASH_ME)
	uintptr_t *crash_p = (uintptr_t *)main;
#endif  /*  CRASH_ME  */

	for(i = 0; argc > i; ++i) 
		yatos_printf("[%d]: %d: argv[%d]: %s\n", 
		    yatos_thread_getid(), i, i, argv[i]);

	for(i = 0; environ[i] != NULL; ++i) 
		yatos_printf("[%d]: %d: environ[%d]: %s\n", 
		    yatos_thread_getid(), i, i, environ[i]);

	yatos_printf("[%d]: %d: data_bss: %x, data %x\n", 
	    yatos_thread_getid(), i, data_bss, data);

	for(i = 0; i < 100; ++i) {

		data_bss += i;
		data += i*2;
	}

	yatos_printf("[%d]: %d: data_bss: %x, data %x\n", 
	    yatos_thread_getid(), i, data_bss, data);

	yatos_printf("[%d]: floating test\n", 
	    yatos_thread_getid());

	fval = 1.0;
	fval *= 2.0;

	old_heap = yatos_vm_sbrk(0);
	yatos_printf("[%d]: sbrk(0) old-heap=%p\n",
	    yatos_thread_getid(), old_heap);

	old_heap = yatos_vm_sbrk(10);
	yatos_printf("[%d]: sbrk(10) old-heap=%p\n",
	    yatos_thread_getid(), old_heap);

	old_heap = yatos_vm_sbrk(4*1024*1024);
	yatos_printf("[%d]: sbrk(4M) old-heap=%p\n",
	    yatos_thread_getid(), old_heap);

	old_heap = yatos_vm_sbrk(-2*1024*1024);
	yatos_printf("[%d]: sbrk(-2M) old-heap=%p\n",
	    yatos_thread_getid(), old_heap);

	old_heap = yatos_vm_sbrk(0);
	yatos_printf("[%d]: sbrk(0) old-heap=%p\n",
	    yatos_thread_getid(), old_heap);

	yatos_printf("[%d]: heap access test:%p \n",
	    	    yatos_thread_getid(), old_heap);
	memset(old_heap - 4096, 0, 4096);

	yatos_printf("[%d]: create-thread start:%p stack:%p\n",
	    yatos_thread_getid(), (void *)new_thread, 
	    (void *)(old_heap - 4096));

	rc = yatos_proc_create_thread(0, (void *)new_thread, (void *)0x1234, 
	    (void *)(old_heap - 4096), &newid);
	yatos_printf("[%d]: create-thread rc=%d id=%d\n",
	    yatos_thread_getid(), rc, newid);

	yatos_printf("[%d]: wait any thread\n",
	    yatos_thread_getid());
	rc = yatos_thread_wait(newid, THR_WAIT_ANY, &chldid, &code);
	yatos_printf("[%d]: wait-thread rc=%d id=%d code=%d\n",
	    yatos_thread_getid(), rc, chldid, code);


	for(i = 0; i < 10; ++i) {

		yatos_thread_yield();

		yatos_printf("[%d]: %d th Hello World\n", 
		    yatos_thread_getid(), i );
	}

	yatos_register_user_event_handler(EV_SIG_USR1, user_handler);
	yatos_printf("[%d]: send event (id, data)=(%d, %p) \n",
	    yatos_thread_getid(), EV_SIG_USR1, (void *)0xdeaddead);
	rc = yatos_proc_send_event(yatos_thread_getid(), EV_SIG_USR1, (void *)0xdeaddead);
	yatos_printf("[%d]: send event rc = %d \n", yatos_thread_getid(), rc);

	yatos_printf("[%d]: consume user cpu resources, please wait\n", yatos_thread_getid());
	tsc1 = rdtsc();
	do{
		tsc2 = rdtsc();
	}while( (tsc2 - tsc1) < (uint64_t)LOOP_TSC );
	yatos_printf("[%d]: get resource usage of this thread and children.\n", yatos_thread_getid());
	rc = yatos_proc_get_thread_resource(yatos_thread_getid(), &tres);
	yatos_printf("[%d]: result (rc, sys, user, child-sys, child-user)"
	    "=(%d, %u, %u, %u, %u)\n", yatos_thread_getid(),
	    rc, tres.sys_time, tres.user_time, tres.children_sys_time, tres.children_user_time);

	yatos_printf("[%d]: process stack demand paging test.\n", yatos_thread_getid());
	user_stack_test();
	yatos_printf("[%d]: process stack demand paging test OK\n", yatos_thread_getid());

#if defined(CRASH_ME)
	*crash_p = 0xdeaddead;
#endif  /*  CRASH_ME  */

	yatos_printf("[%d]: return 0\n", yatos_thread_getid());

	return 0;
}
