#![allow(unused)]

// https://github.com/torvalds/linux/blob/master/include/uapi/linux/limits.h

/// Max character limit for a file name
pub const NAME_MAX: usize = 255;
/// Max character in a path name including null terminator.
pub const PATH_MAX: usize = 4096;
