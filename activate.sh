#!/bin/bash

mode=$1

shift

if [[ $mode == "kernel" ]]; then
	echo "Kernel Configuration."
	for iface in $@; do
		echo "Preparing interface "$iface
		ethtool -K $2 tx off rx off gso off tso off gro off lro off
	done;
fi
# This is needed if using DPDK:
if [[ $mode == "dpdk" ]]; then
	echo "DPDK Configuration."
	echo 2048 > /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages
	mkdir /mnt/huge
	mount -t hugetlbfs nodev /mnt/huge
	modprobe uio
	modprobe vfio-pci
	insmod $RTE_SDK/$RTE_TARGET/kmod/igb_uio.ko
	for iface in $@; do
		echo "Preparing interface "$iface
		bus_id=$(ethtool -i $iface | grep bus | cut -d" " -f 2)
		$RTE_SDK/usertools/dpdk-devbind.py -u $iface
		$RTE_SDK/usertools/dpdk-devbind.py -b igb_uio $bus_id
	done;
fi
if [[ $mode == "netmap" ]]; then
	echo "Netmap Configuration."
	modprobe netmap
fi
