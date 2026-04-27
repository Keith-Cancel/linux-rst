#![no_std]
#![allow(clippy::needless_return)]
#![cfg(all(
    target_os = "linux",
    any(target_arch = "x86_64", target_arch = "aarch64", target_arch = "riscv64")
))]

mod arch;

pub mod fs;

pub use crate::arch::errno;

/// Basic types.
pub mod types {
    pub use crate::arch::posix_types::*;
}

/// Linux errno values.
pub mod error {
    use core::ffi::c_int;

    pub use crate::arch::errno::*;
    pub type ErrNo = c_int;
    pub type ErrResult<T = ()> = Result<T, ErrNo>;
}
