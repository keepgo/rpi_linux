#include <linux/types.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/fs.h>

MODULE_LICENSE("Dual BSD/GPL");

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

