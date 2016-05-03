#define ASM_FILE   1

.code64
/** 指定された値で所定のアドレスの内容をアトミックに交換する
    @param[in] addr ロック変数のアドレス
    @param[in] newval 更新する値
    @retval 交換前の値
    @note  uint32_t x86_64_xchg(volatile uint32_t *addr, uint32_t newval) 相当
*/
.global x86_64_xchg
x86_64_xchg:
	pushq %rbp
	mov   %rsp, %rbp
	lock; xchgl (%rdi), %esi
	movq  %rsi, %rax
	leaveq	
	retq
