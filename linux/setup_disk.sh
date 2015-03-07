# stop if any command returns nonzero
set -e

LINUX_ROOT=`pwd`

tmpdir=`mktemp -d` && cd $tmpdir
echo "Creating disk image: root.bin in temporary dir: $tmpdir"
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
mkdir -p bin etc dev lib proc sbin tmp usr usr/bin usr/lib usr/sbin riscv_tests
cp -v $LINUX_ROOT/busybox-1.21.1/busybox bin
cp -v $LINUX_ROOT/inittab etc/
ln -s /bin/busybox sbin/init

RISCV_BIN="$(which riscv64-unknown-linux-gnu-gcc)"
RISCV_BIN="$(dirname $RISCV_BIN)"
cp -a $RISCV_BIN/../sysroot64/lib/. lib/
if [ "$2" != "" ]; then
    echo "Copying provided files in $2 to this directory"
    cp -rv $LINUX_ROOT/$2 ../mnt/riscv_tests/
fi

if [ "$3" != "" ]; then
    # add to inittab
    echo "::sysinit:/riscv_tests/$(basename $3)" | cat - etc/inittab > /tmp/out && mv /tmp/out etc/inittab
    echo "changed inittab, now:"
    cat etc/inittab
fi

cd ..
echo "Unmounting directory"
sudo umount mnt
rm -rf mnt/
echo "Configuration complete"
echo "Copying "$tmpdir"/root.bin to "$1""
cp -v $tmpdir/root.bin $1
echo "Run spike +disk="$tmpdir"/root.bin vmlinux"
