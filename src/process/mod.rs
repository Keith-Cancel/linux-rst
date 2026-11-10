//! Functions and constants related to controlling processes.
use core::ffi::c_int;

use crate::arch::Reg;
use crate::arch::SysCall;
use crate::arch::call1_no_return;

/// Exit all threads in a process.
///
/// This directly invokes the linux exit_group syscall.
/// # Safety
/// This function immediately exists and because
/// it does not try to run any cleanup code or
/// anything is marked unsafe.
pub unsafe fn exit_group(code: c_int) -> ! {
    unsafe { call1_no_return(code as Reg, SysCall::EXIT_GROUP) };
}
