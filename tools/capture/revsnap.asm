; REVSNAP.COM: a small resident DOS program that plays a key script into the
; original game under DOSBox-X and captures the screen and memory at its
; @snaps, so the original's traces can be compared with the port's goldens
; (tools/trace/original.py). Our own code, for studying the original; its
; shape follows mech1-re's MWSNAP. It feeds keys by the harness's rules
; (src/SDL/harness.c), so a script means the same on both sides.
;
; Usage: REVSNAP, then the game. It reads KEYS.BIN (tools/capture/keys.py).
;
; The game's keyboard is its own: an int 9 handler (3037:0510 in the
; listing's numbering, image loaded at 0800) fills a ring of key events in its
; code segment, KeyboardInputPending (3037:01A0) compares the ring's ends, and
; ReadRawKeypress (3037:01B5) spins until the ring has an event. A poll is a
; call of the first or a pass of the second's spin, as in the port, where
; both call SDLBackend_InputPending.
;
; When the game installs its int 9 handler (int 21h AX=2509h, DX=0510h),
; REVSNAP takes DS as segment 3037, checks a sum of the bytes at both entries
; (on any other bytes it writes NOPATCH.TXT and patches nothing) and puts a
; far jump to itself at each, in memory only, never in the file:
; - 01A0: one poll, then the ring's ends compared as the game does.
; - 01B5: polls until the ring has an event, then back to the game at 01C1.
; At each poll the script moves on as in the harness: the next item is due on
; the 4th poll since the last key or snap, plus its idle:N, once steps:N
; battle steps (moves of the subtick at DS:8339) have been seen since, and
; once still:N polls have gone by without a step. A due key goes in
; through the game's own int 9 handler, as from the keyboard (the 8042's D2h
; command puts each scan code in its output buffer and raises IRQ1): Shift,
; Ctrl, Alt pressed first when the script says so, the key pressed and
; released, the modifiers released. A due snap writes a capture; a due mouse
; item calls the handler the game gave the mouse driver (int 33h AX=0Ch) as a
; driver would. When the items are done, the next wait writes REVSNAPE.TXT.
;
; So that every run is the same: the BIOS's Num Lock and Caps Lock are cleared
; (the game's handler reads them on every key), and the DOS date and time the
; game asks for are 1991-01-01 12:00:00 plus the BIOS ticks since REVSNAP
; started, as the harness's are.
;
; A capture, RVSnnnnn.BIN: 'RVS1' (4), version 2 (1), the BIOS video mode (1),
; the image's load segment (2), polls (4), keys taken (4), the BIOS ticks
; since REVSNAP started (2), the CRTC start address (2), the attribute
; registers 0-15, 10h and 14h (18), the DAC's 256 RGB (768, 6-bit); then the
; frame: in mode 0Dh the four planes, 8000 bytes each from the CRTC start, in
; any other mode the 64000 bytes at A000:0000; then 'MEMS' (4) and the first
; 640K of memory.
;
; Files it writes in the current directory: RVSnnnnn.BIN, REVSNAPE.TXT
; ("done items N polls P reads R"), NOPATCH.TXT, KEYFAIL.TXT (a key the game's
; handler never took), ITEMFAIL.TXT (a mouse item before the game set its
; mouse handler).
;
; Build: nasm -f bin -o REVSNAP.COM revsnap.asm

        org 100h
        jmp near install

POLLS           equ 4
MAX_ITEMS       equ 2000
ITEM_SIZE       equ 20
KIND_KEY        equ 1
KIND_SNAP       equ 2
KIND_MOUSE      equ 3

; The game, in the listing's numbering (the image loaded at 0800).
LISTING_BASE    equ 0800h
KEYBOARD_SEG    equ 3037h
HANDLER_OFF     equ 0510h
PENDING_OFF     equ 01A0h
PENDING_LEN     equ 10
PENDING_SUM     equ 0cddeh
READ_OFF        equ 01B5h
READ_LEN        equ 12
READ_SUM        equ 6b51h
READ_RESUME     equ 01C1h
RING_READ       equ 00FCh
RING_WRITE      equ 00FEh
DATA_SEG        equ 3A6Ah
SUBTICK         equ 8339h               ; the battle clock's subtick: one move a battle step

; --- resident data -----------------------------------------------------------
old21           dd 0
old33           dd 0
game_int9       dd 0                    ; the game's own int 9 handler
mouse_handler   dd 0                    ; what the game gave int 33h AX=0Ch
mouse_set       db 0
game_seg        dw 0                    ; the game's segment 3037
load_seg        dw 0                    ; the image's load segment
data_seg        dw 0                    ; the game's DS (3A6A)
steps_since     dd 0                    ; battle steps seen since the last key or snap
last_subtick    db 0                    ; the subtick at the previous poll
polls_still     dd 0                    ; polls since the last step, key or snap
resume          dw READ_RESUME          ; far pointer: game_seg:01C1
resume_seg      dw 0
irq_seen        db 0
busy            db 0
done            db 0
polls           dd 0
reads           dd 0
since           dd 0                    ; polls since the last key or snap
item_ptr        dw items
items_end       dw items
item_count      dw 0
start_tick      dw 0
capture_number  dw 0
save_ss         dw 0
save_sp         dw 0
dac             times 768 db 0
header          db 'RVS1', 2
header_mode     db 0
header_load     dw 0
header_polls    dd 0
header_reads    dd 0
header_ticks    dw 0
header_start    dw 0
header_attr     times 18 db 0
header_end:
gc_index        db 0
gc_map          db 0
gc_mode         db 0
mems_tag        db 'MEMS'
capture_name    db 'RVS00000.BIN', 0
done_name       db 'REVSNAPE.TXT', 0
nopatch_name    db 'NOPATCH.TXT', 0
keyfail_name    db 'KEYFAIL.TXT', 0
itemfail_name   db 'ITEMFAIL.TXT', 0
done_text       db 'done items '
done_items      db '00000 polls '
done_polls      db '0000000000 reads '
done_reads      db '0000000000', 13, 10
done_text_end:

; --- int 21h: the game's int 9 install, and the date and time ----------------
int21:
        cmp byte [cs:busy], 0
        jne .chain
        cmp ax, 2509h
        je .vector
        cmp ah, 2ah
        je .date
        cmp ah, 2ch
        je .time
.chain:
        jmp far [cs:old21]
.date:
        mov cx, 1991
        mov dx, 0101h
        mov al, 2                       ; a Tuesday
        iret
.time:
        push ax
        push bx
        push es
        mov ax, 40h
        mov es, ax
        mov ax, [es:6ch]
        sub ax, [cs:start_tick]
        mov bx, 10
        mul bx                          ; ticks x 10 / 182 = seconds
        mov bx, 182
        div bx
        push ax                         ; seconds
        mov ax, dx                      ; the rest, in 182nds of a second
        mov bx, 100
        mul bx
        mov bx, 182
        div bx
        mov dl, al                      ; hundredths
        pop ax
        push dx
        xor dx, dx
        mov bx, 60
        div bx
        mov dh, dl                      ; seconds
        push dx
        xor dx, dx
        div bx                          ; ax hours, dx minutes
        mov cl, dl
        mov ch, al
        add ch, 12
        pop dx
        pop bx
        mov dl, bl
        pop es
        pop bx
        pop ax
        iret
.vector:
        cmp dx, HANDLER_OFF
        jne .chain
        cmp word [cs:game_seg], 0
        jne .chain
        push ax
        push bx
        push cx
        push si
        push es
        mov [cs:game_int9], dx
        mov [cs:game_int9 + 2], ds
        mov ax, ds
        mov es, ax
        mov si, PENDING_OFF
        mov cx, PENDING_LEN
        call code_sum
        cmp bx, PENDING_SUM
        jne .refuse
        mov si, READ_OFF
        mov cx, READ_LEN
        call code_sum
        cmp bx, READ_SUM
        jne .refuse
        mov [cs:game_seg], ax
        mov [cs:resume_seg], ax
        sub ax, KEYBOARD_SEG - LISTING_BASE
        mov [cs:load_seg], ax
        add ax, DATA_SEG - LISTING_BASE
        mov [cs:data_seg], ax
        mov byte [es:PENDING_OFF], 0eah ; jmp far pending_hook
        mov word [es:PENDING_OFF + 1], pending_hook
        mov [es:PENDING_OFF + 3], cs
        mov byte [es:READ_OFF], 0eah    ; jmp far read_hook
        mov word [es:READ_OFF + 1], read_hook
        mov [es:READ_OFF + 3], cs
        pop es
        pop si
        pop cx
        pop bx
        pop ax
        ; Install int9_wrap in the handler's place, so send_scan knows when
        ; the game's handler has run. The caller's DS:DX are kept.
        push ds
        push dx
        push cs
        pop ds
        mov dx, int9_wrap
        pushf
        call far [cs:old21]
        pop dx
        pop ds
        iret
.refuse:
        pop es
        pop si
        pop cx
        pop bx
        pop ax
        push dx
        push ds
        push cs
        pop ds
        mov dx, nopatch_name
        call touch
        pop ds
        pop dx
        jmp .chain

; The sum the guards compare: h = rol(h, 3) xor byte, over CX bytes at ES:SI.
; Returns BX.
code_sum:
        push ax
        push cx
        push si
        xor bx, bx
.next:
        rol bx, 3
        mov al, [es:si]
        xor ah, ah
        xor bx, ax
        inc si
        loop .next
        pop si
        pop cx
        pop ax
        ret

; The game's int 9 handler, wrapped: notes that it ran, then runs it.
int9_wrap:
        mov byte [cs:irq_seen], 1
        jmp far [cs:game_int9]

; --- int 33h: the handler the game gives the mouse driver --------------------
int33:
        cmp ax, 0ch
        jne .chain
        mov [cs:mouse_handler], dx
        mov [cs:mouse_handler + 2], es
        mov byte [cs:mouse_set], 1
.chain:
        jmp far [cs:old33]

; --- the two hooks -----------------------------------------------------------
; 3037:01A0, by a far jump: the game's far return is on the stack.
pending_hook:
        call poll_on_own_stack
        push ds
        mov ds, [cs:game_seg]
        mov ax, [RING_READ]
        xor ax, [RING_WRITE]            ; the flags as the game's xor left them
        pop ds
        retf

; 3037:01B5, by a far jump: as the game, DI is saved first.
read_hook:
        push di
.spin:
        call poll_on_own_stack
        push ds
        mov ds, [cs:game_seg]
        mov ax, [RING_READ]
        cmp ax, [RING_WRITE]
        pop ds
        je .spin
        jmp far [cs:resume]             ; 01C1 takes the event at AX

; One poll, on REVSNAP's stack; every register but SP is kept.
poll_on_own_stack:
        pushf
        push ax
        push bx
        push cx
        push dx
        push si
        push di
        push bp
        push ds
        push es
        mov ax, cs
        mov ds, ax
        mov [save_ss], ss
        mov [save_sp], sp
        cli
        mov ss, ax
        mov sp, stack_top
        sti
        cld
        call poll
        cli
        mov ss, [cs:save_ss]
        mov sp, [cs:save_sp]
        pop es
        pop ds
        pop bp
        pop di
        pop si
        pop dx
        pop cx
        pop bx
        pop ax
        popf
        ret

; --- the script, at a poll (DS = CS) -----------------------------------------
poll:
        add word [polls], 1
        adc word [polls + 2], 0
        add word [since], 1
        adc word [since + 2], 0
        add word [polls_still], 1
        adc word [polls_still + 2], 0
        call count_step
.next:
        mov si, [item_ptr]
        cmp si, [items_end]
        jae .end
        mov ax, [si + 2]                ; due at POLLS + idle polls since
        mov dx, [si + 4]
        add ax, POLLS
        adc dx, 0
        cmp [since + 2], dx
        jb .wait
        ja .polls_done
        cmp [since], ax
        jb .wait
.polls_done:
        mov ax, [steps_since]           ; and steps:N battle steps since
        mov dx, [steps_since + 2]
        cmp dx, [si + 8]
        jb .wait
        ja .steps_done
        cmp ax, [si + 6]
        jb .wait
.steps_done:
        mov ax, [polls_still]           ; and still:N polls without a step
        mov dx, [polls_still + 2]
        cmp dx, [si + 12]
        jb .wait
        ja .due
        cmp ax, [si + 10]
        jb .wait
.due:
        mov ax, [si]
        cmp ax, KIND_KEY
        je .key
        cmp ax, KIND_SNAP
        je .snap
        call mouse_item
        add word [item_ptr], ITEM_SIZE
        jmp .next
.key:
        call send_key
        add word [reads], 1
        adc word [reads + 2], 0
        jmp .taken
.snap:
        call capture
.taken:
        add word [item_ptr], ITEM_SIZE
        mov word [since], 0
        mov word [since + 2], 0
        mov word [steps_since], 0
        mov word [steps_since + 2], 0
        mov word [polls_still], 0
        mov word [polls_still + 2], 0
.wait:
        ret
.end:
        cmp byte [done], 0
        jne .wait
        cmp word [since + 2], 0
        jne .finish
        cmp word [since], POLLS
        jb .wait
.finish:
        mov byte [done], 1
        call write_done
        ret

; A poll that sees the subtick moved counts one battle step (the battle loop
; polls at least once a pass and steps at most once).
count_step:
        push es
        mov es, [data_seg]
        mov al, [es:SUBTICK]
        pop es
        cmp al, [last_subtick]
        je .same
        mov [last_subtick], al
        add word [steps_since], 1
        adc word [steps_since + 2], 0
        mov word [polls_still], 0
        mov word [polls_still + 2], 0
.same:
        ret

; The key at [SI]: modifier flags at +14, the scan code at +16.
send_key:
        call clear_locks
        mov bl, [si + 14]
        test bl, 1
        jz .no_shift
        mov al, 2ah
        call send_scan
.no_shift:
        test bl, 2
        jz .no_ctrl
        mov al, 1dh
        call send_scan
.no_ctrl:
        test bl, 4
        jz .no_alt
        mov al, 38h
        call send_scan
.no_alt:
        mov al, [si + 16]
        call send_scan
        mov al, [si + 16]
        or al, 80h
        call send_scan
        test bl, 4
        jz .up_alt
        mov al, 0b8h
        call send_scan
.up_alt:
        test bl, 2
        jz .up_ctrl
        mov al, 9dh
        call send_scan
.up_ctrl:
        test bl, 1
        jz .up_shift
        mov al, 0aah
        call send_scan
.up_shift:
        ret

; Num Lock and Caps Lock off in the BIOS's keyboard flags.
clear_locks:
        push es
        mov ax, 40h
        mov es, ax
        and byte [es:17h], 9fh
        pop es
        ret

; Waits (bounded) until the 8042 can take a byte; carry when it never could.
kbc_ready:
        push cx
        mov cx, 0ffffh
.wait:
        in al, 64h
        test al, 2
        jz .ok
        loop .wait
        pop cx
        stc
        ret
.ok:
        pop cx
        clc
        ret

; The scan code in AL, as from the keyboard. Waits until the game's handler
; has run, at most 3 timer ticks; KEYFAIL.TXT otherwise.
send_scan:
        push bx
        push cx
        push dx
        push es
        mov ah, al
        call kbc_ready
        jc .fail
        mov al, 0d2h
        out 64h, al
        call kbc_ready
        jc .fail
        mov byte [irq_seen], 0
        mov al, ah
        out 60h, al
        sti
        mov bx, 40h
        mov es, bx
        mov dx, [es:6ch]
        mov cx, 3
.taken:
        cmp byte [irq_seen], 0
        jne .done
        cmp dx, [es:6ch]
        je .taken
        mov dx, [es:6ch]
        loop .taken
.fail:
        mov dx, keyfail_name
        call touch
.done:
        pop es
        pop dx
        pop cx
        pop bx
        ret

; The mouse item at [SI]: X at +14, Y at +16, the buttons at +18, given to the
; game's handler as a driver gives an event: AX the conditions (moved, and a
; button pressed or released), BX the buttons, CX X in the driver's 640-wide
; range, DX Y.
mouse_item:
        cmp byte [mouse_set], 0
        jne .call
        mov dx, itemfail_name
        call touch
        ret
.call:
        mov bx, [si + 18]
        mov cl, [last_buttons]
        mov [last_buttons], bl
        mov ax, 1
        mov ch, bl
        xor ch, cl                      ; the buttons that changed
        test ch, 1
        jz .left_same
        test bl, 1
        mov dx, 2                       ; left pressed
        jnz .left_set
        mov dx, 4                       ; left released
.left_set:
        or ax, dx
.left_same:
        test ch, 2
        jz .right_same
        test bl, 2
        mov dx, 8                       ; right pressed
        jnz .right_set
        mov dx, 16                      ; right released
.right_set:
        or ax, dx
.right_same:
        mov cx, [si + 14]
        shl cx, 1
        mov dx, [si + 16]
        xor si, si
        xor di, di
        push ds
        call far [cs:mouse_handler]
        pop ds
        mov si, [item_ptr]
        ret
last_buttons    db 0

; --- files -------------------------------------------------------------------
; Creates the file named at DS:DX, empty.
touch:
        push ax
        push bx
        push cx
        mov byte [cs:busy], 1
        mov ah, 3ch
        xor cx, cx
        int 21h
        jc .out
        mov bx, ax
        mov ah, 3eh
        int 21h
.out:
        mov byte [cs:busy], 0
        pop cx
        pop bx
        pop ax
        ret

; Writes CX bytes at DS:DX to the handle in BX (DS may be any segment).
write_block:
        mov ah, 40h
        int 21h
        ret

; A capture (DS = CS).
capture:
        mov byte [busy], 1
        mov ax, [load_seg]
        mov [header_load], ax
        mov ax, [polls]
        mov [header_polls], ax
        mov ax, [polls + 2]
        mov [header_polls + 2], ax
        mov ax, [reads]
        mov [header_reads], ax
        mov ax, [reads + 2]
        mov [header_reads + 2], ax
        push es
        mov ax, 40h
        mov es, ax
        mov al, [es:49h]
        mov [header_mode], al
        mov ax, [es:6ch]
        sub ax, [start_tick]
        mov [header_ticks], ax
        pop es
        call read_crtc_start
        call read_attributes
        mov dx, 3c7h                    ; the DAC, from colour 0
        xor al, al
        out dx, al
        mov dx, 3c9h
        mov di, dac
        mov cx, 768
.dac:
        in al, dx
        mov [di], al
        inc di
        loop .dac
        mov ah, 3ch
        xor cx, cx
        mov dx, capture_name
        int 21h
        jc .out
        mov bx, ax
        mov dx, header
        mov cx, header_end - header
        call write_block
        mov dx, dac
        mov cx, 768
        call write_block
        cmp byte [header_mode], 0dh
        jne .linear
        call write_planes
        jmp .frame_done
.linear:
        push ds
        mov ax, 0a000h
        mov ds, ax
        xor dx, dx
        mov cx, 64000
        call write_block
        pop ds
.frame_done:
        mov dx, mems_tag
        mov cx, 4
        call write_block
        xor si, si                      ; 640K in 40 blocks of 16K
.memory:
        push ds
        mov ds, si
        xor dx, dx
        mov cx, 4000h
        call write_block
        pop ds
        add si, 400h
        cmp si, 0a000h
        jb .memory
        mov ah, 3eh
        int 21h
        mov si, capture_name + 7        ; the next number
.carry:
        inc byte [si]
        cmp byte [si], '9'
        jbe .out
        mov byte [si], '0'
        dec si
        jmp .carry
.out:
        mov byte [busy], 0
        mov si, [item_ptr]
        ret

; The CRTC's start address (registers 0Ch, 0Dh) into header_start; the
; CRTC index is kept.
read_crtc_start:
        mov dx, 3d4h
        in al, dx
        mov bl, al
        mov al, 0ch
        out dx, al
        inc dx
        in al, dx
        mov ah, al
        dec dx
        mov al, 0dh
        out dx, al
        inc dx
        in al, dx
        mov [header_start], ax
        dec dx
        mov al, bl
        out dx, al
        ret

; Attribute registers 0-15, 10h and 14h into header_attr, read with the
; palette address source set (the screen stays on); the controller's index
; is put back.
read_attributes:
        mov dx, 3dah
        in al, dx                       ; the flip-flop to "index"
        mov dx, 3c0h
        in al, dx
        mov bh, al                      ; the index as it was
        mov di, header_attr
        xor cl, cl
.next:
        mov al, cl
        call read_attribute
        inc cl
        cmp cl, 16
        jb .next
        mov al, 10h
        call read_attribute
        mov al, 14h
        call read_attribute
        mov dx, 3dah
        in al, dx
        mov dx, 3c0h
        mov al, bh
        out dx, al
        ret

; Attribute register AL into [DI], DI moved on.
read_attribute:
        push ax
        mov dx, 3dah
        in al, dx
        pop ax
        or al, 20h
        mov dx, 3c0h
        out dx, al
        inc dx
        in al, dx
        mov [di], al
        inc di
        ret

; Mode 0Dh: the four planes, 8000 bytes each from the CRTC start, to the
; handle in BX; the graphics controller's index, read map and mode are put
; back.
write_planes:
        mov dx, 3ceh
        in al, dx
        mov [gc_index], al
        mov al, 4
        out dx, al
        inc dx
        in al, dx
        mov [gc_map], al
        dec dx
        mov al, 5
        out dx, al
        inc dx
        in al, dx
        mov [gc_mode], al
        and al, 0f7h                    ; read mode 0
        out dx, al
        xor cl, cl
.plane:
        mov dx, 3ceh
        mov al, 4
        out dx, al
        inc dx
        mov al, cl
        out dx, al
        push cx
        push ds
        mov dx, [header_start]
        mov ax, 0a000h
        mov ds, ax
        mov cx, 8000
        call write_block
        pop ds
        pop cx
        inc cl
        cmp cl, 4
        jb .plane
        mov dx, 3ceh
        mov al, 5
        out dx, al
        inc dx
        mov al, [gc_mode]
        out dx, al
        dec dx
        mov al, 4
        out dx, al
        inc dx
        mov al, [gc_map]
        out dx, al
        dec dx
        mov al, [gc_index]
        out dx, al
        ret

; REVSNAPE.TXT: "done items N polls P reads R".
write_done:
        mov ax, [item_count]
        xor dx, dx
        mov di, done_items + 4
        mov cx, 5
        call decimal
        mov ax, [polls]
        mov dx, [polls + 2]
        mov di, done_polls + 9
        mov cx, 10
        call decimal
        mov ax, [reads]
        mov dx, [reads + 2]
        mov di, done_reads + 9
        mov cx, 10
        call decimal
        mov byte [busy], 1
        mov ah, 3ch
        xor cx, cx
        mov dx, done_name
        int 21h
        jc .out
        mov bx, ax
        mov dx, done_text
        mov cx, done_text_end - done_text
        call write_block
        mov ah, 3eh
        int 21h
.out:
        mov byte [busy], 0
        ret

; DX:AX in CX decimal digits, right to left from DI (leading zeros kept).
decimal:
        push bx
.digit:
        push ax                         ; DX:AX / 10, the remainder in BX
        mov ax, dx
        xor dx, dx
        mov bx, 10
        div bx
        mov bp, ax                      ; the quotient's high word
        pop ax
        div bx
        add dl, '0'
        mov [di], dl
        dec di
        mov dx, bp
        loop .digit
        pop bx
        ret

                align 2
                times 512 dw 0
stack_top:
items:          times MAX_ITEMS * ITEM_SIZE db 0
resident_end:

; --- install (not resident) --------------------------------------------------
install:
        mov ax, 3d00h                   ; KEYS.BIN
        mov dx, keys_name
        int 21h
        jc .no_keys
        mov bx, ax
        mov ah, 3fh
        mov cx, 2
        mov dx, item_count
        int 21h
        jc .no_keys
        mov ax, [item_count]
        cmp ax, MAX_ITEMS
        ja .no_keys
        mov cx, ITEM_SIZE
        mul cx
        mov cx, ax
        mov ah, 3fh
        mov dx, items
        int 21h
        jc .no_keys
        cmp ax, cx
        jne .no_keys
        add ax, items
        mov [items_end], ax
        mov ah, 3eh
        int 21h
        call clear_locks
        push es
        mov ax, 40h
        mov es, ax
        mov ax, [es:6ch]
        mov [start_tick], ax
        pop es
        mov ax, 3521h
        int 21h
        mov [old21], bx
        mov [old21 + 2], es
        mov ax, 3533h
        int 21h
        mov [old33], bx
        mov [old33 + 2], es
        mov ax, 2521h
        mov dx, int21
        int 21h
        mov ax, 2533h
        mov dx, int33
        int 21h
        mov ah, 9
        mov dx, installed
        int 21h
        mov ax, 3100h
        mov dx, (resident_end - $$ + 100h + 15) / 16
        int 21h
.no_keys:
        mov ah, 9
        mov dx, no_keys
        int 21h
        mov ax, 4c01h
        int 21h

keys_name       db 'KEYS.BIN', 0
installed       db 'REVSNAP resident', 13, 10, '$'
no_keys         db 'REVSNAP: no KEYS.BIN, or too many items', 13, 10, '$'
