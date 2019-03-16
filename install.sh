#! /bin/bash
OBSI_DIR=$(pwd)
OPENBOX_CLICK_PACKAGE=$OBSI_DIR/openbox-click-package
BUILD_DIR="/tmp/build"
RE2_URL="https://github.com/google/re2.git"
RE2_TAG="2015-11-01"
CLICK_URL="https://github.com/tbarbette/fastclick.git"
NETMAP_URL="https://github.com/luigirizzo/netmap.git"
#CLICK_INSTALL_DIR=$HOME/click

function install_build_utils {
	apt-get update
	apt-get install build-essential python-dev g++ python-pip
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

function install_click {
	echo "[+] Clonning Click"
	cd $BUILD_DIR
	git clone $CLICK_URL
	cd fastclick
	echo "[+] Patching Click"
	git apply $OBSI_DIR/click_controlsocket.patch $OBSI_DIR/fromhost_reconfigure_tun_fix.patch
	echo "[+] Configuring Click"
	./configure --with-netmap --enable-netmap-pool --enable-multithread --disable-linuxmodule --enable-intel-cpu --enable-user-multithread --verbose --enable-select=poll --enable-poll --enable-bound-port-transfer --enable-local --enable-zerocopy --enable-batch --enable-json --disable-test --enable-stats
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
install_netmap
install_click
install_re2
install_openbox_click_package
install_python_dependency

# this command is needed to disable NIC offloading
# sudo ethtool -K eth1 tx off rx off gso off tso off gro off lro off
