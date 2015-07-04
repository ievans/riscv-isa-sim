# stop if any command returns nonzero
set -e

echo "Creating disk image: blankdisk.bin"
rm -f --preserve-root blankdisk.bin
dd if=/dev/zero of=blankdisk.bin bs=3M count=64
sudo mkfs.ext2 -F blankdisk.bin
