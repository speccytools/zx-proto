mkdir build
cd build
cmake -G "Unix Makefiles" -DZCCTARGET=zx -DCMAKE_TOOLCHAIN_FILE=${ZCCCFG}/../../cmake/Toolchain-zcc.cmake ..
make example_terminal_client