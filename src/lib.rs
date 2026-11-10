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

/// System limits.
pub use arch::limits;

/// Basic types.
pub mod types {
    pub use crate::arch::posix_types::*;
}
