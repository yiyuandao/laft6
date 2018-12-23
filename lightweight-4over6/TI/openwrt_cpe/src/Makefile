obj-m :=cpe_kernel.o
#cpe_kernel-objs := cpe_entry.o dbkeyhash.o fragment.o global.o helper.o log.o modules.o packet.o pktdown.o pktgen.o pktup.o shash.o timerproc.o tool.o mac.o  netlink.o dhcp.o
cpe_kernel-objs := cpe_entry.o pseudo.o dbkeyhash.o fragment.o global.o helper.o log.o modules.o packet.o pktdown.o pktgen.o pktup.o shash.o timerproc.o tool.o mac.o netlink.o dhcp.o
all:
	make -C /lib/modules/`uname -r`/build M=`pwd`
clean:
	make -C /lib/modules/`uname -r`/build M=`pwd` clean

install:
	/sbin/insmod cpe_kernel.ko
remove:
	/sbin/rmmod cpe_kernel

