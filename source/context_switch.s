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

// int mkw_switch_co_register_roundtrip(void** scheduler_sp, void** worker_sp)
//
// Save the caller's state, seed AAPCS64 callee-saved registers with sentinels,
// perform one cooperative switch away and back, then verify the sentinels.
// x30 continuity is exercised implicitly by returning to the instruction after
// `bl mkw_switch_co_switch` and then to the C++ caller.
.global mkw_switch_co_register_roundtrip
.type mkw_switch_co_register_roundtrip, %function
mkw_switch_co_register_roundtrip:
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

    mov x19, #0x1901
    mov x20, #0x2002
    mov x21, #0x2103
    mov x22, #0x2204
    mov x23, #0x2305
    mov x24, #0x2406
    mov x25, #0x2507
    mov x26, #0x2608
    mov x27, #0x2709
    mov x28, #0x2810
    mov x29, #0x2911

    mov x9, #0x0808
    fmov d8, x9
    mov x9, #0x0909
    fmov d9, x9
    mov x9, #0x1010
    fmov d10, x9
    mov x9, #0x1111
    fmov d11, x9
    mov x9, #0x1212
    fmov d12, x9
    mov x9, #0x1313
    fmov d13, x9
    mov x9, #0x1414
    fmov d14, x9
    mov x9, #0x1515
    fmov d15, x9

    bl mkw_switch_co_switch

    mov w10, #1

    mov x11, #0x1901
    cmp x19, x11
    b.ne .Lregister_fail
    mov x11, #0x2002
    cmp x20, x11
    b.ne .Lregister_fail
    mov x11, #0x2103
    cmp x21, x11
    b.ne .Lregister_fail
    mov x11, #0x2204
    cmp x22, x11
    b.ne .Lregister_fail
    mov x11, #0x2305
    cmp x23, x11
    b.ne .Lregister_fail
    mov x11, #0x2406
    cmp x24, x11
    b.ne .Lregister_fail
    mov x11, #0x2507
    cmp x25, x11
    b.ne .Lregister_fail
    mov x11, #0x2608
    cmp x26, x11
    b.ne .Lregister_fail
    mov x11, #0x2709
    cmp x27, x11
    b.ne .Lregister_fail
    mov x11, #0x2810
    cmp x28, x11
    b.ne .Lregister_fail
    mov x11, #0x2911
    cmp x29, x11
    b.ne .Lregister_fail

    fmov x12, d8
    mov x11, #0x0808
    cmp x12, x11
    b.ne .Lregister_fail
    fmov x12, d9
    mov x11, #0x0909
    cmp x12, x11
    b.ne .Lregister_fail
    fmov x12, d10
    mov x11, #0x1010
    cmp x12, x11
    b.ne .Lregister_fail
    fmov x12, d11
    mov x11, #0x1111
    cmp x12, x11
    b.ne .Lregister_fail
    fmov x12, d12
    mov x11, #0x1212
    cmp x12, x11
    b.ne .Lregister_fail
    fmov x12, d13
    mov x11, #0x1313
    cmp x12, x11
    b.ne .Lregister_fail
    fmov x12, d14
    mov x11, #0x1414
    cmp x12, x11
    b.ne .Lregister_fail
    fmov x12, d15
    mov x11, #0x1515
    cmp x12, x11
    b.ne .Lregister_fail
    b .Lregister_restore

.Lregister_fail:
    mov w10, #0

.Lregister_restore:
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
    mov w0, w10
    add sp, sp, #240
    ret
.size mkw_switch_co_register_roundtrip, .-mkw_switch_co_register_roundtrip
