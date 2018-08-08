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
    EXIT_CODE=$?

    echo "------------------------------------------------------------------------------------------"
    echo -en "[...] Make config ... "
    [ ${EXIT_CODE} -eq 0 ] && echo "PASS" || echo "FAIL"
    echo "------------------------------------------------------------------------------------------"
    return ${EXIT_CODE}
}

make_kernel()
{
    make ${MAKE_PARAMS} zImage modules dtbs
    EXIT_CODE=$?

    echo "------------------------------------------------------------------------------------------"
    echo -en "[...] Building kernel sources ... "
    [ ${EXIT_CODE} -eq 0 ] && echo "PASS" || echo "FAIL"
    echo "------------------------------------------------------------------------------------------"
    return ${EXIT_CODE}
}

mount_sdcard()
{
    echo -e "------------------------------------------------------------------------------------------"
    echo -en "[...] Mount SDCard ... "
    mkdir -p ${MNT_DIR} ${BOOT_DIR} ${SYSTEM_DIR}
    [ $? -gt 0 ] && echo "FAIL" && return 1

    sudo mount /dev/mmcblk0p1 ${BOOT_DIR}
    [ $? -gt 0 ] && echo "FAIL" && return 1

    sudo mount /dev/mmcblk0p2 ${SYSTEM_DIR}
    [ $? -gt 0 ] && echo "FAIL" && return 1

    echo "PASS"
    echo -e "------------------------------------------------------------------------------------------"
    return 0
}

umount_sdcard()
{
    sudo umount ${BOOT_DIR}
    sudo umount ${SYSTEM_DIR}
}

check_sdcard()
{
    echo -e "------------------------------------------------------------------------------------------"
    echo -n "[...] Checking SDCard ... "
    [ -e /dev/mmcblk0p1 -a -e /dev/mmcblk0p2 ] && EXIT_CODE=1 || EXIT_CODE=0

    [ ${EXIT_CODE} -eq 1 ] && echo "PASS" || echo "FAIL"
    [ ${EXIT_CODE} -eq 0 ] && echo "[ERR] SDCard not found! Please enter SDCard and repeat..."
    echo -e "------------------------------------------------------------------------------------------"
    return ${EXIT_CODE}
}

install_modules()
{
    sudo make ${MAKE_PARAMS} INSTALL_MOD_PATH=${SYSTEM_DIR} modules_install
    EXIT_CODE=$?

    echo -e "------------------------------------------------------------------------------------------"
    echo -n "[...] Installing modules ... "
    [ ${EXIT_CODE} -eq 0 ] && echo "PASS" || echo "FAIL"
    echo -e "------------------------------------------------------------------------------------------"
    return ${EXIT_CODE}
}

install_image()
{
    sudo cp -v ${BOOT_DIR}/$KERNEL.img ${BOOT_DIR}/$KERNEL-backup.img
    [ $? -eq 0 ] && sudo cp -v ${BOOT_BUILDED}/zImage ${BOOT_DIR}/$KERNEL.img
    [ $? -eq 0 ] && sudo cp -v ${BOOT_BUILDED}/dts/*.dtb ${BOOT_DIR}
    [ $? -eq 0 ] && sudo cp -v ${BOOT_BUILDED}/dts/overlays/*.dtb* ${BOOT_DIR}/overlays/
    [ $? -eq 0 ] && sudo cp -v ${BOOT_BUILDED}/dts/overlays/README ${BOOT_DIR}/overlays/
    EXIT_CODE=$?

    echo -e "------------------------------------------------------------------------------------------"
    echo -n "[...] Install boot (kernel, overlays etc...) ... "
    [ ${EXIT_CODE} -eq 0 ] && echo "PASS" || echo "FAIL"
    echo -e "------------------------------------------------------------------------------------------"
}

print_help()
{
    echo -e "Building and install linux kernel for Raspberry Pi based devices"
    echo -e "------------------------------------------------------------------------------------------"
    echo -e "Usage:\t$0 [OPTIONS]\n"
    echo -e "\t\t-c, --clear\t\tclear builded sources"
    echo -e "\t\t-g, --config\t\tmake kernel config"
    echo -e "\t\t-nb, --no-build\t\tdon't build sources (usng prebuilded state)"
    echo -e "\t\t-ii, --install-image\tinstall builded kernel image and overlays to the prepared sdcard"
    echo -e "\t\t-im, --install-modules\tinstall builded kernel modules to the prepared sdcard"
    echo -e "\t\t-v, --verbose\t\texplain what is being done"
    echo -e "\t\t-h, --help\t\tdisplay this help and exit"
    echo -e "\n"
}

main()
{
    # Default parameters
    CONFIG=0; BUILD=1; INSTALL=0; INSTALL_IMAGE=0; INSTALL_MODULES=0; CLEAR=0; VERBOSE=0

    while [ "$1" != "" ]; do
        case $1 in
            -c | --clear )
                CLEAR=1
                ;;
            -v | --verbose )
                VERBOSE=1
                ;;
            -g | --config )
                CONFIG=1
                ;;
            -nb | --no-build )
                BUILD=0
                ;;
            -ii | --install-image )
                INSTALL=1;
                INSTALL_IMAGE=1;
                ;;
            -im | --install-modules )
                INSTALL=1;
                INSTALL_MODULES=1;
                ;;
            -h | --help )
                print_help
                exit
                ;;
        esac
        shift
    done

    # Config kernel
    [ ${CONFIG} -eq 1 ] && make_config
    [ $? -gt 0 ] && [ ${CONFIG} -eq 1 ] && exit 1

    # Build kernel
    [ ${BUILD} -eq 1 ] && make_kernel
    [ $? -gt 0 ]  && [ ${BUILD} -eq 1 ] && exit 1
    [ ${INSTALL} -eq 0 ] && echo -e "Builded kernel image and modules can be installed to the prepared SDCard with Raspbian.\nOS image can be downloaded from https://www.raspberrypi.org/downloads/raspbian/\n"

    # Check SDCard
    [ ${INSTALL} -eq 1 ] && check_sdcard
    [ $? -eq 0 ] && [ ${INSTALL} -eq 1 ] && exit 1

    # Mount SDCard
    [ ${INSTALL} -eq 1 ] && mount_sdcard || MOUNTED=0
    [ $? -gt 0 ] && [ ${INSTALL} -eq 1 ] && exit 1
    [ ${INSTALL} -eq 1 ] && MOUNTED=1

    # Install kernel and modules
    [ ${INSTALL_MODULES} -eq 1 ] && install_modules
    [ $? -gt 0 ] && [ ${INSTALL_MODULES} -eq 1 ] && exit 1
    [ ${INSTALL_IMAGE} -eq 1 ] && install_image
    [ $? -gt 0 ] && [ ${INSTALL_IMAGE} -eq 1 ] && exit 1

    # Umount SDCard
    [ ${MOUNTED} -eq 1 ] && umount_sdcard
}

main $@
