unsafe extern "C" {
}
pub unsafe fn no_extra_parens_on_cast() {
fun(std::mem::transmute::<i32, u32>(1 << 0));
}
