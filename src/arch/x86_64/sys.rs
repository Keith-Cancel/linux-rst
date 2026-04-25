use core::arch::asm;

use super::def_sys_call;

/// System call register.
pub type Reg = u64;

// System call number
// https://chromium.googlesource.com/chromiumos/docs/+/master/constants/syscalls.md#x86_64-64_bit
def_sys_call! {
    read:       0,
    write:      1,
    close:      3,
    open_at:    257,
    unlink_at:  263,
    exit:       60,
    exit_group: 231,
    socket:     41,
    connect:    42,
    accept:     43,
    bind:       49,
    listen:     50,
    prctl:      157,
}

#[inline(always)]
pub unsafe fn call0(num: SysCall) -> Reg {
    let mut rax = num.get();
    unsafe {
        asm!(
            "syscall",
            inout("rax") rax,
            out("r11") _, // syscall clobbers both rcx and r11
            out("rcx") _,
            options(nostack, preserves_flags),
        );
    }
    return rax;
}

#[inline(always)]
pub unsafe fn call1(arg0: Reg, num: SysCall) -> Reg {
    let mut rax = num.get();
    unsafe {
        asm!(
            "syscall",
            in("rdi") arg0,
            inout("rax") rax,
            out("r11") _, // syscall clobbers both rcx and r11
            out("rcx") _,
            options(nostack, preserves_flags),
        );
    }
    return rax;
}

#[inline(always)]
pub unsafe fn call1_no_return(arg0: Reg, num: SysCall) -> ! {
    let rax = num.get();
    unsafe {
        asm!(
            "syscall",
            in("rdi") arg0,
            in("rax") rax,
            options(preserves_flags, nostack, noreturn),
        );
    }
}

#[inline(always)]
pub unsafe fn call2(arg0: Reg, arg1: Reg, num: SysCall) -> Reg {
    let mut rax = num.get();
    unsafe {
        asm!(
            "syscall",
            in("rdi") arg0,
            in("rsi") arg1,
            inout("rax") rax,
            out("r11") _, // syscall clobbers both rcx and r11
            out("rcx") _,
            options(nostack, preserves_flags),
        );
    }
    return rax;
}

#[inline(always)]
pub unsafe fn call3(arg0: Reg, arg1: Reg, arg2: Reg, num: SysCall) -> Reg {
    let mut rax = num.get();
    unsafe {
        asm!(
            "syscall",
            in("rdi") arg0,
            in("rsi") arg1,
            in("rdx") arg2,
            inout("rax") rax,
            out("r11") _, // syscall clobbers both rcx and r11
            out("rcx") _,
            options(nostack, preserves_flags),
        );
    }
    return rax;
}

#[inline(always)]
pub unsafe fn call4(arg0: Reg, arg1: Reg, arg2: Reg, arg3: Reg, num: SysCall) -> Reg {
    let mut rax = num.get();
    unsafe {
        asm!(
            "syscall",
            in("rdi") arg0,
            in("rsi") arg1,
            in("rdx") arg2,
            in("r10") arg3,
            inout("rax") rax,
            out("r11") _, // syscall clobbers both rcx and r11
            out("rcx") _,
            options(nostack, preserves_flags),
        );
    }
    return rax;
}

#[inline(always)]
pub unsafe fn call5(arg0: Reg, arg1: Reg, arg2: Reg, arg3: Reg, arg4: Reg, num: SysCall) -> Reg {
    let mut rax = num.get();
    unsafe {
        asm!(
            "syscall",
            in("rdi") arg0,
            in("rsi") arg1,
            in("rdx") arg2,
            in("r10") arg3,
            in("r8")  arg4,
            inout("rax") rax,
            out("r11") _, // syscall clobbers both rcx and r11
            out("rcx") _,
            options(nostack, preserves_flags),
        );
    }
    return rax;
}
