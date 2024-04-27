
	global _start

	section .text
_start:
	; strlen(message)
	mov	rdi, message
	call	strlen
	mov	rdx, rax

	; write(1, message, 13)
	mov	rax, 1
	mov	rdi, 1
	mov	rsi, message
	syscall

	; exit(0)
	mov	eax, 60
	xor	rdi, rdi
	syscall

; ------------------------------------------------------------------------
; int strlen(char* p_str);
; ------------------------------------------------------------------------
strlen:
	mov	rax, 0			; �ַ������ȿ�ʼ�� 0
	mov	rsi, rdi		; rdi �Ǻ�������,rsi ָ���׵�ַ

.1:
	cmp	byte [rsi], 0		; �� rsi ָ����ַ��Ƿ��� '\0'
	jz	.2			; ����� '\0'���������
	inc	rsi			; ������� '\0'��rsi ָ����һ���ַ�
	inc	rax			;	  ���ң�eax �Լ�һ
	jmp	.1			; ���ѭ��

.2:
	ret				; ��������������
; ------------------------------------------------------------------------


	section .data
message:
	db	"Hello, World", 10
