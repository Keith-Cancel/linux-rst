//! Linux errno values.

use core::ffi::c_int;
use core::num::NonZeroU32;

pub use crate::arch::errno::*;
pub type ErrnoRaw = c_int;
pub type ErrResult<T = ()> = Result<T, ErrnoRaw>;

/// An errno value.
#[repr(transparent)]
#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub struct Errno(NonZeroU32);

impl Errno {
    /// Create a new [`Errno`] from a raw value.
    ///
    /// # Safety
    /// The raw value must not be zero.
    #[inline(always)]
    pub const unsafe fn from_raw_unchecked(e: c_int) -> Self {
        return Errno(unsafe { NonZeroU32::new_unchecked(e as u32) });
    }

    #[inline(always)]
    pub const fn into_raw(self) -> c_int {
        return self.0.get() as c_int;
    }
}

impl From<Errno> for c_int {
    #[inline]
    fn from(value: Errno) -> Self {
        return value.into_raw();
    }
}

impl core::fmt::Debug for Errno {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        // Build the debug representation with only specific fields
        return f
            .debug_tuple("Errno")
            .field(&format_args!("{}: {}", self.0.get(), self.name()))
            .finish();
    }
}

impl Errno {
    pub const fn name(&self) -> &'static str {
        let e = self.0.get() as c_int;
        #[rustfmt::skip]
        let r = match e {
            EPERM   => "EPERM",
            ENOENT  => "ENOENT",
            ESRCH   => "ESRCH",
            EINTR   => "EINTR",
            EIO     => "EIO",
            ENXIO   => "ENXIO",
            E2BIG   => "E2BIG",
            ENOEXEC => "ENOEXEC",
            EBADF   => "EBADF",
            ECHILD  => "ECHILD",
            EAGAIN  => "EAGAIN",
            ENOMEM  => "ENOMEM",
            EACCES  => "EACCES",
            EFAULT  => "EFAULT",
            ENOTBLK => "ENOTBLK",
            EBUSY   => "EBUSY",
            EEXIST  => "EEXIST",
            EXDEV   => "EXDEV",
            ENODEV  => "ENODEV",
            ENOTDIR => "ENOTDIR",
            EISDIR  => "EISDIR",
            EINVAL  => "EINVAL",
            ENFILE  => "ENFILE",
            EMFILE  => "EMFILE",
            ENOTTY  => "ENOTTY",
            ETXTBSY => "ETXTBSY",
            EFBIG   => "EFBIG",
            ENOSPC  => "ENOSPC",
            ESPIPE  => "ESPIPE",
            EROFS   => "EROFS",
            EMLINK  => "EMLINK",
            EPIPE   => "EPIPE",
            EDOM    => "EDOM",
            ERANGE  => "ERANGE",

            EDEADLK         => "EDEADLK",
            ENAMETOOLONG    => "ENAMETOOLONG",
            ENOLCK          => "ENOLCK",
            ENOSYS          => "ENOSYS",
            ENOTEMPTY       => "ENOTEMPTY",
            ELOOP           => "ELOOP",
            ENOMSG          => "ENOMSG",
            EIDRM           => "EIDRM",
            ECHRNG          => "ECHRNG",
            EL2NSYNC        => "EL2NSYNC",
            EL3HLT          => "EL3HLT",
            EL3RST          => "EL3RST",
            ELNRNG          => "ELNRNG",
            EUNATCH         => "EUNATCH",
            ENOCSI          => "ENOCSI",
            EL2HLT          => "EL2HLT",
            EBADE           => "EBADE",
            EBADR           => "EBADR",
            EXFULL          => "EXFULL",
            ENOANO          => "ENOANO",
            EBADRQC         => "EBADRQC",
            EBADSLT         => "EBADSLT",
            EBFONT          => "EBFONT",
            ENOSTR          => "ENOSTR",
            ENODATA         => "ENODATA",
            ETIME           => "ETIME",
            ENOSR           => "ENOSR",
            ENONET          => "ENONET",
            ENOPKG          => "ENOPKG",
            EREMOTE         => "EREMOTE",
            ENOLINK         => "ENOLINK",
            EADV            => "EADV",
            ESRMNT          => "ESRMNT",
            ECOMM           => "ECOMM",
            EPROTO          => "EPROTO",
            EMULTIHOP       => "EMULTIHOP",
            EDOTDOT         => "EDOTDOT",
            EBADMSG         => "EBADMSG",
            EOVERFLOW       => "EOVERFLOW",
            ENOTUNIQ        => "ENOTUNIQ",
            EBADFD          => "EBADFD",
            EREMCHG         => "EREMCHG",
            ELIBACC         => "ELIBACC",
            ELIBBAD         => "ELIBBAD",
            ELIBSCN         => "ELIBSCN",
            ELIBMAX         => "ELIBMAX",
            ELIBEXEC        => "ELIBEXEC",
            EILSEQ          => "EILSEQ",
            ERESTART        => "ERESTART",
            ESTRPIPE        => "ESTRPIPE",
            EUSERS          => "EUSERS",
            ENOTSOCK        => "ENOTSOCK",
            EDESTADDRREQ    => "EDESTADDRREQ",
            EMSGSIZE        => "EMSGSIZE",
            EPROTOTYPE      => "EPROTOTYPE",
            ENOPROTOOPT     => "ENOPROTOOPT",
            EPROTONOSUPPORT => "EPROTONOSUPPORT",
            ESOCKTNOSUPPORT => "ESOCKTNOSUPPORT",
            EOPNOTSUPP      => "EOPNOTSUPP",
            EPFNOSUPPORT    => "EPFNOSUPPORT",
            EAFNOSUPPORT    => "EAFNOSUPPORT",
            EADDRINUSE      => "EADDRINUSE",
            EADDRNOTAVAIL   => "EADDRNOTAVAIL",
            ENETDOWN        => "ENETDOWN",
            ENETUNREACH     => "ENETUNREACH",
            ENETRESET       => "ENETRESET",
            ECONNABORTED    => "ECONNABORTED",
            ECONNRESET      => "ECONNRESET",
            ENOBUFS         => "ENOBUFS",
            EISCONN         => "EISCONN",
            ENOTCONN        => "ENOTCONN",
            ESHUTDOWN       => "ESHUTDOWN",
            ETOOMANYREFS    => "ETOOMANYREFS",
            ETIMEDOUT       => "ETIMEDOUT",
            ECONNREFUSED    => "ECONNREFUSED",
            EHOSTDOWN       => "EHOSTDOWN",
            EHOSTUNREACH    => "EHOSTUNREACH",
            EALREADY        => "EALREADY",
            EINPROGRESS     => "EINPROGRESS",
            ESTALE          => "ESTALE",
            EUCLEAN         => "EUCLEAN",
            ENOTNAM         => "ENOTNAM",
            ENAVAIL         => "ENAVAIL",
            EISNAM          => "EISNAM",
            EREMOTEIO       => "EREMOTEIO",
            EDQUOT          => "EDQUOT",
            ENOMEDIUM       => "ENOMEDIUM",
            EMEDIUMTYPE     => "EMEDIUMTYPE",
            ECANCELED       => "ECANCELED",
            ENOKEY          => "ENOKEY",
            EKEYEXPIRED     => "EKEYEXPIRED",
            EKEYREVOKED     => "EKEYREVOKED",
            EKEYREJECTED    => "EKEYREJECTED",
            EOWNERDEAD      => "EOWNERDEAD",
            ENOTRECOVERABLE => "ENOTRECOVERABLE",
            ERFKILL         => "ERFKILL",
            EHWPOISON       => "EHWPOISON",
            _               => "UNKNOWN",
        };
        return r;
    }

    /// Get the error message [`str`] much like `strerror`.
    pub fn message(&self) -> &'static str {
        let e = self.0.get() as c_int;
        #[rustfmt::skip]
        let s = match e {
            EPERM           => "Operation not permitted",
            ENOENT          => "No such file or directory",
            ESRCH           => "No such process",
            EINTR           => "Interrupted system call",
            EIO             => "Input/output error",
            ENXIO           => "No such device or address",
            E2BIG           => "Argument list too long",
            ENOEXEC         => "Exec format error",
            EBADF           => "Bad file descriptor",
            ECHILD          => "No child processes",
            EAGAIN          => "Resource temporarily unavailable",
            ENOMEM          => "Cannot allocate memory",
            EACCES          => "Permission denied",
            EFAULT          => "Bad address",
            ENOTBLK         => "Block device required",
            EBUSY           => "Device or resource busy",
            EEXIST          => "File exists",
            EXDEV           => "Invalid cross-device link",
            ENODEV          => "No such device",
            ENOTDIR         => "Not a directory",
            EISDIR          => "Is a directory",
            EINVAL          => "Invalid argument",
            ENFILE          => "Too many open files in system",
            EMFILE          => "Too many open files",
            ENOTTY          => "Inappropriate ioctl for device",
            ETXTBSY         => "Text file busy",
            EFBIG           => "File too large",
            ENOSPC          => "No space left on device",
            ESPIPE          => "Illegal seek",
            EROFS           => "Read-only file system",
            EMLINK          => "Too many links",
            EPIPE           => "Broken pipe",
            EDOM            => "Numerical argument out of domain",
            ERANGE          => "Numerical result out of range",
            EDEADLK         => "Resource deadlock avoided",
            ENAMETOOLONG    => "File name too long",
            ENOLCK          => "No locks available",
            ENOSYS          => "Function not implemented",
            ENOTEMPTY       => "Directory not empty",
            ELOOP           => "Too many levels of symbolic links",
            ENOMSG          => "No message of desired type",
            EIDRM           => "Identifier removed",
            ECHRNG          => "Channel number out of range",
            EL2NSYNC        => "Level 2 not synchronized",
            EL3HLT          => "Level 3 halted",
            EL3RST          => "Level 3 reset",
            ELNRNG          => "Link number out of range",
            EUNATCH         => "Protocol driver not attached",
            ENOCSI          => "No CSI structure available",
            EL2HLT          => "Level 2 halted",
            EBADE           => "Invalid exchange",
            EBADR           => "Invalid request descriptor",
            EXFULL          => "Exchange full",
            ENOANO          => "No anode",
            EBADRQC         => "Invalid request code",
            EBADSLT         => "Invalid slot",
            EBFONT          => "Bad font file format",
            ENOSTR          => "Device not a stream",
            ENODATA         => "No data available",
            ETIME           => "Timer expired",
            ENOSR           => "Out of streams resources",
            ENONET          => "Machine is not on the network",
            ENOPKG          => "Package not installed",
            EREMOTE         => "Object is remote",
            ENOLINK         => "Link has been severed",
            EADV            => "Advertise error",
            ESRMNT          => "Srmount error",
            ECOMM           => "Communication error on send",
            EPROTO          => "Protocol error",
            EMULTIHOP       => "Multihop attempted",
            EDOTDOT         => "RFS specific error",
            EBADMSG         => "Bad message",
            EOVERFLOW       => "Value too large for defined data type",
            ENOTUNIQ        => "Name not unique on network",
            EBADFD          => "File descriptor in bad state",
            EREMCHG         => "Remote address changed",
            ELIBACC         => "Can not access a needed shared library",
            ELIBBAD         => "Accessing a corrupted shared library",
            ELIBSCN         => ".lib section in a.out corrupted",
            ELIBMAX         => "Attempting to link in too many shared libraries",
            ELIBEXEC        => "Cannot exec a shared library directly",
            EILSEQ          => "Invalid or incomplete multibyte or wide character",
            ERESTART        => "Interrupted system call should be restarted",
            ESTRPIPE        => "Streams pipe error",
            EUSERS          => "Too many users",
            ENOTSOCK        => "Socket operation on non-socket",
            EDESTADDRREQ    => "Destination address required",
            EMSGSIZE        => "Message too long",
            EPROTOTYPE      => "Protocol wrong type for socket",
            ENOPROTOOPT     => "Protocol not available",
            EPROTONOSUPPORT => "Protocol not supported",
            ESOCKTNOSUPPORT => "Socket type not supported",
            EOPNOTSUPP      => "Operation not supported",
            EPFNOSUPPORT    => "Protocol family not supported",
            EAFNOSUPPORT    => "Address family not supported by protocol",
            EADDRINUSE      => "Address already in use",
            EADDRNOTAVAIL   => "Cannot assign requested address",
            ENETDOWN        => "Network is down",
            ENETUNREACH     => "Network is unreachable",
            ENETRESET       => "Network dropped connection on reset",
            ECONNABORTED    => "Software caused connection abort",
            ECONNRESET      => "Connection reset by peer",
            ENOBUFS         => "No buffer space available",
            EISCONN         => "Transport endpoint is already connected",
            ENOTCONN        => "Transport endpoint is not connected",
            ESHUTDOWN       => "Cannot send after transport endpoint shutdown",
            ETOOMANYREFS    => "Too many references: cannot splice",
            ETIMEDOUT       => "Connection timed out",
            ECONNREFUSED    => "Connection refused",
            EHOSTDOWN       => "Host is down",
            EHOSTUNREACH    => "No route to host",
            EALREADY        => "Operation already in progress",
            EINPROGRESS     => "Operation now in progress",
            ESTALE          => "Stale file handle",
            EUCLEAN         => "Structure needs cleaning",
            ENOTNAM         => "Not a XENIX named type file",
            ENAVAIL         => "No XENIX semaphores available",
            EISNAM          => "Is a named type file",
            EREMOTEIO       => "Remote I/O error",
            EDQUOT          => "Disk quota exceeded",
            ENOMEDIUM       => "No medium found",
            EMEDIUMTYPE     => "Wrong medium type",
            ECANCELED       => "Operation canceled",
            ENOKEY          => "Required key not available",
            EKEYEXPIRED     => "Key has expired",
            EKEYREVOKED     => "Key has been revoked",
            EKEYREJECTED    => "Key was rejected by service",
            EOWNERDEAD      => "Owner died",
            ENOTRECOVERABLE => "State not recoverable",
            ERFKILL         => "Operation not possible due to RF-kill",
            EHWPOISON       => "Memory page has hardware error",
            _               => "Unknown error code",
        };
        return s;
    }
}

#[rustfmt::skip]
impl Errno {
    pub const EPERM:   Self = Self(NonZeroU32::new(EPERM   as u32).unwrap());
    pub const ENOENT:  Self = Self(NonZeroU32::new(ENOENT  as u32).unwrap());
    pub const ESRCH:   Self = Self(NonZeroU32::new(ESRCH   as u32).unwrap());
    pub const EINTR:   Self = Self(NonZeroU32::new(EINTR   as u32).unwrap());
    pub const EIO:     Self = Self(NonZeroU32::new(EIO     as u32).unwrap());
    pub const ENXIO:   Self = Self(NonZeroU32::new(ENXIO   as u32).unwrap());
    pub const E2BIG:   Self = Self(NonZeroU32::new(E2BIG   as u32).unwrap());
    pub const ENOEXEC: Self = Self(NonZeroU32::new(ENOEXEC as u32).unwrap());
    pub const EBADF:   Self = Self(NonZeroU32::new(EBADF   as u32).unwrap());
    pub const ECHILD:  Self = Self(NonZeroU32::new(ECHILD  as u32).unwrap());
    pub const EAGAIN:  Self = Self(NonZeroU32::new(EAGAIN  as u32).unwrap());
    pub const ENOMEM:  Self = Self(NonZeroU32::new(ENOMEM  as u32).unwrap());
    pub const EACCES:  Self = Self(NonZeroU32::new(EACCES  as u32).unwrap());
    pub const EFAULT:  Self = Self(NonZeroU32::new(EFAULT  as u32).unwrap());
    pub const ENOTBLK: Self = Self(NonZeroU32::new(ENOTBLK as u32).unwrap());
    pub const EBUSY:   Self = Self(NonZeroU32::new(EBUSY   as u32).unwrap());
    pub const EEXIST:  Self = Self(NonZeroU32::new(EEXIST  as u32).unwrap());
    pub const EXDEV:   Self = Self(NonZeroU32::new(EXDEV   as u32).unwrap());
    pub const ENODEV:  Self = Self(NonZeroU32::new(ENODEV  as u32).unwrap());
    pub const ENOTDIR: Self = Self(NonZeroU32::new(ENOTDIR as u32).unwrap());
    pub const EISDIR:  Self = Self(NonZeroU32::new(EISDIR  as u32).unwrap());
    pub const EINVAL:  Self = Self(NonZeroU32::new(EINVAL  as u32).unwrap());
    pub const ENFILE:  Self = Self(NonZeroU32::new(ENFILE  as u32).unwrap());
    pub const EMFILE:  Self = Self(NonZeroU32::new(EMFILE  as u32).unwrap());
    pub const ENOTTY:  Self = Self(NonZeroU32::new(ENOTTY  as u32).unwrap());
    pub const ETXTBSY: Self = Self(NonZeroU32::new(ETXTBSY as u32).unwrap());
    pub const EFBIG:   Self = Self(NonZeroU32::new(EFBIG   as u32).unwrap());
    pub const ENOSPC:  Self = Self(NonZeroU32::new(ENOSPC  as u32).unwrap());
    pub const ESPIPE:  Self = Self(NonZeroU32::new(ESPIPE  as u32).unwrap());
    pub const EROFS:   Self = Self(NonZeroU32::new(EROFS   as u32).unwrap());
    pub const EMLINK:  Self = Self(NonZeroU32::new(EMLINK  as u32).unwrap());
    pub const EPIPE:   Self = Self(NonZeroU32::new(EPIPE   as u32).unwrap());
    pub const EDOM:    Self = Self(NonZeroU32::new(EDOM    as u32).unwrap());
    pub const ERANGE:  Self = Self(NonZeroU32::new(ERANGE  as u32).unwrap());

    pub const EDEADLK:         Self = Self(NonZeroU32::new(EDEADLK         as u32).unwrap());
    pub const ENAMETOOLONG:    Self = Self(NonZeroU32::new(ENAMETOOLONG    as u32).unwrap());
    pub const ENOLCK:          Self = Self(NonZeroU32::new(ENOLCK          as u32).unwrap());
    pub const ENOSYS:          Self = Self(NonZeroU32::new(ENOSYS          as u32).unwrap());
    pub const ENOTEMPTY:       Self = Self(NonZeroU32::new(ENOTEMPTY       as u32).unwrap());
    pub const ELOOP:           Self = Self(NonZeroU32::new(ELOOP           as u32).unwrap());
    pub const ENOMSG:          Self = Self(NonZeroU32::new(ENOMSG          as u32).unwrap());
    pub const EIDRM:           Self = Self(NonZeroU32::new(EIDRM           as u32).unwrap());
    pub const ECHRNG:          Self = Self(NonZeroU32::new(ECHRNG          as u32).unwrap());
    pub const EL2NSYNC:        Self = Self(NonZeroU32::new(EL2NSYNC        as u32).unwrap());
    pub const EL3HLT:          Self = Self(NonZeroU32::new(EL3HLT          as u32).unwrap());
    pub const EL3RST:          Self = Self(NonZeroU32::new(EL3RST          as u32).unwrap());
    pub const ELNRNG:          Self = Self(NonZeroU32::new(ELNRNG          as u32).unwrap());
    pub const EUNATCH:         Self = Self(NonZeroU32::new(EUNATCH         as u32).unwrap());
    pub const ENOCSI:          Self = Self(NonZeroU32::new(ENOCSI          as u32).unwrap());
    pub const EL2HLT:          Self = Self(NonZeroU32::new(EL2HLT          as u32).unwrap());
    pub const EBADE:           Self = Self(NonZeroU32::new(EBADE           as u32).unwrap());
    pub const EBADR:           Self = Self(NonZeroU32::new(EBADR           as u32).unwrap());
    pub const EXFULL:          Self = Self(NonZeroU32::new(EXFULL          as u32).unwrap());
    pub const ENOANO:          Self = Self(NonZeroU32::new(ENOANO          as u32).unwrap());
    pub const EBADRQC:         Self = Self(NonZeroU32::new(EBADRQC         as u32).unwrap());
    pub const EBADSLT:         Self = Self(NonZeroU32::new(EBADSLT         as u32).unwrap());
    pub const EBFONT:          Self = Self(NonZeroU32::new(EBFONT          as u32).unwrap());
    pub const ENOSTR:          Self = Self(NonZeroU32::new(ENOSTR          as u32).unwrap());
    pub const ENODATA:         Self = Self(NonZeroU32::new(ENODATA         as u32).unwrap());
    pub const ETIME:           Self = Self(NonZeroU32::new(ETIME           as u32).unwrap());
    pub const ENOSR:           Self = Self(NonZeroU32::new(ENOSR           as u32).unwrap());
    pub const ENONET:          Self = Self(NonZeroU32::new(ENONET          as u32).unwrap());
    pub const ENOPKG:          Self = Self(NonZeroU32::new(ENOPKG          as u32).unwrap());
    pub const EREMOTE:         Self = Self(NonZeroU32::new(EREMOTE         as u32).unwrap());
    pub const ENOLINK:         Self = Self(NonZeroU32::new(ENOLINK         as u32).unwrap());
    pub const EADV:            Self = Self(NonZeroU32::new(EADV            as u32).unwrap());
    pub const ESRMNT:          Self = Self(NonZeroU32::new(ESRMNT          as u32).unwrap());
    pub const ECOMM:           Self = Self(NonZeroU32::new(ECOMM           as u32).unwrap());
    pub const EPROTO:          Self = Self(NonZeroU32::new(EPROTO          as u32).unwrap());
    pub const EMULTIHOP:       Self = Self(NonZeroU32::new(EMULTIHOP       as u32).unwrap());
    pub const EDOTDOT:         Self = Self(NonZeroU32::new(EDOTDOT         as u32).unwrap());
    pub const EBADMSG:         Self = Self(NonZeroU32::new(EBADMSG         as u32).unwrap());
    pub const EOVERFLOW:       Self = Self(NonZeroU32::new(EOVERFLOW       as u32).unwrap());
    pub const ENOTUNIQ:        Self = Self(NonZeroU32::new(ENOTUNIQ        as u32).unwrap());
    pub const EBADFD:          Self = Self(NonZeroU32::new(EBADFD          as u32).unwrap());
    pub const EREMCHG:         Self = Self(NonZeroU32::new(EREMCHG         as u32).unwrap());
    pub const ELIBACC:         Self = Self(NonZeroU32::new(ELIBACC         as u32).unwrap());
    pub const ELIBBAD:         Self = Self(NonZeroU32::new(ELIBBAD         as u32).unwrap());
    pub const ELIBSCN:         Self = Self(NonZeroU32::new(ELIBSCN         as u32).unwrap());
    pub const ELIBMAX:         Self = Self(NonZeroU32::new(ELIBMAX         as u32).unwrap());
    pub const ELIBEXEC:        Self = Self(NonZeroU32::new(ELIBEXEC        as u32).unwrap());
    pub const EILSEQ:          Self = Self(NonZeroU32::new(EILSEQ          as u32).unwrap());
    pub const ERESTART:        Self = Self(NonZeroU32::new(ERESTART        as u32).unwrap());
    pub const ESTRPIPE:        Self = Self(NonZeroU32::new(ESTRPIPE        as u32).unwrap());
    pub const EUSERS:          Self = Self(NonZeroU32::new(EUSERS          as u32).unwrap());
    pub const ENOTSOCK:        Self = Self(NonZeroU32::new(ENOTSOCK        as u32).unwrap());
    pub const EDESTADDRREQ:    Self = Self(NonZeroU32::new(EDESTADDRREQ    as u32).unwrap());
    pub const EMSGSIZE:        Self = Self(NonZeroU32::new(EMSGSIZE        as u32).unwrap());
    pub const EPROTOTYPE:      Self = Self(NonZeroU32::new(EPROTOTYPE      as u32).unwrap());
    pub const ENOPROTOOPT:     Self = Self(NonZeroU32::new(ENOPROTOOPT     as u32).unwrap());
    pub const EPROTONOSUPPORT: Self = Self(NonZeroU32::new(EPROTONOSUPPORT as u32).unwrap());
    pub const ESOCKTNOSUPPORT: Self = Self(NonZeroU32::new(ESOCKTNOSUPPORT as u32).unwrap());
    pub const EOPNOTSUPP:      Self = Self(NonZeroU32::new(EOPNOTSUPP      as u32).unwrap());
    pub const EPFNOSUPPORT:    Self = Self(NonZeroU32::new(EPFNOSUPPORT    as u32).unwrap());
    pub const EAFNOSUPPORT:    Self = Self(NonZeroU32::new(EAFNOSUPPORT    as u32).unwrap());
    pub const EADDRINUSE:      Self = Self(NonZeroU32::new(EADDRINUSE      as u32).unwrap());
    pub const EADDRNOTAVAIL:   Self = Self(NonZeroU32::new(EADDRNOTAVAIL   as u32).unwrap());
    pub const ENETDOWN:        Self = Self(NonZeroU32::new(ENETDOWN        as u32).unwrap());
    pub const ENETUNREACH:     Self = Self(NonZeroU32::new(ENETUNREACH     as u32).unwrap());
    pub const ENETRESET:       Self = Self(NonZeroU32::new(ENETRESET       as u32).unwrap());
    pub const ECONNABORTED:    Self = Self(NonZeroU32::new(ECONNABORTED    as u32).unwrap());
    pub const ECONNRESET:      Self = Self(NonZeroU32::new(ECONNRESET      as u32).unwrap());
    pub const ENOBUFS:         Self = Self(NonZeroU32::new(ENOBUFS         as u32).unwrap());
    pub const EISCONN:         Self = Self(NonZeroU32::new(EISCONN         as u32).unwrap());
    pub const ENOTCONN:        Self = Self(NonZeroU32::new(ENOTCONN        as u32).unwrap());
    pub const ESHUTDOWN:       Self = Self(NonZeroU32::new(ESHUTDOWN       as u32).unwrap());
    pub const ETOOMANYREFS:    Self = Self(NonZeroU32::new(ETOOMANYREFS    as u32).unwrap());
    pub const ETIMEDOUT:       Self = Self(NonZeroU32::new(ETIMEDOUT       as u32).unwrap());
    pub const ECONNREFUSED:    Self = Self(NonZeroU32::new(ECONNREFUSED    as u32).unwrap());
    pub const EHOSTDOWN:       Self = Self(NonZeroU32::new(EHOSTDOWN       as u32).unwrap());
    pub const EHOSTUNREACH:    Self = Self(NonZeroU32::new(EHOSTUNREACH    as u32).unwrap());
    pub const EALREADY:        Self = Self(NonZeroU32::new(EALREADY        as u32).unwrap());
    pub const EINPROGRESS:     Self = Self(NonZeroU32::new(EINPROGRESS     as u32).unwrap());
    pub const ESTALE:          Self = Self(NonZeroU32::new(ESTALE          as u32).unwrap());
    pub const EUCLEAN:         Self = Self(NonZeroU32::new(EUCLEAN         as u32).unwrap());
    pub const ENOTNAM:         Self = Self(NonZeroU32::new(ENOTNAM         as u32).unwrap());
    pub const ENAVAIL:         Self = Self(NonZeroU32::new(ENAVAIL         as u32).unwrap());
    pub const EISNAM:          Self = Self(NonZeroU32::new(EISNAM          as u32).unwrap());
    pub const EREMOTEIO:       Self = Self(NonZeroU32::new(EREMOTEIO       as u32).unwrap());
    pub const EDQUOT:          Self = Self(NonZeroU32::new(EDQUOT          as u32).unwrap());
    pub const ENOMEDIUM:       Self = Self(NonZeroU32::new(ENOMEDIUM       as u32).unwrap());
    pub const EMEDIUMTYPE:     Self = Self(NonZeroU32::new(EMEDIUMTYPE     as u32).unwrap());
    pub const ECANCELED:       Self = Self(NonZeroU32::new(ECANCELED       as u32).unwrap());
    pub const ENOKEY:          Self = Self(NonZeroU32::new(ENOKEY          as u32).unwrap());
    pub const EKEYEXPIRED:     Self = Self(NonZeroU32::new(EKEYEXPIRED     as u32).unwrap());
    pub const EKEYREVOKED:     Self = Self(NonZeroU32::new(EKEYREVOKED     as u32).unwrap());
    pub const EKEYREJECTED:    Self = Self(NonZeroU32::new(EKEYREJECTED    as u32).unwrap());
    pub const EOWNERDEAD:      Self = Self(NonZeroU32::new(EOWNERDEAD      as u32).unwrap());
    pub const ENOTRECOVERABLE: Self = Self(NonZeroU32::new(ENOTRECOVERABLE as u32).unwrap());
    pub const ERFKILL:         Self = Self(NonZeroU32::new(ERFKILL         as u32).unwrap());
    pub const EHWPOISON:       Self = Self(NonZeroU32::new(EHWPOISON       as u32).unwrap());
}
