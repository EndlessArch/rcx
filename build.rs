use std::{fs::{read_dir}, path::{PathBuf}, env};

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

  cc::Build::new()
    .cpp(true)
    .files(compile_target_files)
    .include(COMPILE_DIR)
    .include("/usr/local/include")
    .include("./subprojects/spdlog/include")
    .flag("-std=c++17")
    .flag("-Wpedantic")
    .flag("-Wno-unused-parameter")
    .flag("-fno-rtti")
    // .flag("-g")
    // .flag("-v")
    .compile("rcx");

  println!("cargo:rustc-link-search=/usr/local/lib");
  let target_link_libs: Vec<&str> = vec![
    "LLVM",
    "clang",
    "clangAST",
    "clangAnalysis",
    "clangBasic",
    "clangDriver",
    "clangEdit",
    "clangFrontend",
    "clangLex",
    "clangParse",
    "clangRewrite",
    "clangSema",
    "clangSerialization"
  ];
  for lib in target_link_libs.iter() {
    println!("cargo:rustc-link-lib={}", lib);
  }

  let target_out_dir: PathBuf = PathBuf::from(env::var("OUT_DIR")?).canonicalize().unwrap();  

  bindgen::Builder::default()
    // .allowlist_recursively(true)
    // .allowlist_function("rcx_main")
    .header(format!("{}/rcx.h", COMPILE_DIR))
    .clang_args(
      [ "-std=c17"
      // , "-g"
      // , "-v"
      ] )
    .parse_callbacks(Box::new(bindgen::CargoCallbacks))
    .generate()
    .expect("Failed to generate rcx bindings")
    .write_to_file(target_out_dir.join("bindings.rs"))
    .expect("Failed to write bindings");

  Ok(())
}