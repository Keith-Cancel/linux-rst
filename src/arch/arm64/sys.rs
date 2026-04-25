use core::arch::asm;

use super::sys_call;

/// System call register.
pub type Reg = u64;

// System call number
// https://chromium.googlesource.com/chromiumos/docs/+/master/constants/syscalls.md#arm64-64_bit
sys_call! {
    read:       63,
    write:      64,
    close:      57,
    open_at:    56,
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
    let ret;
    unsafe {
        asm!(
            "svc #0",
            in("x8") num.get(),
            out("x0") ret,
            options(nostack, preserves_flags),
        );
    }
    return ret;
}

#[inline(always)]
pub unsafe fn call1(mut arg0: Reg, num: SysCall) -> Reg {
    unsafe {
        asm!(
            "svc #0",
            in("x8") num.get(),
            inout("x0") arg0,
            options(nostack, preserves_flags),
        );
    }
    return arg0;
}

#[inline(always)]
pub unsafe fn call1_no_return(arg0: Reg, num: SysCall) -> ! {
    unsafe {
        asm!(
            "svc #0",
            in("x8") num.get(),
            in("x0") arg0,
            options(preserves_flags, nostack, noreturn),
        );
    }
}

#[inline(always)]
pub unsafe fn call2(mut arg0: Reg, arg1: Reg, num: SysCall) -> Reg {
    unsafe {
        asm!(
            "svc #0",
            in("x8") num.get(),
            inout("x0") arg0,
            in("x1") arg1,
            options(nostack, preserves_flags),
        );
    }
    return arg0;
}

#[inline(always)]
pub unsafe fn call3(mut arg0: Reg, arg1: Reg, arg2: Reg, num: SysCall) -> Reg {
    unsafe {
        asm!(
            "svc #0",
            in("x8") num.get(),
            inout("x0") arg0,
            in("x1") arg1,
            in("x2") arg2,
            options(nostack, preserves_flags),
        );
    }
    return arg0;
}

#[inline(always)]
pub unsafe fn call4(mut arg0: Reg, arg1: Reg, arg2: Reg, arg3: Reg, num: SysCall) -> Reg {
    unsafe {
        asm!(
            "svc #0",
            in("x8") num.get(),
            inout("x0") arg0,
            in("x1") arg1,
            in("x2") arg2,
            in("x3") arg3,
            options(nostack, preserves_flags),
        );
    }
    return arg0;
}

#[inline(always)]
pub unsafe fn call5(mut arg0: Reg, arg1: Reg, arg2: Reg, arg3: Reg, arg4: Reg, num: SysCall) -> Reg {
    unsafe {
        asm!(
            "svc #0",
            in("x8") num.get(),
            inout("x0") arg0,
            in("x1") arg1,
            in("x2") arg2,
            in("x3") arg3,
            in("x4") arg4,
            options(nostack, preserves_flags),
        );
    }
    return arg0;
}
