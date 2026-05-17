use core::arch::asm;

use super::def_sys_call;

/// System call register.
pub type Reg = u64;

// System call number
// https://gpages.juszkiewicz.com.pl/syscalls-table/syscalls.html
def_sys_call! {
    read:       63,
    write:      64,
    close:      57,
    dup:        23,
    open_at:    56,
    mkdir_at:   34,
    unlink_at:  35,
    exit:       93,
    exit_group: 94,
    socket:     198,
    connect:    203,
    accept:     202,
    bind:       200,
    listen:     201,
    prctl:      167,
}

#[inline(always)]
pub unsafe fn call0(num: SysCall) -> Reg {
    let a0;
    unsafe {
        asm!(
            "ecall",
            in("a7") num.get(),
            out("a0") a0,
            options(nostack, preserves_flags),
        );
    }
    return a0;
}

#[inline(always)]
pub unsafe fn call1(mut arg0: Reg, num: SysCall) -> Reg {
    unsafe {
        asm!(
            "ecall",
            in("a7") num.get(),
            inout("a0") arg0,
            options(nostack, preserves_flags),
        );
    }
    return arg0;
}

#[inline(always)]
pub unsafe fn call1_no_return(arg0: Reg, num: SysCall) -> ! {
    unsafe {
        asm!(
            "ecall",
            in("a7") num.get(),
            in("a0") arg0,
            options(nostack, preserves_flags, noreturn),
        );
    }
}

#[inline(always)]
pub unsafe fn call2(mut arg0: Reg, arg1: Reg, num: SysCall) -> Reg {
    unsafe {
        asm!(
            "ecall",
            in("a7") num.get(),
            inout("a0") arg0,
            in("a1") arg1,
            options(nostack, preserves_flags),
        );
    }
    return arg0;
}

#[inline(always)]
pub unsafe fn call3(mut arg0: Reg, arg1: Reg, arg2: Reg, num: SysCall) -> Reg {
    unsafe {
        asm!(
            "ecall",
            in("a7") num.get(),
            inout("a0") arg0,
            in("a1") arg1,
            in("a2") arg2,
            options(nostack, preserves_flags),
        );
    }
    return arg0;
}

#[inline(always)]
pub unsafe fn call4(mut arg0: Reg, arg1: Reg, arg2: Reg, arg3: Reg, num: SysCall) -> Reg {
    unsafe {
        asm!(
            "ecall",
            in("a7") num.get(),
            inout("a0") arg0,
            in("a1") arg1,
            in("a2") arg2,
            in("a3") arg3,
            options(nostack, preserves_flags),
        );
    }
    return arg0;
}

#[inline(always)]
pub unsafe fn call5(mut arg0: Reg, arg1: Reg, arg2: Reg, arg3: Reg, arg4: Reg, num: SysCall) -> Reg {
    unsafe {
        asm!(
            "ecall",
            in("a7") num.get(),
            inout("a0") arg0,
            in("a1") arg1,
            in("a2") arg2,
            in("a3") arg3,
            in("a4") arg4,
            options(nostack, preserves_flags),
        );
    }
    return arg0;
}
