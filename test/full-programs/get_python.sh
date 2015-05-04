#!/bin/bash
set -e

echo "Obtaining python binary from web.mit.edu/jugonz97..."
wget http://web.mit.edu/jugonz97/www/riscv/python.tar
tar -xf python.tar
