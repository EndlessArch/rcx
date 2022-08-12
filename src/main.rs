use std::{ffi::CString, env::args};

extern "C" {

fn rcx_main(argc: i32, argv: *const *const i8) -> i32;

}

fn main() {
    let argv: Vec<CString> = args().map(|a| CString::new(a).unwrap()).collect();
    let argv: Vec<*const i8> = argv.iter().map(|a| a.as_ptr()).collect();
    let argc: i32 = argv.len() as i32;

    let exit_code = unsafe { rcx_main(argc, argv.as_ptr()) };
    std::process::exit(exit_code)
}