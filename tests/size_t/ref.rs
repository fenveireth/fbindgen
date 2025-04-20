pub type off_t = i64;
unsafe extern "C" {
	pub fn mmap(addr: *mut std::ffi::c_void, length: usize, prot: i32, flags: i32, fd: i32, offset: off_t) -> *mut std::ffi::c_void;
}
