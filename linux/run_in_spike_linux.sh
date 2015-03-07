set -e

echo "running $1 in spike linux"

if [ "$SPIKE" == "" ]; then
    SPIKE=spike
fi

# build a new disk
tmpdir=`mktemp -d`
./setup_disk.sh $tmpdir/root.bin $1 $1

$SPIKE +disk=$tmpdir/root.bin vmlinux
