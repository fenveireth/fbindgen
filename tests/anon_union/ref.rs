#[repr(C)]
pub union anon_0 {
  pub x: core::mem::ManuallyDrop<i32>,
  pub y: core::mem::ManuallyDrop<f64>,
}
#[repr(C)]
pub struct A {
  pub u: anon_0,
}
extern {
}
