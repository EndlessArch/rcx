cmake -S. -Bout -DCMAKE_PREFIX_PATH=/usr/lib/llvm15 -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build out --parallel
