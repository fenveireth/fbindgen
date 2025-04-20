#[repr(C)]
pub union anon_0 {
	pub x: core::mem::ManuallyDrop<i32>,
	pub y: core::mem::ManuallyDrop<f64>,
}
#[repr(C)]
pub union anon_1 {
	pub z: core::mem::ManuallyDrop<i32>,
	pub w: core::mem::ManuallyDrop<i32>,
}
#[repr(C)]
pub struct A {
	pub u: anon_0,
	pub anon_2: anon_1,
}
