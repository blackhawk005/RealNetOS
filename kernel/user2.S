.section .text
.global user2_entry
.type user2_entry, %function

user2_entry:
    mov x0, #0
    mov x8, #3
    svc #0

    b .