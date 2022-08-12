use std::{fs::{read_dir}, path::{PathBuf}, env};

use cc::Build;

fn dfs_dir(folder_name: &str, target_filelist: &mut Vec<PathBuf>) -> Result<(), Box<dyn std::error::Error>> {
  for files in read_dir(&folder_name)? {
    let filename = files.unwrap().file_name();
    let file = PathBuf::from(&filename);

    if file.is_dir() {
      dfs_dir(filename.to_str().unwrap(), target_filelist)?
    }

    let filename = filename.to_str().unwrap();
    
    if filename.ends_with(".cxx") {
      target_filelist.push(PathBuf::from(format!("{}/{}", folder_name, filename)));
    }
  }

  Ok(())
}

fn main() -> Result<(), Box<dyn std::error::Error>> {

  const COMPILE_DIR: &str = "./src";

  let mut compile_target_files: Vec<PathBuf> = Vec::new();  
  
  dfs_dir(COMPILE_DIR, &mut compile_target_files)?;

  // for it in compile_target_files.iter() {
  //   println!("F: {}", it.to_str().unwrap().to_string());
  // }

  Build::new()
    .cpp(true)
    .files(compile_target_files)
    .include(COMPILE_DIR)
    .include("./subprojects/spdlog/include")
    .flag("-std=c++17")
    .flag("-Wextra")
    .flag("-Wpedantic")
    .flag("-g")
    .flag("-Wno-unused-parameter")
    .flag("-fno-rtti")
    .compile("librcx.a");

  bindgen::builder()
    .generate().expect("Failed to generate rcx bindings")
    .write_to_file(PathBuf::from(env::var("OUT_DIR")?).join("bindings.rs"))
    .expect("Failed to write bindings");

  Ok(())
}