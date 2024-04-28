
	global _start

	section .text
_start:
	; print(message)
	mov	rdi, message
	call	print

	mov	rdi, message
	add	rdi, 14
	call	print

	; exit(0)
	mov	eax, 60
	xor	rdi, rdi
	syscall

; ------------------------------------------------------------------------
; int print(char* p_str);
; ------------------------------------------------------------------------
print:
	push	rdi

	; strlen(p_str)
	call	strlen
	mov	rdx, rax

	; write(1, p_str, strlen(p_str))
	mov	rax, 1
	mov	rdi, 1
	pop	rsi
	syscall

	ret
; ------------------------------------------------------------------------

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
	db	"Hello, World", 10, 0
	db	"This is a test for nasm", 10, 0
