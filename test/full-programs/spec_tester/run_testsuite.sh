#!/bin/bash

# stop if any command returns nonzero
set -e

if [ $# -ne 3 ] ; then
  echo "usage: run_testsuite.sh <path to test folder sources> <path to linux folder holding testrunner_setup_disk.sh and vmlinux> <spike variant name>"
  exit 0
fi

RUN_DIRS=$1
LINUX_ROOT=$2
SETUP_DISK=$LINUX_ROOT/testrunner_setup_disk.sh
VMLINUX=$LINUX_ROOT/vmlinux
SPIKE=$3

echo "Starting compilation..."
if [ ! -x runner ] ; then
  rm --preserve-root runner 2> /dev/null
  riscv64-unknown-linux-gnu-gcc run_spec_tests.c -o runner
fi
echo "Compilation complete."

echo "Starting spike(s)..."
python run_spec_tests.py --path $RUN_DIRS --sd $SETUP_DISK --vml $VMLINUX --spike $SPIKE
echo "Test complete."