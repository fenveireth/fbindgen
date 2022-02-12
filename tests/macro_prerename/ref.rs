extern {
	pub fn prerenamed_fun_1();
}
pub const prerenamed_fun: unsafe extern fn() = prerenamed_fun_1;
