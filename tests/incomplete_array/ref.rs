#[repr(C)]
pub struct cmsghdr {
	pub cmsg_len: usize,
	pub cmsg_level: i32,
	pub cmsg_type: i32,
	pub __cmsg_data: [u8; 0],
}
extern {
}
