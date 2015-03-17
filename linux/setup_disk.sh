# stop if any command returns nonzero
set -e

if [[ $# -eq 0 ]] ; then
    echo "setup_disk.sh: missing output disk image name"
    echo "usage: setup_disk.sh <output disk image name> [directory to copy into root of image] [path to executable to run on init] "
fi

# if this script is called in parallel, it will need a lot of loop devices
#for i in {8..100};
#do
#    /bin/mknod -m640 /dev/loop$i b 7 $i
#    /bin/chown root:disk /dev/loop$i
#done

SCRIPT=$(readlink -f "$0")
# Absolute path this script is in
LINUX_ROOT=$(dirname "$SCRIPT")

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

if [ -f $LINUX_ROOT/bash-4.3.30/bash ];
then
  cp -v $LINUX_ROOT/inittab_bash etc/inittab
  cp -v $LINUX_ROOT/bash-4.3.30/bash bin/
fi

ln -s /bin/busybox sbin/init

RISCV_BIN="$(which riscv64-unknown-linux-gnu-gcc)"
RISCV_BIN="$(dirname $RISCV_BIN)"
cp -a $RISCV_BIN/../sysroot64/lib/. lib/
if [ "$2" != "" ]; then
    echo "Copying provided files in $2 to this directory"
    if [[ "$2" = /* ]]; then
        cp -rv $2 ../mnt/riscv_tests/
    else
        cp -rv $LINUX_ROOT/$2 ../mnt/riscv_tests/        
    fi
fi

if [ "$3" != "" ]; then
    # write a custom inittab
    cp -v $LINUX_ROOT/inittab_autoshutdown etc/
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
cd $LINUX_ROOT
cp -v $tmpdir/root.bin $1
echo "Run spike +disk="$1"/root.bin vmlinux"
