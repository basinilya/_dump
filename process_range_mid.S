.text
.globl process_range_mid
.type   process_range_mid, @function

process_range_mid:
	#push   %rbp                               
	#mov    %rsp,%rbp                          

	sub    $24,%rsp
	mov    %rsp, %rdx

	push %rdi
	call  process_range_mid_real
	pop %rdi
	lea    0(%rsp),%rsi                   
	call  *%rax                              

	add $24,%rsp
	#leave                                    
	ret                                      

