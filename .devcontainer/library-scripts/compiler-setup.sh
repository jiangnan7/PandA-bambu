#!/bin/bash
###
#     Build environment setup
#
#     Download requested compilers and generate neccessary symlinks to enable ccache.
#     The script takes a single argument containing a comma separated list of compilers.
#     It is safe to call the script multiple times, but be aware that installing older
#     versions of GCC compilers after newer ones may cause compatibility issues.
###
set -e
export XZ_OPT='-T0'

install="$1"

inflate() {
   echo "Installing $1"
   case $1 in
      clang-12 )
         wget https://github.com/llvm/llvm-project/releases/download/llvmorg-12.0.1/clang+llvm-12.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz -nv -O - | tar -C $2 -xJf - &
         ;;
      clang-11 )
         wget https://github.com/llvm/llvm-project/releases/download/llvmorg-11.1.0/clang+llvm-11.1.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz -nv -O - | tar -C $2 -xJf - &
         ;;
      clang-10 )
         wget https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.1/clang+llvm-10.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz -nv -O - | tar -C $2 -xJf - &
         ;;
      clang-9 )
         wget https://github.com/llvm/llvm-project/releases/download/llvmorg-9.0.1/clang+llvm-9.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz -nv -O - | tar -C $2 -xJf - &
         ;;
      clang-8 )
         wget https://releases.llvm.org/8.0.0/clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz -nv -O - | tar -C $2 -xJf - &
         ;;
      clang-7 )
         wget https://releases.llvm.org/7.0.1/clang+llvm-7.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz -nv -O - | tar -C $2 -xJf - &
         ;;
      clang-6 )
         wget https://releases.llvm.org/6.0.1/clang+llvm-6.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz -nv -O - | tar -C $2 -xJf - &
         ;;
      clang-5 )
         wget https://releases.llvm.org/5.0.2/clang+llvm-5.0.2-x86_64-linux-gnu-ubuntu-16.04.tar.xz -nv -O - | tar -C $2 -xJf - &
         ;;
      clang-4 )
         wget https://releases.llvm.org/4.0.0/clang+llvm-4.0.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz -nv -O - | tar -C $2 -xJf - &
         ;;
      gcc-4.5 )
         wget https://release.bambuhls.eu:8080/compiler/gcc-4.5-bambu-Ubuntu_16.04.tar.xz --no-check-certificate -nv -O - | tar -C $2 -xJf - 
         ;;
      gcc-4.6 )
         wget https://release.bambuhls.eu:8080/compiler/gcc-4.6-bambu-Ubuntu_16.04.tar.xz --no-check-certificate -nv -O - | tar -C $2 -xJf - 
         ;;
      gcc-4.7 )
         wget https://release.bambuhls.eu:8080/compiler/gcc-4.7-bambu-Ubuntu_16.04.tar.xz --no-check-certificate -nv -O - | tar -C $2 -xJf - 
         ;;
      gcc-4.8 )
         wget https://release.bambuhls.eu:8080/compiler/gcc-4.8-bambu-Ubuntu_16.04.tar.xz --no-check-certificate -nv -O - | tar -C $2 -xJf - 
         ;;
      gcc-4.9 )
         wget https://release.bambuhls.eu:8080/compiler/gcc-4.9-bambu-Ubuntu_16.04.tar.xz --no-check-certificate -nv -O - | tar -C $2 -xJf - 
         ;;
      gcc-5 )
         wget https://release.bambuhls.eu:8080/compiler/gcc-5-bambu-Ubuntu_16.04.tar.xz --no-check-certificate -nv -O - | tar -C $2 -xJf - 
         ;;
      gcc-6 )
         wget https://release.bambuhls.eu:8080/compiler/gcc-6-bambu-Ubuntu_16.04.tar.xz --no-check-certificate -nv -O - | tar -C $2 -xJf - 
         ;;
      gcc-7 )
         wget https://release.bambuhls.eu:8080/compiler/gcc-7-bambu-Ubuntu_16.04.tar.xz --no-check-certificate -nv -O - | tar -C $2 -xJf - 
         ;;
      gcc-8 )
         wget https://release.bambuhls.eu:8080/compiler/gcc-8-bambu-Ubuntu_16.04.tar.xz --no-check-certificate -nv -O - | tar -C $2 -xJf - 
         ;;
      * )
         ;;
   esac
} 
IFS=',' read -r -a compilers <<< "${install}"
compilers=( $(IFS=$'\n'; echo "${compilers[*]}" | sort -V) )
for compiler in "${compilers[@]}"
do
inflate $compiler /
done
wait

if [[ "$PATH" != */usr/lib/ccache* ]]; then
   echo "export PATH=/usr/lib/ccache:$PATH" >> /etc/profile
fi
if [[ "$LIBRARY_PATH" != /usr/lib/x86_64-linux-gnu* ]]; then
   echo "export LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:$LIBRARY_PATH" >> /etc/profile
fi
ln -sf /usr/include/x86_64-linux-gnu/gmp.h /usr/include/gmp.h

GCC_BINS=("`find /usr/bin -type f -regextype posix-extended -regex '.*g(cc|\+\+)-[0-9]+\.?[0-9]?'`")
CLANG_BINS=("`find /clang+llvm-*/bin -type f -regextype posix-extended -regex '.*clang-[0-9]+\.?[0-9]?'`")
CLANG_EXES=("clang" "clang++" "clang-cl" "clang-cpp" "ld.lld" "lld" "lld-link" "llvm-ar" "llvm-config" "llvm-dis" "llvm-link" "llvm-lto" "llvm-lto2" "llvm-ranlib" "mlir-opt" "mlir-translate" "opt")

mkdir -p "$workspace_dir/dist/usr/bin"
for clang_exe in $CLANG_BINS
do
   CLANG_VER=$(sed 's/clang-//g' <<< "$(basename $clang_exe)")
   CLANG_DIR=$(dirname $clang_exe)
   echo "Generating system links for clang/llvm $CLANG_VER"
   for app in "${CLANG_EXES[@]}"
   do
      if [[ -f "$CLANG_DIR/$app" ]]; then
         ln -sf "$CLANG_DIR/$app" "/usr/bin/$app-$CLANG_VER"
      fi
   done
   echo "Generating ccache alias for clang-$CLANG_VER"
   ln -sf ../../bin/ccache "/usr/lib/ccache/clang-$CLANG_VER"
   echo "Generating ccache alias for clang++-$CLANG_VER"
   ln -sf ../../bin/ccache "/usr/lib/ccache/clang++-$CLANG_VER"
done

for compiler in $GCC_BINS
do
   echo "Generating ccache alias for $(basename $compiler)"
   ln -sf ../../bin/ccache "/usr/lib/ccache/$(basename $compiler)"
done