#!/bin/bash

# stop if any command returns nonzero
if [ $# -ne 3 ] ; then
  echo "usage: run_testsuite.sh <path to test folder sources> <path to linux folder holding testrunner_setup_disk.sh and vmlinux> <spike variant name>"
  exit 0
fi

TEST_SOURCES=$1
LINUX_ROOT=$2
SETUP_DISK=$LINUX_ROOT/testrunner_setup_disk.sh
VMLINUX=$LINUX_ROOT/vmlinux
SPIKE=$3

echo "Starting compilation..."
python compile_tests.py --path $TEST_SOURCES --runner run_tests.c
echo "Compilation complete."
echo "Starting spike(s)..."
python run_tests.py --sd $SETUP_DISK --vml $VMLINUX --spike $3
echo "Test complete."
