pub type pRENDERDOC_GetCaptureFilePathTemplate = Option<extern "C" fn() -> *const i8>;
pub type pRENDERDOC_GetLogFilePathTemplate = pRENDERDOC_GetCaptureFilePathTemplate;
