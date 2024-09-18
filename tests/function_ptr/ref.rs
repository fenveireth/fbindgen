pub type sqlite3_destructor_type = Option<extern "C" fn(*mut std::ffi::c_void)>;
pub const SQLITE_STATIC: sqlite3_destructor_type = None;
extern {
}
