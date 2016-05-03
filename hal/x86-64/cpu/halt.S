#define ASM_FILE   1

.code64
/** アイドル処理(halt命令によるCPU停止)
*/
.global hal_idle
hal_idle:
	pushq %rbp
	mov   %rsp, %rbp
	pushfq
	sti
	hlt
	popfq
	leaveq
	retq
	
