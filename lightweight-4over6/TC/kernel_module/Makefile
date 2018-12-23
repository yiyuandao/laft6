obj-m :=test_entry.o
test_entry-objs :=  src/bplus.o src/entry.o src/global.o src/memdebug.o src/modules.o src/netlink.o src/packet.o src/storage.o src/help.o src/nlmessage.o src/hash.o src/defrag6.o src/mac.o src/tccorestat.o src/tcuserinfo.o

EXTRA_CFLAGS := -I$(src)/include

all:
	make -C /lib/modules/`uname -r`/build M=`pwd`/
clean:
	make -C /lib/modules/`uname -r`/build M=`pwd` clean
	rm tmp/modules.order  tmp/Module.symvers

run:
	insmod /lib/modules/`uname -r`/kernel/net/ipv4/netfilter/nf_defrag_ipv4.ko
	@lsmod | grep -q test_entry  && /sbin/rmmod test_entry
	/sbin/insmod test_entry.ko
rem:
	/sbin/rmmod test_entry

