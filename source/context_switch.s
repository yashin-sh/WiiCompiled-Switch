.text
.align 2

// Horizon/AAPCS64 cooperative context frame (240 bytes): x18-x30, q8-q15.
// We preserve x18 conservatively as platform state and the full q8-q15 values.
// x0 = address holding target saved SP
// x1 = address receiving current saved SP
.global mkw_switch_co_switch
.type mkw_switch_co_switch, %function
mkw_switch_co_switch:
    sub sp, sp, #240
    str x18, [sp, #0]
    stp x19, x20, [sp, #16]
    stp x21, x22, [sp, #32]
    stp x23, x24, [sp, #48]
    stp x25, x26, [sp, #64]
    stp x27, x28, [sp, #80]
    stp x29, x30, [sp, #96]
    stp q8, q9, [sp, #112]
    stp q10, q11, [sp, #144]
    stp q12, q13, [sp, #176]
    stp q14, q15, [sp, #208]

    mov x2, sp
    str x2, [x1]

    ldr x2, [x0]
    mov sp, x2
    ldr x18, [sp, #0]
    ldp x19, x20, [sp, #16]
    ldp x21, x22, [sp, #32]
    ldp x23, x24, [sp, #48]
    ldp x25, x26, [sp, #64]
    ldp x27, x28, [sp, #80]
    ldp x29, x30, [sp, #96]
    ldp q8, q9, [sp, #112]
    ldp q10, q11, [sp, #144]
    ldp q12, q13, [sp, #176]
    ldp q14, q15, [sp, #208]
    add sp, sp, #240
    ret
.size mkw_switch_co_switch, .-mkw_switch_co_switch

// x0 = one-past-end stack address
// x1 = entry(void*)
// x2 = entry argument
// returns x0 = saved SP compatible with mkw_switch_co_switch
.global mkw_switch_co_init
.type mkw_switch_co_init, %function
mkw_switch_co_init:
    bic x0, x0, #0xf
    sub x0, x0, #240
    str x18, [x0, #0]
    str x1, [x0, #16]      // x19 = entry
    str x2, [x0, #24]      // x20 = argument
    str xzr, [x0, #96]     // x29
    adr x3, mkw_switch_co_entry_trampoline
    str x3, [x0, #104]     // x30
    ret
.size mkw_switch_co_init, .-mkw_switch_co_init

.type mkw_switch_co_entry_trampoline, %function
mkw_switch_co_entry_trampoline:
    mov x0, x20
    blr x19
    brk #0
.size mkw_switch_co_entry_trampoline, .-mkw_switch_co_entry_trampoline
