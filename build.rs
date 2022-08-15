use std::{fs::{read_dir}, path::{PathBuf}, env};

use cc::Build;

fn dfs_dir(folder_name: &str, target_filelist: &mut Vec<PathBuf>) -> Result<(), Box<dyn std::error::Error>> {
  let current_dir = read_dir(&folder_name)?.map(|a| a.unwrap());
  for file in current_dir {
    let filename: String = file.file_name().to_str().unwrap().to_string();
    let filepath: String = format!("{}/{}", &folder_name, &filename).to_string();
    
    let file_pb = PathBuf::from(format!("{}/{}", &folder_name, &filename));
    if file_pb.is_dir() {
      dfs_dir(filepath.as_str(), target_filelist)?;

      continue;
    }

    if filename.ends_with(".cxx") {
      target_filelist.push(file_pb);
    }
  }

  Ok(())
}

fn main() -> Result<(), Box<dyn std::error::Error>> {

  const COMPILE_DIR: &str = "./src";

  let mut compile_target_files: Vec<PathBuf> = Vec::new();  
  
  dfs_dir(COMPILE_DIR, &mut compile_target_files)?;

  let target_out_dir: PathBuf = PathBuf::from(env::var("OUT_DIR")?).canonicalize().unwrap();  

  println!("-L{}", target_out_dir.to_string_lossy());

  Build::new()
    .cpp(true)
    .files(compile_target_files)
    .include(COMPILE_DIR)
    .include("/usr/local/include")
    .include("./subprojects/spdlog/include")
    .flag("-std=c++17")
    .flag("-Wpedantic")
    .flag("-Wno-unused-parameter")
    .flag("-fno-rtti")
    .flag("-g")
    .compile("rcx");

  println!("cargo:rustc-link-search=/usr/local/lib");
  println!("cargo:rustc-link-lib=LLVM");
  println!("cargo:rustc-link-lib=boost_program_options");
  println!("cargo:rustc-link-lib=clang");
  println!("cargo:rustc-link-lib=clangAST");
  println!("cargo:rustc-link-lib=clangAnalysis");
  println!("cargo:rustc-link-lib=clangBasic");
  println!("cargo:rustc-link-lib=clangCodeGen");
  println!("cargo:rustc-link-lib=clangDriver");
  println!("cargo:rustc-link-lib=clangEdit");
  println!("cargo:rustc-link-lib=clangFrontend");
  println!("cargo:rustc-link-lib=clangLex");
  println!("cargo:rustc-link-lib=clangParse");
  println!("cargo:rustc-link-lib=clangRewrite");
  println!("cargo:rustc-link-lib=clangSema");
  println!("cargo:rustc-link-lib=clangSerialization");

    bindgen::Builder::default()
    .allowlist_recursively(true)
    .allowlist_function("rcx_main")
    .header("./src/rcx.h")
    .clang_args(
      [ "-std=c17"
      , "-g"
      // , "-L/usr/local/lib"
      // , "-lLLVM"
      // , "-lboost_program_options"
      // , "-lclang"
      // , "-lclangAST"
      // , "-lclangAnalysis"
      // , "-lclangBasic"
      // , "-lclangCodeGen"
      // , "-lclangDriver"
      // , "-lclangEdit"
      // , "-lclangFrontend"
      // , "-lclangLex"
      // , "-lclangParse"
      // , "-lclangRewrite"
      // , "-lclangSema"
      // , "-lclangSerialization"
      ] )
    .parse_callbacks(Box::new(bindgen::CargoCallbacks))
    .generate()
    .expect("Failed to generate rcx bindings")
    .write_to_file(target_out_dir.join("bindings.rs"))
    .expect("Failed to write bindings");

  Ok(())
}