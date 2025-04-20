unsafe extern "C" {
}
pub unsafe fn f() {
let mut i: i32;
i = 0;
while i < 100 {
f();
++i
};
}
