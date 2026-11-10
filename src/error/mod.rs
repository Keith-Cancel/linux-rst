//! Linux errno values.

use core::ffi::c_int;

pub use crate::arch::errno::*;
pub type ErrnoRaw = c_int;
pub type ErrResult<T = ()> = Result<T, ErrnoRaw>;
