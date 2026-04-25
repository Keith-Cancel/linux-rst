mod generic;

use decl_macros::config_match;

config_match! {
    #[cfg(target_arch = "x86_64")] => {
        mod x86_64;
        pub use x86_64::*;
    },
    #[cfg(target_arch = "aarch64")] => {
        mod arm64;
        pub use arm64::*;
    },
    #[cfg(target_arch = "riscv64")] => {
        mod riscv64;
        pub use riscv64::*;
    },
    _ => {
        compile_error!{"Unsupported architecture linux currently supports only \"x86_64\", \"ARM64\" and \"RISCV-64\"."}
    }
}

#[rustfmt::skip]
macro_rules! sys_call {
    (
        read: $read:expr,
        write: $write:expr,
        close: $close:expr,
        open_at: $open_at:expr,
        unlink_at: $unlink_at:expr,
        exit: $exit:expr,
        exit_group: $exit_grp:expr,
        socket: $socket:expr,
        connect: $connect:expr,
        accept: $accept:expr,
        bind: $bind:expr,
        listen: $listen:expr,
        prctl: $prctl:expr
        $(,)?
    ) => {
        /// A system call number
        #[derive(Copy, Clone, Eq, PartialEq, PartialOrd, Ord)]
        #[repr(transparent)]
        pub struct SysCall(Reg);

        #[allow(unused)]
        impl SysCall {
            pub const READ:       Self = Self($read);
            pub const WRITE:      Self = Self($write);
            pub const CLOSE:      Self = Self($close);
            pub const OPEN_AT:    Self = Self($open_at);
            pub const UNLINK_AT:  Self = Self($unlink_at);
            pub const EXIT:       Self = Self($exit);
            pub const EXIT_GROUP: Self = Self($exit_grp);
            pub const SOCKET:     Self = Self($socket);
            pub const CONNECT:    Self = Self($connect);
            pub const ACCEPT:     Self = Self($accept);
            pub const BIND:       Self = Self($bind);
            pub const LISTEN:     Self = Self($listen);
            pub const PRCTL:      Self = Self($prctl);
            
            //pub const FSYNC: Self = Self(74);
            //pub const READLINKAT: Self = Self(267);
            //pub const STATX: Self = Self(332);
            #[inline(always)]
            pub const fn get(self) -> Reg {
                return self.0;
            }
        }
    };
}
pub(crate) use sys_call;
