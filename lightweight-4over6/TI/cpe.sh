#!/bin/bash

dir=`pwd`

if [ $1 == "start" ]
then
	lsmod | grep nf_defrag_ipv4 || insmod /lib/modules/`uname -r`/kernel/net/ipv4/netfilter/nf_defrag_ipv4.ko
	insmod $dir/openwrt_cpe/src/cpe_kernel.ko
	cd $dir/mess-handle/
	./cpe_start &
elif [ $1 == "restart" ]
then
	killall cpe_start
	rmmod cpe_kernel
	insmod $dir/openwrt_cpe/src/cpe_kernel.ko
	cd $dir/mess-handle/
	./cpe_start &
elif [ $1 == "pcp" ]
then
	killall cpe_start
	cd $dir/mess-handle/
	./cpe_start &
elif [ $1 == "stop" ]
then
	killall cpe_start
	rmmod cpe_kernel
elif [ $1 == "compile" ]
then
	cd $dir/openwrt_cpe/src/
	make
	cd $dir/mess-handle/
	make clean
	make
elif [ $1 == "clean" ]
then
	cd $dir/openwrt_cpe/src/
	make clean
	cd $dir/mess-handle/
	make clean
elif [ $1 == "test" ]
then
	test -e /lib/modules/`uname -r`/kernel/net/ipv4/netfilter/nf_defrag_ipv4.ko && echo "nf_defrag_ipv4 exist" || echo "nf_defrag_ipv4.ko not exist"
fi

