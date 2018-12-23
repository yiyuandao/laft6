#!/bin/bash

dir=`pwd`

if [ $1 == "start" ]
then
	insmod /lib/modules/`uname -r`/kernel/net/ipv4/netfilter/nf_defrag_ipv4.ko
	insmod $dir/kernel_module/test_entry.ko
	cd $dir/user_module/
	./tc &
elif [ $1 == "restart" ]
then
	killall tc
	rmmod test_entry
	insmod $dir/kernel_module/test_entry.ko
	cd $dir/user_module/
	./tc &
elif [ $1 == "stop" ]
then
	killall tc
	sleep 2
	rmmod test_entry
elif [ $1 == "compile" ]
then
	cd $dir/kernel_module
	./compile.sh
	cd $dir/user_module
	make
elif [ $1 == "clean" ]
then
	cd $dir/kernel_module
	make clean
	cd $dir/user_module
	make clean
elif [ $1 == "test" ]
then
	test -e /lib/modules/`uname -r`/kernel/net/ipv4/netfilter/nf_defrag_ipv4.ko && echo "nf_defrag_ipv4 exist" || echo "nf_defrag_ipv4.ko not exist"
fi

