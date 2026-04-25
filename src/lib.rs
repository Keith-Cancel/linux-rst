#![no_std]
#![cfg(all(
    target_os = "linux",
    any(target_arch = "x86_64", target_arch = "aarch64", target_arch = "riscv64")
))]

mod arch;

/// Linux errno values.
pub use crate::arch::errno;

/// Basic types.
pub mod types {
    pub use crate::arch::posix_types::*;

    pub type ErrResult<T> = Result<T, core::ffi::c_int>;
}
