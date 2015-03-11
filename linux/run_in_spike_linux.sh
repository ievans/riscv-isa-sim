set -e

echo "usage: run_in_spike_linux.sh [path to a riscv executable]"
echo "[note: if this script is run in parallel, make sure you have enough loopback devices]"
echo ""

echo "running $1 in spike linux"

if [ "$SPIKE" == "" ]; then
    SPIKE=spike
fi

# build a new disk
tmpdir=`mktemp -d`

SCRIPT=$(readlink -f "$0")
# Absolute path this script is in
SCRIPTDIR=$(dirname "$SCRIPT")
$SCRIPTDIR/setup_disk.sh $tmpdir/root.bin $1 $1

echo "disk setup OK, starting $SPIKE"
timeout --preserve-status --foreground 130 $SPIKE +disk=$tmpdir/root.bin $SCRIPTDIR/vmlinux
