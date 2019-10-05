#! /bin/bash
OBSI_DIR=$(pwd)
OPENBOX_CLICK_PACKAGE=$OBSI_DIR/openbox-click-package
BUILD_DIR="/tmp/build"
RE2_URL="https://github.com/google/re2.git"
RE2_TAG="2015-11-01"
CLICK_URL="https://github.com/tbarbette/fastclick.git"
NETMAP_URL="https://github.com/luigirizzo/netmap.git"
DPDK_URL="https://github.com/DPDK/dpdk.git"
NUM_THREAD=${NUM_THREAD:-4}
#CLICK_INSTALL_DIR=$HOME/click

function install_build_utils {
	apt-get update
	apt-get install build-essential python-dev g++ python-pip autoconf git libpcap-dev zlib1g-dev pkg-config ethtool libnuma-dev linux-headers-$(uname -r)
}

function install_re2 {
	echo "[+] Clonning RE2"
	cd $BUILD_DIR
	git clone $RE2_URL
	cd re2
	git checkout $RE2_TAG
	echo "[+] Compiling RE2"
	make
	echo "[+] Testing RE2"
	make test
	echo "[+] Installing RE2"
	make install
	make testinstall
	echo "[+] Fixind ld paths"
	ldconfig
}

function install_netmap {
	echo "[+] Clonning Netmap"
	cd $BUILD_DIR
	git clone $NETMAP_URL
	cd netmap
	echo "[+] Configuring Netmap"
	./configure
	echo "[+] Building Netmap"
	make
	echo "[+] Installing Netmap"
	make install
}

function install_dpdk {
	RTE_TARGET=x86_64-native-linux-gcc
	echo "[+] Cloning DPDK"
	cd $BUILD_DIR
	git clone $DPDK_URL
	cd dpdk
	git checkout v19.05
	echo "[+] Configuring DPDK"
	make config T=$RTE_TARGET O=$RTE_TARGET
	cd $BUILD_DIR/dpdk/$RTE_TARGET
	sed -ri 's,(CONFIG_RTE_LIBRTE_PMD_PCAP=).*,\1y,' .config
	echo "[+] Building DPDK"
	make -j $NUM_THREAD T=$RTE_TARGET EXTRA_FLAGS="-fPIC"
	echo "[+] Installing DPDK"
	make install
	RTE_SDK=$BUILD_DIR/dpdk
	echo "export RTE_TARGET="$RTE_TARGET >> ~/.bashrc
	echo "export RTE_SDK="$BUILD_DIR/dpdk >> ~/.bashrc
}

function install_click {
	echo "[+] Clonning Click"
	cd $BUILD_DIR
	git clone $CLICK_URL
	cd fastclick
	git checkout 3199a7f5c57ed7aab157766b97c3ee7ca9a57513
	echo "[+] Patching Click"
	git apply $OBSI_DIR/click_controlsocket.patch $OBSI_DIR/dpdk_reconfiguration.patch  $OBSI_DIR/fromhost_reconfigure_tun_fix.patch
	echo "[+] Configuring Click"
	# ./configure --disable-linuxmodule --disable-linux-symbols --disable-linuxmodule --disable-bsdmodule --enable-all-elements --enable-user-multithread --enable-stats=1 --enable-json --disable-test
	# Uncomment to compile with netmap
	#./configure --with-netmap --enable-netmap-pool --enable-multithread --disable-linuxmodule --enable-intel-cpu --enable-user-multithread --verbose --enable-select=poll --enable-poll --enable-bound-port-transfer --enable-local --enable-zerocopy --enable-batch --enable-json --disable-test --enable-stats
	# Uncomment to compile with dpdk
	./configure RTE_TARGET=$RTE_TARGET RTE_SDK=$RTE_SDK --enable-multithread --disable-linuxmodule --enable-intel-cpu --enable-user-multithread --verbose --enable-poll --enable-bound-port-transfer --enable-dpdk --enable-batch --with-netmap=no --enable-zerocopy --disable-dpdk-pool --disable-dpdk-packet --enable-json --enable-local --disable-test --enable-stats
	echo "[+] Compiling Click"
	make
	echo "[+] Installing Click"
	make install
}

function install_openbox_click_package {
	cd $OPENBOX_CLICK_PACKAGE
	echo "[+] Configuring OpenBox Click Package"
	autoconf
	./configure
	echo "[+] Compiling OpenBox Click Package"
	make
	echo "[+] Installing OpenBox Click Package"
	make install
}

function install_python_dependency {
	pip install tornado
	pip install psutil
}

install_build_utils
rm -rf $BUILD_DIR
mkdir $BUILD_DIR
#install_netmap
install_dpdk
install_re2
install_click
install_openbox_click_package
install_python_dependency

# this command is needed to disable NIC offloading
# sudo ethtool -K eth1 tx off rx off gso off tso off gro off lro off
# This is needed if using DPDK:
# echo 1024 > /sys/devices/system/node/node0/hugepages/hugepages-1024kB/nr_hugepages
# mkdir /mnt/huge
# mount -t hugetlbfs nodev /mnt/huge
# modprobe uio
# modprobe vfio-pci

