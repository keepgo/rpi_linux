#include <linux/init.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/fs.h>

#define DEVICE_NAME "virtualsim"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Boris Frenkel");
MODULE_DESCRIPTION("SPI slave software driver for virtual SIM technology");
MODULE_VERSION("0.1");

static int dev_open(struct inode *, struct file *);


static int spi_slave_sim_init(void)
{
    printk("Virtual SIM module is installed\n");
    return 0;
}

static void spi_slave_sim_exit(void)
{
    printk("Virtual SIM module is removed\n");
}

module_init(spi_slave_sim_init);
module_exit(spi_slave_sim_exit);

