#![no_std]
#![allow(clippy::needless_return)]
#![cfg(all(
    target_os = "linux",
    any(target_arch = "x86_64", target_arch = "aarch64", target_arch = "riscv64")
))]

mod arch;

pub mod error;
pub mod fd;
pub mod fs;
pub mod io;
pub mod process;

/// System limits.
pub use arch::limits;

/// Basic types.
pub mod types {
    pub use crate::arch::posix_types::*;
}

/// Raw system call functionality.
pub mod sys {
    /// Native register type.
    pub use crate::arch::Reg;
    pub use crate::arch::SysCall;
    /// Perform a raw system call with no arguments.
    pub use crate::arch::call0;
    /// Perform a raw system call with one argument.
    pub use crate::arch::call1;
    /// Perform a raw system call with two arguments.
    pub use crate::arch::call2;
    /// Perform a raw system call with three arguments.
    pub use crate::arch::call3;
    /// Perform a raw system call with four arguments.
    pub use crate::arch::call4;
    /// Perform a raw system call with five arguments.
    pub use crate::arch::call5;
}
