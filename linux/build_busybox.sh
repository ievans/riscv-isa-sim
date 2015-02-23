curl -L http://busybox.net/downloads/busybox-1.21.1.tar.bz2 | tar -xj
cd busybox-1.21.1
make allnoconfig
cp ../busybox.config .config
make -j 24

