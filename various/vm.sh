
RAMSIZE_PER_SOCKET_HUMAN_READABLE=2G
PMEMSIZE_PER_SOCKET_HUMAN_READABLE=4G

# Convert everything to MB for ease
RAMSIZE_PER_SOCKET=$(numfmt --from=iec $RAMSIZE_PER_SOCKET_HUMAN_READABLE)
RAMSIZE_PER_SOCKET=$((RAMSIZE_PER_SOCKET/2**20))
PMEMSIZE_PER_SOCKET=$(numfmt --from=iec $PMEMSIZE_PER_SOCKET_HUMAN_READABLE)
PMEMSIZE_PER_SOCKET=$((PMEMSIZE_PER_SOCKET/2**20))

NUM_SOCKETS=2
TOTAL_RAMSIZE=$((NUM_SOCKETS*RAMSIZE_PER_SOCKET))
TOTAL_PMEMSIZE=$((NUM_SOCKETS*PMEMSIZE_PER_SOCKET))

args=(	# We have 4 slots since we have 2 RAM devices and 2 PMEM devices
	-m ${TOTAL_RAMSIZE}M,slots=4,maxmem=$((TOTAL_RAMSIZE + TOTAL_PMEMSIZE))M
	-machine pc,nvdimm=on

	# Boot drive and KVM support
	-hda debian.img
	-enable-kvm
	-nographic

	###########################
	# USELESS EXPERIMENTATION #
	###########################

	#--bios OVMF.fd
	#-drive if=pflash,format=raw,unit=0,file=OVMF_CODE.fd,readonly=on
	#-drive if=pflash,format=raw,unit=1,file=OVMF_VARS.fd

	#################
	# CUSTOM KERNEL #
	#################

	# Upon compilation on a different machine,
	# don't forget to tranfer the necessary modules
	# (to /lib/modules/<new_kernel>)

	#-append "root=/dev/sda1 console=ttyS0"
	#-kernel bzImage
	#== Enable debugging with GDB ==#
	# Execute "make vmlinux" in the VM
	# and transfer it over to the host.
	# Then do "gdb vmlinux" on the host
	# followed by "(gdb) target remote
	# :1234"
	# -s -S

	########
	# DRAM #
	########

	# one RAM module for each socket
	-object memory-backend-ram,size=${RAMSIZE_PER_SOCKET}M,id=m0
	-object memory-backend-ram,size=${RAMSIZE_PER_SOCKET}M,id=m1

	#####################
	# PERSISTENT MEMORY #
	#####################

	# Non volatile memory assigned to socket 0
	-object memory-backend-file,id=nvm0,share=on,mem-path=nvdimm/nvdimm0,size=${PMEMSIZE_PER_SOCKET}M
	-device nvdimm,id=nvdimm0,memdev=nvm0,node=0
	# Non volatile memory assigned to socket 1
	-object memory-backend-file,id=nvm1,share=on,mem-path=nvdimm/nvdimm1,size=${PMEMSIZE_PER_SOCKET}M
	-device nvdimm,id=nvdimm1,memdev=nvm1,node=1

	#== Change mode from 'raw' to 'fsdax' to enable DAX support ==#
	# ndctl create-namespace -f -e namespace0.0 --mode=fsdax
	#== Create a filesystem ==#
	# mkfs.ext4 /dev/pmem0
	#== Mount the filesystem ==#
	# mkdir /mnt/pmem0
	# mount -o dax /dev/pmem0 /mnt/pmem0

	##############
	# NUMA NODES #
	##############

	# We create 2 NUMA nodes with 1 core each
	# smp -> permitted number of cores to use
	-smp 4,sockets=2,maxcpus=4
	-numa node,cpus=0-1,memdev=m0,nodeid=0
	-numa node,cpus=2-3,memdev=m1,nodeid=1
	#-numa hmat-lb,initiator=1,target=0,hierarchy=memory,data-type=access-latency,latency=70
	#-numa hmat-lb,initiator=0,target=1,hierarchy=memory,data-type=access-latency,latency=70

	################
	# INSTALLATION #
	################

	#-cdrom debian-11.8.0-amd64-DVD-1.iso
	#-boot d

	# To install from serial console: Enable the 
	# "-nographic" option, and upon boot press Esc and
	# provide the command "install console=ttyS0,9600,n8"
	# to the boot prompt
	# Source: https://wiki.debian.org/DebianInstaller/Qemu
	
	##############
	# NETWORKING #
	##############
	
	# Network interafe for SSH
	# (use 'dhclient -d <interface>' with a custom kernel)
	# Connect using the following command:
	# TERM=xterm ssh -p 2222 <username>@127.0.0.1
	# (TERM helps fixing the 'backspace is space' issue)
 	-net user,hostfwd=tcp::2222-:22
	-net nic
	# Alternative: Preferred since the virtio driver should 
	# be faster. Enable it during the kernel configuration
	#-netdev type=user,id=net0,hostfwd=tcp::2222-:22
 	#-device virtio-net-pci,netdev=net0
)

# Create the persistent memory files if they don't exist
mkdir -p nvdimm
touch nvdimm/nvdimm0
touch nvdimm/nvdimm1

qemu-system-x86_64 "${args[@]}"
