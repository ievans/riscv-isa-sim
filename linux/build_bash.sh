curl https://ftp.gnu.org/gnu/bash/bash-4.3.30.tar.gz | tar -zx
cd bash-4.3.30
CC=riscv64-unknown-linux-gnu-gcc ./configure --host=riscv64-unknown-linux-gnu
make -j
