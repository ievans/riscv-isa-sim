echo "Creating disk image"
rm -f root.bin
dd if=/dev/zero of=root.bin bs=1M count=64
sudo mkfs.ext2 -F root.bin
echo "Mounting disk image"
rm -rf mnt/
mkdir mnt
sudo mount -o loop root.bin mnt
sudo chown $USER mnt/
cd mnt/
echo "Initializing disk image"
mkdir -p bin etc dev lib proc sbin tmp usr usr/bin usr/lib usr/sbin
cp ../busybox-1.21.1/busybox bin
cp ../inittab etc/
ln -s ../bin/busybox sbin/init
cp $RISCV/sysroot64/lib/libc.so.6 lib/
cp $RISCV/sysroot64/lib/ld.so.1 lib/
cp $RISCV/sysroot64/lib/libm.so.6 lib/
cd ..
if [ "$1" != "" ]; then
    echo "Copying provided files to directory"
    cp -r $1 mnt/
fi
echo "Unmounting directory"
sudo umount mnt
rm -rf mnt/
echo "Configuration complete"
echo "Run spike +disk=root.bin vmlinux"
