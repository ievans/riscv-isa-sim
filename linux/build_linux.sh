rm -rf --preserve-root linux-3.14.29
curl https://www.kernel.org/pub/linux/kernel/v3.x/linux-3.14.29.tar.xz | tar -xJ
cd linux-3.14.29
git init
git remote add origin https://github.com/riscv-mit/riscv-linux.git
git fetch
git checkout -f -t origin/tags
make ARCH=riscv defconfig
make ARCH=riscv -j 24 vmlinux
cp vmlinux ../
