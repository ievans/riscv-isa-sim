#!/bin/bash

# stop if any command returns nonzero
set -e

function usage {
    echo "usage: setup_disk.sh <output disk image name> [-nosudo] [-d directory to copy into root of image] [-x path to executable to run on init] [-xarg argument to executable as a relative path]"
    exit 0
}

DISK_IMAGE_NAME=""
DIRECTORY_PATH=""
EXE_PATH=""
EXE_ARG=""
USE_SUDO=1

# stackoverflow.com/questions/192249/how-do-i-parse-command-line-arguments-in-bash
while [[ $# > 0 ]]
do
    key="$1"

    case $key in
        -d)
            DIRECTORY_PATH="$2"
            shift # past argument
        ;;
        -x)
            EXE_PATH="$2"
            shift # past argument
        ;;
        -xarg)
            EXE_ARG="$2"
            shift # past argument
        ;;
        -nosudo)
            USE_SUDO=0
        ;;
        *)
            if [[ "$DISK_IMAGE_NAME" == "" ]] ; then
                DISK_IMAGE_NAME=$1
            else
                echo "setup_disk.sh: too many arguments"
                usage
            fi
        ;;
    esac
    shift # past argument or value
done

if [[ "$DISK_IMAGE_NAME" == "" ]] ; then
    echo "setup_disk.sh: missing output disk image name"
    usage
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
rm -f --preserve-root root.bin

if [[ "$USE_SUDO" -eq 1 ]] ; then
    dd if=/dev/zero of=root.bin bs=1M count=64
    sudo mkfs.ext2 -F root.bin

    echo "Mounting disk image"

    rm -rf --preserve-root mnt/
    mkdir mnt
    sudo mount -o loop root.bin mnt
    sudo chown $USER mnt/
else
    if [ ! -f $LINUX_ROOT/blankdisk.bin ]; then
        echo "blankdisk.bin not found. Run make_blank_disk.sh before using -nosudo."
        exit 0
    fi
    cp $LINUX_ROOT/blankdisk.bin root.bin

    echo "Mounting disk image"

    rm -rf --preserve-root mnt/
    mkdir mnt
    fuseext2 -o rw+ root.bin mnt
fi

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

if [[ "$EXE_PATH" != "" ]]; then
  # use inittab_nosh
  cp -v $LINUX_ROOT/inittab_nosh etc/inittab
fi

ln -s /bin/busybox sbin/init

RISCV_BIN="$(which riscv64-unknown-linux-gnu-gcc)"
RISCV_BIN="$(dirname $RISCV_BIN)"
cp -a $RISCV_BIN/../sysroot64/lib/. lib/
if [[ "$DIRECTORY_PATH" != "" ]]; then
    echo "Copying provided files in $DIRECTORY_PATH to this directory"
    if [[ "$DIRECTORY_PATH" = /* ]]; then
        cp -rv $DIRECTORY_PATH ../mnt/riscv_tests/
    else
        cp -rv $LINUX_ROOT/$DIRECTORY_PATH ../mnt/riscv_tests/
    fi
fi

if [[ "$EXE_PATH" != "" ]]; then
    # write a custom inittab
    if [[ "$DIRECTORY_PATH" == "" ]]; then
        echo "executable provided for inittab but directory is missing"
        usage
    fi
    # chop off the directory path as a prefix
    DIRECTORY_PATH=${DIRECTORY_PATH%/}
    EXE_PATH=$(basename $DIRECTORY_PATH)/${EXE_PATH#$DIRECTORY_PATH/}

    cp -v $LINUX_ROOT/inittab_autoshutdown etc/
    if [[ "$EXE_ARG" != "" ]]; then
        echo "::sysinit:/riscv_tests/$EXE_PATH /riscv_tests/$(dirname $EXE_PATH)/$EXE_ARG" | cat etc/inittab - > /tmp/out && mv /tmp/out etc/inittab
    else
        echo "::sysinit:/riscv_tests/$EXE_PATH" | cat etc/inittab - > /tmp/out && mv /tmp/out etc/inittab
    fi
    echo "changed inittab, now:"
    cat etc/inittab
fi

cd ..
echo "Unmounting directory"
if [[ "$USE_SUDO" -eq 1 ]] ; then
    sudo umount mnt
else
    fusermount -u mnt
fi

rm -rf --preserve-root mnt/
echo "Configuration complete"
echo "Copying "$tmpdir"/root.bin to "$DISK_IMAGE_NAME""
cd $LINUX_ROOT
cp -v $tmpdir/root.bin $DISK_IMAGE_NAME
rm -rf --preserve-root $tmpdir
echo "Run spike +disk="$DISK_IMAGE_NAME"/root.bin vmlinux"
