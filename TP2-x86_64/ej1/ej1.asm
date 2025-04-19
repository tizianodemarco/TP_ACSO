section .text
global string_proc_list_create_asm
global string_proc_node_create_asm
global string_proc_list_add_node_asm
global string_proc_list_concat_asm

extern malloc
extern free
extern strdup
extern str_concat

; string_proc_list* string_proc_list_create_asm(void)
string_proc_list_create_asm:
    push rbp
    mov rbp, rsp

    mov edi, 16                ; tamaño de string_proc_list
    call malloc

    test rax, rax
    je .return

    ; Inicialización de campos a NULL
    xor rcx, rcx
    mov [rax], rcx             ; first
    mov [rax+8], rcx           ; last

.return:
    pop rbp
    ret


; string_proc_node* string_proc_node_create_asm(uint8_t type, char* hash)
string_proc_node_create_asm:
    push rbp
    mov rbp, rsp
    push rbx
    push r12

    movzx rbx, dil             ; type -> rbx
    mov r12, rsi               ; hash

    mov edi, 32                ; tamaño del nodo
    call malloc
    test rax, rax
    je .exit

    ; Inicializar nodo
    xor rcx, rcx
    mov [rax], rcx             ; next
    mov [rax+8], rcx           ; previous
    mov byte [rax+16], bl      ; type
    mov [rax+24], r12          ; hash

.exit:
    pop r12
    pop rbx
    pop rbp
    ret


; void string_proc_list_add_node_asm(string_proc_list* list, uint8_t type, char* hash)
string_proc_list_add_node_asm:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14

    mov rbx, rdi
    movzx r12, sil             ; type
    mov r13, rdx               ; hash

    mov dil, sil
    mov rsi, r13
    call string_proc_node_create_asm
    test rax, rax
    je .done

    mov r14, rax               ; nuevo nodo

    cmp qword [rbx], 0
    jne .append

    ; lista vacía
    mov [rbx], r14
    mov [rbx+8], r14
    jmp .done

.append:
    mov rax, [rbx+8]           ; último nodo
    mov [r14+8], rax           ; new_node->previous = last
    mov [rax], r14             ; last->next = new_node
    mov [rbx+8], r14           ; list->last = new_node

.done:
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret


; char* string_proc_list_concat_asm(string_proc_list* list, uint8_t type, char* hash)
string_proc_list_concat_asm:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov rbx, rdi               ; list
    movzx r12, sil             ; type
    mov r13, rdx               ; hash inicial

    mov rdi, r13
    call strdup
    test rax, rax
    je .final

    mov r14, rax               ; acumulador
    mov r15, [rbx]             ; nodo actual

.loop:
    test r15, r15
    je .final

    movzx eax, byte [r15+16]
    cmp al, r12b
    jne .next

    mov rdi, r14
    mov rsi, [r15+24]
    call str_concat
    test rax, rax
    je .free_and_fail

    ; liberar anterior y reemplazar
    mov rdi, r14
    mov r14, rax
    call free

.next:
    mov r15, [r15]
    jmp .loop

.free_and_fail:
    mov rdi, r14
    call free
    xor r14, r14

.final:
    mov rax, r14
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret
