# rcx
Refined C++ Xpiler

Since Google released carbon-lang, there has been rcx :P

Using cmake:

``cmake -S. -B out -DCMAKE_BUILD_TYPE=Release``
``cmake --build out``

if you want to use GNU compiler in cmake,
cmake can be configured like this:

``CC=/gcc-path/gcc CXX=/gcc-path/g++ CPP=/gcc-path/cpp cmake ...``



Using meson:
``meson setup out``

``meson compile -C out``

``./out/rcx [-o a.out ...] [SOURCE_FILES]``