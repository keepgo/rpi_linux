#include <linux/init.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/fs.h>

#define DEVICE_NAME "virtualsim"
#define CLASS_NAME  "virtualsim"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Boris Frenkel");
MODULE_DESCRIPTION("SPI slave software driver for virtual SIM technology");
MODULE_VERSION("0.1");

static int     majorNumber;
static int     numberOpens = 0;

static struct  class*      spiSlaveSimClass = NULL;
static struct  device*     spiSlaveSimDevice = NULL;

static int     dev_open    (struct inode *, struct file *);
static int     dev_release (struct inode *, struct file *);
static ssize_t dev_read    (struct file *, char *, size_t, loff_t *);
static ssize_t dev_write   (struct file *, const char *, size_t, loff_t *);

static struct file_operations fops =
{
    .open    = dev_open,
    .read    = dev_read,
    .write   = dev_write,
    .release = dev_release,
};


static int __init spi_slave_sim_init(void)
{
    printk(KERN_INFO "Initialising of the Virtual SIM module\n");
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if ( majorNumber < 0 ) {
        printk(KERN_ALERT "SPI-Slave-SIM failed to register a major number %d\n", majorNumber);
        return majorNumber;
    }
    printk(KERN_INFO "SPI-Slave-SIM registered correctly witn major number %d\n", majorNumber);

    // Register the device class
    spiSlaveSimClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(spiSlaveSimClass)){                // Check for error and clean up if there is
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(spiSlaveSimClass);          // Correct way to return an error on a pointer
    }
    printk(KERN_INFO "SPI-Slave-SIM: device class registered correctly\n");

    // Register the device driver
    spiSlaveSimDevice = device_create(spiSlaveSimClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(spiSlaveSimDevice)){               // Clean up if there is an error
        class_destroy(spiSlaveSimClass);           // Repeated code but the alternative is goto statements
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(spiSlaveSimDevice);
    }
    printk(KERN_INFO "EBBChar: device class created correctly\n"); // Made it! device was initialized

    return 0;
}

static void __exit spi_slave_sim_exit(void)
{
    device_destroy(spiSlaveSimClass, MKDEV(majorNumber, 0));     // remove the device
    class_unregister(spiSlaveSimClass);                          // unregister the device class
    class_destroy(spiSlaveSimClass);                             // remove the device class
    unregister_chrdev(majorNumber, DEVICE_NAME);                 // unregister the major number

    printk(KERN_INFO "Virtual SIM module is removed\n");
}

static int dev_open(struct inode *inodep, struct file *filep){
   numberOpens++;
   printk(KERN_INFO "SPI-Slave-SIM: Device has been opened %d time(s)\n", numberOpens);
   return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
}

module_init(spi_slave_sim_init);
module_exit(spi_slave_sim_exit);

