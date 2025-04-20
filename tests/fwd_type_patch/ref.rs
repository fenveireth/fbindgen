#[repr(C)]
pub struct T {
	pub member: i32,
}
pub type r1 = Option<extern "C" fn(*mut T)>;
unsafe extern "C" {
	pub fn r2(r#ref: *mut T);
}
pub unsafe fn r3() {
let mut ptr: *mut T = std::ptr::null_mut::<T>();
}
