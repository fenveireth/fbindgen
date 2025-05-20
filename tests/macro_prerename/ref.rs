unsafe extern "C" {
	pub fn prerenamed_fun_1();
}
pub const prerenamed_fun: unsafe extern "C" fn() = prerenamed_fun_1;
