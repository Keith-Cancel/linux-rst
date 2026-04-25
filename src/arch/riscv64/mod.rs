mod sys;

pub use sys::*;

use super::generic;
use super::sys_call;

#[allow(unused)]
pub mod errno {
    // No over-rides
    pub use super::generic::errno::*;
}

#[allow(unused)]
pub mod fcntl {
    // No over-rides
    pub use super::generic::fcntl::*;
}

#[allow(unused)]
pub mod limits {
    // No over-rides
    pub use super::generic::limits::*;
}
