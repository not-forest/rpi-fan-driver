/*
 *  Header file for rpifan driver.
 * */

#ifndef __RPI4F__
#define __RPI4F__

#include<linux/kernel.h>
#include<linux/fs.h>
#include<linux/pwm.h>

#define OUT 0
#define LOW 0
#define HIGH 1

// All raspberry pi GPIO pins, that can be used as a PWM.
#define PWM_GPIOS 12:case 13:case 18:case 19
// Physical pins (27, 28) are reserved for advanced use.
#define RESERVED 0 ... 1
#define OOR GPIO_AMOUNT ... 255

// First GPIO pin
#define GPIO_MIN 2
// Amount of available GPIO pins.
#define GPIO_AMOUNT 28 

// Default PWM option
#define PWM_OFF 0b111
// Adaptive PWM. The value is defined based on the CPU load. 
#define PWM_ADP 0b000

#define DEVICE_NAME "rpifan"
#define CLASS_NAME "fan"
#define GPIO_NAME "FAN_GPIO"
#define KBUF_SIZE 4


// IOCTL command call for controlling the PWM state completely from user space.
#define WR_PWM_VALUE _IOW('r', 'a', u64*)
// IOCTL command call for reading the current state of the PWM.
#define R_PWM_VALUE _IOR('r', 'b', u64*)
// Starting period value.
#define PWM_PERIOD 50000000

/**********************************/

/* 
 * Fan configuration structure
 *
 * Lower 5 bits for 26 available GPIO pins (the rest are reserved). Three bits for
 * PWM mode configuration.
 * */
union fan_config {
    uint8_t bytes;

    struct {
        uint8_t gpio_num: 5;
        uint8_t pwm_mode: 3;
    };
};

/**************** Driver data fields ****************/
// Static PWM state function with default parameters.
static struct pwm_state pwm_state = {
    .period = PWM_PERIOD,
    .duty_cycle = PWM_PERIOD,
    .polarity = PWM_POLARITY_NORMAL,
    .enabled = true,
};

// Pointer to prevent bugs in IOCTL call.
static struct pwm_state *pwm_s = &pwm_state;

/***************** Driver functions *****************/
static int __init rpfan_driver_init(void);
static void __exit rpfan_driver_exit(void);

static int rpfan_open(struct inode *inode, struct file *file);
static int rpfan_release(struct inode *inode, struct file *file);
static ssize_t rpfan_read(struct file *file, char __user *buf, size_t len, loff_t *off);
static ssize_t rpfan_write(struct file *file, const char *buf, size_t len, loff_t *off);
static long rpfan_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

/* Sets the new GPIO while parsing values and returning obtained errors */
int set_gpio(union fan_config *config, uint8_t old_gpio);
/* Initializes the first available GPIO in the system. */
int init_gpio(union fan_config *config);

/* Sets a new PWM to a certain GPIO based on the fan configuration */
int set_fan_pwm(union fan_config *config, u64 extra);
/* Initializes the PWM lookup table */
int init_fan_pwm(void);
/* Frees the PWM device. */
void free_fan_pwm(void);
/****************************************************/

#endif
