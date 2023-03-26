make modules_install
find /lib/modules/5.13.0Ext4MinKernel/ -iname "*.ko" -exec strip \
     --strip-debug {} \;
make install
# reboot
