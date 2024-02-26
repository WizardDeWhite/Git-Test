
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                              string.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                       Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

[SECTION .text]

; ��������
global	memcpy
global	memset


; ------------------------------------------------------------------------
; void* memcpy(void* es:pDest, void* ds:pSrc, int iSize);
; ------------------------------------------------------------------------
memcpy:
	push	ebp
	mov	ebp, esp

	; current stack
	;+----------------------+
	;|    ebp               |  <- esp, ebp = esp
	;+----------------------+
	;|    EIP               |
	;+----------------------+
	;|    pDest             |
	;+----------------------+
	;|    pSrc              |
	;+----------------------+
	;|    iSize             |
	;+----------------------+

	push	esi
	push	edi
	push	ecx

	; current stack
	;+----------------------+
	;|    esi               |  <- esp
	;+----------------------+
	;|    edi               |
	;+----------------------+
	;|    ecx               |
	;+----------------------+
	;|    ebp               |  <- ebp
	;+----------------------+
	;|    EIP               |
	;+----------------------+
	;|    pDest             |
	;+----------------------+
	;|    pSrc              |
	;+----------------------+
	;|    iSize             |
	;+----------------------+

	mov	edi, [ebp + 8]	; Destination
	mov	esi, [ebp + 12]	; Source
	mov	ecx, [ebp + 16]	; Counter
.1:
	cmp	ecx, 0		; �жϼ�����
	jz	.2		; ������Ϊ��ʱ����

	mov	al, [ds:esi]		; ��
	inc	esi			; ��
					; �� ���ֽ��ƶ�
	mov	byte [es:edi], al	; ��
	inc	edi			; ��

	dec	ecx		; ��������һ
	jmp	.1		; ѭ��
.2:
	mov	eax, [ebp + 8]	; ����ֵ

	pop	ecx
	pop	edi
	pop	esi
	mov	esp, ebp
	pop	ebp

	ret			; ��������������
; memcpy ����-------------------------------------------------------------


; ------------------------------------------------------------------------
; void memset(void* p_dst, char ch, int size);
; ------------------------------------------------------------------------
memset:
	push	ebp
	mov	ebp, esp

	push	esi
	push	edi
	push	ecx
	push	edx

	mov	edi, [ebp + 8]	; Destination
	mov	edx, [ebp + 12]	; Char to be putted
	mov	ecx, [ebp + 16]	; Counter
.1:
	cmp	ecx, 0		; �жϼ�����
	jz	.2		; ������Ϊ��ʱ����

	mov	byte [edi], dl		; ��
	inc	edi			; ��

	dec	ecx		; ��������һ
	jmp	.1		; ѭ��
.2:

	pop	edx
	pop	ecx
	pop	edi
	pop	esi
	mov	esp, ebp
	pop	ebp

	ret			; ��������������
; ------------------------------------------------------------------------


