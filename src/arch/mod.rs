mod generic;

cfg_select! {
    target_arch = "x86_64" => {
        mod x86_64;
        pub use x86_64::*;
    },
    target_arch = "aarch64" => {
        mod arm64;
        pub use arm64::*;
    },
    target_arch = "riscv64" => {
        mod riscv64;
        pub use riscv64::*;
    },
    _ => {
        compile_error! {"Unsupported architecture linux currently supports only \"x86_64\", \"ARM64\" and \"RISCV-64\"."}
    },
}

#[rustfmt::skip]
macro_rules! def_sys_call {
    (
        read:       $read:expr,
        write:      $write:expr,
        close:      $close:expr,
        dup:        $dup:expr,
        open_at:    $open_at:expr,
        mkdir_at:   $mkdir_at:expr,
        unlink_at:  $unlink_at:expr,
        exit:       $exit:expr,
        exit_group: $exit_grp:expr,
        socket:     $socket:expr,
        connect:    $connect:expr,
        accept:     $accept:expr,
        bind:       $bind:expr,
        listen:     $listen:expr,
        prctl:      $prctl:expr
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
            pub const DUP:        Self = Self($dup);
            pub const OPEN_AT:    Self = Self($open_at);
            pub const MKDIR_AT:   Self = Self($mkdir_at);
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
            /// Get the system call number
            #[inline(always)]
            pub const fn get(self) -> Reg {
                return self.0;
            }
            /// Create a new system call from a raw number.
            #[inline(always)]
            pub const fn new_raw(no: Reg) -> Self {
                return Self(no)
            }
        }
    };
}
pub(crate) use def_sys_call;

macro_rules! syscall {
    ($num:ident) => {
        crate::arch::call0(crate::arch::SysCall::$num)
    };
    ($arg0:expr, $num:ident) => {{ unsafe { crate::arch::call1($arg0 as crate::arch::Reg, crate::arch::SysCall::$num) } }};
    ($arg0:expr, $arg1:expr, $num:ident) => {{
        unsafe {
            crate::arch::call2(
                $arg0 as crate::arch::Reg,
                $arg1 as crate::arch::Reg,
                crate::arch::SysCall::$num,
            )
        }
    }};
    ($arg0:expr, $arg1:expr, $arg2:expr, $num:ident) => {{
        unsafe {
            crate::arch::call3(
                $arg0 as crate::arch::Reg,
                $arg1 as crate::arch::Reg,
                $arg2 as crate::arch::Reg,
                crate::arch::SysCall::$num,
            )
        }
    }};
    ($arg0:expr, $arg1:expr, $arg2:expr, $arg3:expr, $num:ident) => {{
        unsafe {
            crate::arch::call4(
                $arg0 as crate::arch::Reg,
                $arg1 as crate::arch::Reg,
                $arg2 as crate::arch::Reg,
                $arg3 as crate::arch::Reg,
                crate::arch::SysCall::$num,
            )
        }
    }};
    ($arg0:expr, $arg1:expr, $arg2:expr, $arg3:expr, $arg4:expr, $num:ident) => {{
        unsafe {
            crate::arch::call5(
                $arg0 as crate::arch::Reg,
                $arg1 as crate::arch::Reg,
                $arg2 as crate::arch::Reg,
                $arg3 as crate::arch::Reg,
                $arg4 as crate::arch::Reg,
                crate::arch::SysCall::$num,
            )
        }
    }};
}
pub(crate) use syscall;
