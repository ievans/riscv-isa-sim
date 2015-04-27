set -e

echo "usage: run_trinity.sh [path to a riscv executable] args"
echo "[note: if this script is run in parallel, make sure you have enough loopback devices]"
echo ""

echo "running $1 in spike linux with args $2 $3 $4 $5 $6 $7 $8"

if [ "$SPIKE" == "" ]; then
    SPIKE=spike-no-return-copy
fi

# build a new disk
tmpdir=`mktemp -d`

SCRIPT=$(readlink -f "$0")
# Absolute path this script is in
SCRIPTDIR=$(dirname "$SCRIPT")
# up to 7 arguments passed to program
$SCRIPTDIR/setup_disk_trinity.sh $tmpdir/root.bin $1 $1 $2 $3 $4 $5 $6 $7 $8

echo "disk setup OK, starting $SPIKE"
timeout --preserve-status --foreground 130 $SPIKE +disk=$tmpdir/root.bin $SCRIPTDIR/vmlinux
