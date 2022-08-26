use std::{ffi::CString, env::args};

use clap::{ Arg, Command };

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

fn main() {
    let argv: Vec<CString> = args().map(|a| CString::new(a).unwrap()).collect();
    let argv: Vec<*const i8> = argv.iter().map(|a| a.as_ptr()).collect();
    let argc: i32 = argv.len() as i32;

    let matches =
        Command::new("rcx")
            .about("Refined CXX Xpiler")
            .version("1.0.0")
            .subcommand_required(false)
            .arg_required_else_help(true)
            .author("Endless Arch")
            .arg(
                Arg::new("if")
                    .short('s')
                    .long("source")
                    .help("Specify compile target input file")
                    .action(clap::ArgAction::Set)
                    .number_of_values(1)
                    .required(true)
            )
            .arg(
                Arg::new("of")
                    .short('o')
                    .long("output")
                    .help("Specify output file name")
                    .action(clap::ArgAction::Set)
                    .default_value("./rcx_program.out")
            )
            .get_matches();

    if matches.contains_id("if") {
        println!("if: {}", matches.value_of("if").unwrap());
    }
    if matches.contains_id("of") {
        println!("of: {}", matches.value_of("of").unwrap());
    }

    std::process::exit(
        unsafe { rcx_main(argc, argv.as_ptr()) })
}