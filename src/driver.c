/*
 *  file: driver.c
 *
 *  Main driver that handles fan's speed based on the user's configuration.
 *
 *  Tested with Linux raspberry pi 4, kernel built with buildroot: 6.1.61-v8
 * */

#include<linux/kernel.h>
#include<linux/kdev_t.h>
#include<linux/cdev.h>
#include<linux/module.h>
#include<linux/fs.h>
#include<linux/gpio.h>
#include<linux/kdev_t.h>
#include<linux/init.h>
#include<linux/pwm.h>

#include"rpi.h"

/**************** Driver data fields ****************/
static struct class *dev_class;
static struct cdev rpfan_gpio_cdev;
static union fan_config config;

dev_t dev = 0;
/****************************************************/

static struct file_operations fops = {
    .owner          = THIS_MODULE,
    .read           = rpfan_read,
    .write          = rpfan_write,
    .open           = rpfan_open,
    .release        = rpfan_release,
};

/* Character device open. */
static int rpfan_open(struct inode *inode, struct file *file) {
    pr_debug("%s: Configuration file opened.\n", THIS_MODULE->name);
    return 0;
}

/* Character device closed. */
static int rpfan_release(struct inode *inode, struct file *file) {
    pr_debug("%s: Configuration file closed.\n", THIS_MODULE->name);
    return 0;
}

/* Providing the current configuration. */
static ssize_t rpfan_read(struct file *file, char __user *buf, size_t len, loff_t *off) {
    char kbuf[KBUF_SIZE];
    len = snprintf(kbuf, KBUF_SIZE, "%u", config.bytes);
    if(copy_to_user(buf, &kbuf, len)) { 
        pr_err("%s: Failed to provide config data to the user.\n", THIS_MODULE->name);
        return -EIO;
    }
    pr_info("%s: Data read succesfully: %s\n", THIS_MODULE->name, kbuf);

    return 0;
}

/* Writing new configuration to the character device. */
static ssize_t rpfan_write(struct file *file, const char *buf, size_t len, loff_t *off) {
    uint8_t gpio = config.gpio_num;

    if(len > KBUF_SIZE) {
        pr_err("Invalid data size\n");
        return -EINVAL;
    }
   
    // Converting to u8.
    if(kstrtou8_from_user(buf, len, 10, &config.bytes)) {
        pr_err("%s: ERROR: Unconvertable u8 value: %s", THIS_MODULE->name, buf);
        return -EINVAL;
    }

    uint8_t ret;
    if((ret = set_gpio(&config, gpio)) < 0) {
        return ret;
    }

    return len;
}

/* Environment initialization. */
static int __init rpfan_driver_init(void) {
    // RPi fan character device region.
    if(alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME) < 0) {
        pr_err("%s: ERROR: Unable to allocate major number, aborting...\n", THIS_MODULE->name);
        goto _unreg;
    }

    pr_debug("%s: DEBUG: Major = %d, Minor = %d\n", THIS_MODULE->name, MAJOR(dev), MINOR(dev));
    cdev_init(&rpfan_gpio_cdev, &fops);

    // RPi character device driver for choosing the GPIO pin.
    if(cdev_add(&rpfan_gpio_cdev, dev, 1) < 0) {
        pr_err("%s: ERROR: Unable to add the character device for raspberry pi fan.\n", THIS_MODULE->name);
        goto _cdev;
    }

    // Creating class
    if(IS_ERR(dev_class = class_create(THIS_MODULE, CLASS_NAME))) {
        pr_err("%s: ERROR: Unable to create the structure class.\n", THIS_MODULE->name);
        goto _class;
    }

    // Spawning the device
    if(IS_ERR(device_create(dev_class, NULL, dev, NULL, DEVICE_NAME))) {
        pr_err("%s: ERROR: Unable to create the device.\n", THIS_MODULE->name);
        goto _dev;
    }

    // If everything is ok, trying to obtain one of the first available pins.
    if(init_gpio(&config) < 0) goto _dev; 

    pr_info("%s: Fan driver properly initialized for pin: GPIO_%d.\n", THIS_MODULE->name, config.gpio_num);
    return 0;

_dev:
    device_destroy(dev_class, dev);
_class:
    class_destroy(dev_class);
_cdev:
    cdev_del(&rpfan_gpio_cdev);
_unreg:
    unregister_chrdev_region(dev, 1);

    return -1;
}

/* Freeing the last used pin */
static void __exit rpfan_driver_exit(void) {
    gpio_set_value_cansleep(config.gpio_num, LOW);
    gpio_free(config.gpio_num);
    device_destroy(dev_class, dev);
    class_destroy(dev_class);
    cdev_del(&rpfan_gpio_cdev);
    unregister_chrdev_region(dev, 1);
    pr_info("%s: Fan driver closed.\n", THIS_MODULE->name);
}

module_init(rpfan_driver_init);
module_exit(rpfan_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("notforest <sshkliaiev@gmail.com>");
MODULE_DESCRIPTION("Driver for optimizing raspberry pi's fan and configurating it from the user space.");
MODULE_VERSION("0.1.alpha");
