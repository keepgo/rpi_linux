/* SPDX-License-Identifier: GPL-2.0 */
/* keepgo_vsim.h */
#ifndef _LINUX_SPI_KEEPGO_VSIM_H
#define _LINUX_SPI_KEEPGO_VSIM_H

#define DEBOUNCE_TIME 200

#define DEVICE_NAME "vsim"
#define CLASS_NAME  "vsim"

static char   gpioName[8] = "gpioXXX";
static int    irqNumberReset, irqNumberClock;
static bool   ledOn = false;
static bool   isDebounce = true;
static struct timespec ts_last, ts_current, ts_diff;

static irq_handler_t vsim_reset_irq_handler (unsigned int irq, void *dev_id, struct pt_regs * regs);
static irq_handler_t vsim_clock_irq_handler (unsigned int irq, void *dev_id, struct pt_regs * regs);

static ssize_t       isDebounce_show        (struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t       isDebounce_store       (struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
static ssize_t       ledOn_show             (struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t       lastTime_show          (struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t       diffTime_show          (struct kobject *kobj, struct kobj_attribute *attr, char *buf);

static struct kobject *vsim_kobj;


#endif  // _LINUX_SPI_KEEPGO_VSIM_H

