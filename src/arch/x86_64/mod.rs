mod sys;

pub use sys::*;

use super::def_sys_call;
use super::generic;

pub mod errno {
    // No over-rides
    pub use super::generic::errno::*;
}

pub mod fcntl {
    // No over-rides
    pub use super::generic::fcntl::*;
}

#[allow(unused)]
pub mod limits {
    // No over-rides
    pub use super::generic::limits::*;
}

pub mod posix_types {
    // No over-rides yet.
    pub use super::generic::posix_types::*;
}
