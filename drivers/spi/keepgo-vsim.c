#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/kobject.h>
#include <linux/time.h>

#include <linux/spi/keepgo_vsim.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Boris Frenkel");
MODULE_DESCRIPTION("SPI slave software driver for virtual SIM technology");
MODULE_VERSION("0.1");

static unsigned int gpioSimReset = 12;
module_param(gpioSimReset, uint, S_IRUGO);
MODULE_PARM_DESC(gpioSimReset, " GPIO SIM Reset number (default = 12)");

static unsigned int gpioLED = 19;
module_param(gpioLED, uint, S_IRUGO);
MODULE_PARM_DESC(gpioLED, " GPIO LED number (default = 19)");

static unsigned int gpioSimClock = 16;
module_param(gpioSimClock, uint, S_IRUGO);
MODULE_PARM_DESC(gpioSimClock, " GPIO SIM clock (default = 16)");

static unsigned int gpioSimData = 20;
module_param(gpioSimData, uint, S_IRUGO);
MODULE_PARM_DESC(gpioSimData, " GPIO SIM data (default = 20)");

static struct kobj_attribute debounce_attr = __ATTR(isDebounce, 0664, isDebounce_show, isDebounce_store);

static struct kobj_attribute ledon_attr    = __ATTR_RO(ledOn);     ///< the ledon kobject attr
static struct kobj_attribute time_attr     = __ATTR_RO(lastTime);  ///< the last time pressed kobject attr
static struct kobj_attribute diff_attr     = __ATTR_RO(diffTime);  ///< the difference in time attr

static struct attribute *vsim_attrs[] = {
    &ledon_attr.attr,                  ///< Is the LED on or off?
    &time_attr.attr,                   ///< Time of the last button press in HH:MM:SS:NNNNNNNNN
    &diff_attr.attr,                   ///< The difference in time between the last two presses
    &debounce_attr.attr,               ///< Is the debounce state true or false
    NULL,
};

static struct attribute_group attr_group = {
    .name  = gpioName,                 ///< The name is generated in vsim_init()
    .attrs = vsim_attrs,               ///< The attributes array defined just above
};

static ssize_t ledOn_show (struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", ledOn);
}

static ssize_t lastTime_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%.2lu:%.2lu:%.2lu:%.9lu \n", (ts_last.tv_sec/3600)%24,
                   (ts_last.tv_sec/60) % 60, ts_last.tv_sec % 60, ts_last.tv_nsec );
}

static ssize_t diffTime_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%lu.%.9lu\n", ts_diff.tv_sec, ts_diff.tv_nsec);
}

static ssize_t isDebounce_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", isDebounce);
}

static ssize_t isDebounce_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
    unsigned int temp;
    sscanf(buf, "%du", &temp);                // use a temp varable for correct int->bool
    gpio_set_debounce(gpioSimReset, 0);
    isDebounce = temp;
    if(isDebounce) { gpio_set_debounce(gpioSimReset, DEBOUNCE_TIME);
        printk(KERN_INFO "VSIM Reset: Debounce on\n");
    }
    else { gpio_set_debounce(gpioSimReset, 0);  // set the debounce time to 0
        printk(KERN_INFO "VSIM Reset: Debounce off\n");
    }
    return count;
}

static int __init vsim_init(void){
    int result = 0;

    printk(KERN_INFO "Virtual SIM: Initializing the virtual SIM module\n");
    sprintf(gpioName, "gpio%d", gpioSimReset);           // Create the gpio115 name for /sys/ebb/gpio115

    // create the kobject sysfs entry at /sys/ebb -- probably not an ideal location!
    vsim_kobj = kobject_create_and_add("vsim", kernel_kobj->parent); // kernel_kobj points to /sys/kernel
    if ( !vsim_kobj ) {
        printk(KERN_ALERT "Virtual SIM: failed to create kobject mapping\n");
        return -ENOMEM;
    }

    // add the attributes to /sys/ebb/ -- for example, /sys/ebb/gpio115/numberPresses
    result = sysfs_create_group(vsim_kobj, &attr_group);
    if ( result ) {
        printk(KERN_ALERT "Virtual SIM: failed to create sysfs group\n");
        kobject_put(vsim_kobj);                          // clean up -- remove the kobject sysfs entry
        return result;
    }
    getnstimeofday(&ts_last);                          // set the last time to be the current time
    ts_diff = timespec_sub(ts_last, ts_last);          // set the initial time difference to be 0

    // Going to set up the LED. It is a GPIO in output mode and will be on by default
    ledOn = false;
    gpio_request(gpioLED, "sysfs");          // gpioLED is hardcoded to 49, request it
    gpio_direction_output(gpioLED, ledOn);   // Set the gpio to be in output mode and on
    // gpio_set_value(gpioLED, ledOn);       // Not required as set by line above (here for reference)
    gpio_export(gpioLED, false);             // Causes gpio49 to appear in /sys/class/gpio
                                             // the bool argument prevents the direction from being changed
    gpio_request(gpioSimReset, "sysfs");        // Set up the gpioButton
    gpio_direction_input(gpioSimReset);         // Set the button GPIO to be an input
    gpio_set_debounce(gpioSimReset, DEBOUNCE_TIME); // Debounce the button with a delay of 200ms
    gpio_export(gpioSimReset, false);          // Causes gpio115 to appear in /sys/class/gpio
    // the bool argument prevents the direction from being changed

    // Perform a quick test to see that the button is working as expected on LKM load
    printk(KERN_INFO "Virtual SIM: The SIM reset state is currently: %d\n", gpio_get_value(gpioSimReset));

    /// GPIO numbers and IRQ numbers are not the same! This function performs the mapping for us
    irqNumberReset = gpio_to_irq(gpioSimReset);
    printk(KERN_INFO "Virtual SIM: The SIM reset is mapped to IRQ: %d\n", irqNumberReset);

    irqNumberClock = gpio_to_irq(gpioSimClock);
    printk(KERN_INFO "Virtual SIM: The SIM clock is mapped to IRQ: %d\n", irqNumberClock);

    // This next call requests an interrupt line
    result = request_irq(irqNumberReset,             // The interrupt number requested
                         (irq_handler_t) vsim_reset_irq_handler, // The pointer to the handler function below
                         IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,              // Use the custom kernel param to set interrupt type
                         "vsim_reset_handler",  // Used in /proc/interrupts to identify the owner
                         NULL);                 // The *dev_id for shared interrupt lines, NULL is okay
    printk(KERN_INFO "Virtual SIM: irqNumberReset(result) = %d", result);

    /*
    result = request_irq(irqNumberClock,
                         (irq_handler_t) vsim_clock_irq_handler,
                         IRQF_TRIGGER_RISING,
                         "vsim_clock_handler",
                         NULL);
    printk(KERN_INFO "Virtual SIM: irqNumberClock(result) = %d", result);
    */

    return result;
}

static void __exit vsim_exit(void){
    kobject_put(vsim_kobj);                     // clean up -- remove the kobject sysfs entry
    gpio_set_value(gpioLED, 0);                 // Turn the LED off, makes it clear the device was unloaded
    gpio_unexport(gpioLED);                     // Unexport the LED GPIO
    free_irq(irqNumberReset, NULL);                  // Free the IRQ number, no *dev_id required in this case
    gpio_unexport(gpioSimReset);                // Unexport the Button GPIO
    gpio_free(gpioLED);                         // Free the LED GPIO
    gpio_free(gpioSimReset);                    // Free the Button GPIO
    printk(KERN_INFO "Virtual SIM: Goodbye from the Virtual SIM driver!\n");
}

static irq_handler_t vsim_reset_irq_handler (unsigned int irq, void *dev_id, struct pt_regs *regs) {
    uint gpioSimResetState = gpio_get_value(gpioSimReset);

    // LED
    ledOn = (bool)gpioSimResetState;
    gpio_set_value(gpioLED, ledOn);

    getnstimeofday(&ts_current);                   // Get the current time as ts_current
    ts_diff = timespec_sub(ts_current, ts_last);   // Determine the time difference between last 2 presses
    ts_last = ts_current;                          // Store the current time as the last time ts_last

    printk(KERN_INFO "Virtual SIM: The virtual SIM reset state is currently: %d\n", gpioSimResetState);

    if ( gpioSimResetState == 1 )
        readSpiClock();

    return (irq_handler_t) IRQ_HANDLED;  // Announce that the IRQ has been handled correctly
}

static bool readSpiClock (void) {
    bool clock_state, reset_state;
    volatile unsigned int gpio_value;
    volatile unsigned int * gpio_address = (unsigned int *)0x3F200034;  // GPIO Pin Level 0 (BCM 0-31)

    reset_state = true;
    do {
        gpio_value = * (gpio_address);
        printk(KERN_INFO "Virtual SIM: GPIO value: %d", gpio_value);
        reset_state = (gpio_value & (1 << gpioSimReset));
        clock_state = (gpio_value & (1 << gpioSimClock));
        printk(KERN_INFO "Virtual SIM: Clock: %d", clock_state);
    } while ( reset_state );

    return true;
}

/*
static irq_handler_t vsim_clock_irq_handler (unsigned int irq, void *dev_id, struct pt_regs *regs) {
    // uint gpioSimClockState;

    printk(KERN_INFO "Virtual SIM: %s", __FUNCTION__);
    // gpioSimClockState = gpio_get_value(gpioSimClock);
    // printk(KERN_INFO "Virtual SIM: Clock: %d", gpioSimClockState);

    return (irq_handler_t) IRQ_HANDLED;
}
*/

module_init(vsim_init);
module_exit(vsim_exit);

