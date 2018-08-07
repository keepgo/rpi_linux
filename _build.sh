KERNEL=kernel7
DEFCONFIG=keepgopi_defconfig

MAKE_PARAMS="ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-"

# Directories
MNT_DIR=../_mnt
BOOT_DIR=${MNT_DIR}/boot
SYSTEM_DIR=${MNT_DIR}/system
BOOT_BUILDED=arch/arm/boot

make_config()
{
    make ${MAKE_PARAMS} ${DEFCONFIG}
}

make_kernel()
{
    make ${MAKE_PARAMS} zImage modules dtbs
}

mount_sdcard()
{
    echo "Mount SDCard..."
    mkdir -p ${MNT_DIR} ${BOOT_DIR} ${SYSTEM_DIR}
    sudo mount /dev/mmcblk0p1 ${BOOT_DIR}
    sudo mount /dev/mmcblk0p2 ${SYSTEM_DIR}
}

umount_sdcard()
{
    sudo umount ${BOOT_DIR}
    sudo umount ${SYSTEM_DIR}
}

check_sdcard()
{
    if [ -e /dev/mmcblk0p1 -a -e /dev/mmcblk0p2 ]; then
        return 1
    fi
    return 0
}

install_image()
{
    echo "Install image to the SDCard..."
    mount_sdcard
    echo "Install modules..."
    sudo make ${MAKE_PARAMS} INSTALL_MOD_PATH=${SYSTEM_DIR} modules_install

    echo "Make boot changes..."
    sudo cp -v ${BOOT_DIR}/$KERNEL.img ${BOOT_DIR}/$KERNEL-backup.img
    sudo cp -v ${BOOT_BUILDED}/zImage ${BOOT_DIR}/$KERNEL.img
    sudo cp -v ${BOOT_BUILDED}/dts/*.dtb ${BOOT_DIR}
    sudo cp -v ${BOOT_BUILDED}/dts/overlays/*.dtb* ${BOOT_DIR}/overlays/
    sudo cp -v ${BOOT_BUILDED}/dts/overlays/README ${BOOT_DIR}/overlays/
}

main()
{
    make_config
    make_kernel

    check_sdcard
    if [ $? -eq 1 ]; then
        install_image
        umount_sdcard
    else
        echo "SDCard not found! Please enter SDCard and repeat..."
    fi
}

main $@
