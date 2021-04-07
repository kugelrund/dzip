.386
.model FLAT
externdef _LView:dword
externdef _temp2:dword
externdef __imp__SendMessageA@16:dword
externdef __imp__SetWindowLongA@12:dword
_TEXT SEGMENT

public @LVColumnWidth@4
public @LVDeleteAllItems@0
public @LVDeleteColumn@4
public @LVEditLabel@4
public @LVEnsureVisible@4
public @LVGetHeader@0
public @LVInsertColumn@8
public @LVItemRect@4
public @LVNumSelected@0
public @LVScroll@4
public @LVSetColumnOrder@8
public @LVSetColumnWidth@8
public @LVSetImageList@4
public @LVSetItemCount@4
public @LVSetItemState_Mask@12
public @LVSetItemState@8
public @LVTopIndex@0

@LVDeleteAllItems@0:
	push 0
	push 0
	push 1000h + 9
	jmp LVSendMessage

@LVDeleteColumn@4:
	push 0
	push ecx
	push 1000h + 28
	jmp LVSendMessage

@LVEditLabel@4:
	push 0
	push ecx
	push 1000h + 23
	jmp LVSendMessage

@LVEnsureVisible@4:
	push 0
	push ecx
	push 1000h + 19
	jmp LVSendMessage

@LVGetHeader@0:
	push 0
	push 0
	push 1000h + 31
	jmp LVSendMessage

@LVItemRect@4:
	push ecx		; ecx is a ptr to RECT
	xor edx, edx
	mov [ecx], edx	; sets RECT->left to zero [LVIR_BOUNDS]
	push edx
	push 1000h + 14
	jmp LVSendMessage

@LVScroll@4:
	push ecx
	push 0
	push 1000h + 20
	jmp LVSendMessage

@LVSetImageList@4:
	push ecx
	push 1
	push 1000h + 3
	jmp LVSendMessage

@LVTopIndex@0:
	push 0
	push 0
	push 1000h + 39
	jmp LVSendMessage

@LVNumSelected@0:
	push 0
	push 0
	push 1000h + 50

; everything ends up here
; it's in the middle so all the jmps will be two bytes
LVSendMessage:
	push _LView
	call [__imp__SendMessageA@16]
	ret

@LVSetColumnOrder@8:
	push edx
	push ecx
	push 1000h + 58
	jmp LVSendMessage

@LVSetColumnWidth@8:
	push edx
	push ecx
	push 1000h + 30
	jmp LVSendMessage

@LVSetItemCount@4:
	push 3	; LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL 
	push ecx
	push 1000h + 47
	jmp LVSendMessage

@LVSetItemState_Mask@12:
	; lvitem's stateMask field starts in [esp+4]
	pop eax				; this is the return address
	xchg eax, [esp]		; now the return address is in the right place
	push ebx			; preserve previous ebx
	mov ebx, offset _temp2
	mov [ebx+12], edx	; copy state into temp2 as an lvitem
	mov [ebx+16], eax	; copy stateMask into temp2 as an lvitem
	xchg ebx, [esp]		; restore old ebx and put temp2 address on stack
	jmp LVSIS

@LVSetItemState@8:
	mov eax, offset _temp2
	push eax
	mov [eax+12], edx
	push 3
	pop edx
	mov [eax+16], edx
LVSIS:
	push ecx
	push 1000h + 43
	jmp LVSendMessage

@LVColumnWidth@4:
	push 0
	push ecx
	push 1000h + 29
	jmp LVSendMessage

@LVInsertColumn@8:
	push ecx
	push edx
	push 1000h + 27
	jmp LVSendMessage

_TEXT ENDS
END