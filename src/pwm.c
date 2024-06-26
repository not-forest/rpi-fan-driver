/* 
 *  file: pwm.c
 *
 *  Module for handling PWM management. Based on the provided configuration it initializes the PWM
 *  on the embedded target device via pwm.h API. The PWM interfaces are parsed from the device tree file,
 *  which must be appended to the root folder of this driver.
 *
 *  Tested with Linux raspberry pi 4, kernel built with buildroot: 6.1.61-v8
 * */

#include<linux/version.h>
#include<linux/module.h>
#include<linux/kernel.h>

#include<linux/mod_devicetable.h>
#include<linux/property.h>
#include<linux/of_device.h>
#include<linux/pwm.h>
#include<linux/thermal.h>

#include "rpi.h"

// Legacy function for obtaining PWM via their index.
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 5, 0)
#define LEGACY  // Using legacy 'pwm_request' for older kernels.
#endif

// Must be provided from Makefile.
#ifndef PWM_PINS
#define PWM_PINS 12, 13
#endif

#define _COUNT_PWMS(...) COUNT_ARGS(__VA_ARGS__)
#define PWM_CHANNELS_AMOUNT _COUNT_PWMS(PWM_PINS)
#define __GET_PWM_VAL(i, ...) ((int[]) {__VA_ARGS__})[i]

#ifndef LEGACY
// Obtained pointer to the PWM device (One or two channels).
static struct pwm_device *pwms[PWM_CHANNELS_AMOUNT];
#else
// Obtained pointer to the PWM device (One or two channels).
static int pwms[PWM_PWM_CHANNELS_AMOUNT];
#endif
/****************************************************/

#ifndef LEGACY

/******** Must be configured per overlay ********/
#ifndef DEV_LABEL           // PWM label within the device-tree overlay.
#define DEV_LABEL "rpifan"
#endif
/************************************************/

#define PLATFORM_DRIVER_NAME "rpifan_pwm"
#define DT_DEV_COMP PLATFORM_DRIVER_NAME

#if PWM_CHANNELS_AMOUNT > 1
#define FOR_EACH_CHANNEL(i, code) \
    int i = 0; \
    code \
    i++; \
    code
#else
#define FOR_EACH_CHANNEL(i, code) \
    int i = 0; \
    code
#endif


/************** Driver probe functions **************/
static int rpi_fan_pwm_probe(struct platform_device *pdev);
static int rpi_fan_pwm_remove(struct platform_device *pdev);
/****************************************************/

/**************** Driver data fields ****************/
// Device tree match table.
static struct of_device_id rpifan_ids[] = {
    {
        .compatible = DT_DEV_COMP,
    }, 
    { /* Sentinel */ }
};

// Platform driver for requesting the PWM.
static struct platform_driver rpi_platform_driver = {
    .probe = rpi_fan_pwm_probe,
    .remove = rpi_fan_pwm_remove,
    .driver = {
        .name = PLATFORM_DRIVER_NAME,
        .owner = THIS_MODULE,
        .of_match_table = rpifan_ids,
    },
};

/****************************************************/

MODULE_DEVICE_TABLE(of, rpifan_ids);

/* Requests the PWM when probed. */
static int rpi_fan_pwm_probe(struct platform_device *pdev) {
    pr_info("%s: Platform driver probed, requesting PWM...\n", PLATFORM_DRIVER_NAME);
    struct device *dev = &pdev->dev;
    char *labels[PWM_CHANNELS_AMOUNT] = {"ch0", "ch1"};

    // This prevents segfault.
    if(!device_property_present(dev, "label")) {
        pr_err("%s: No device label present.", PLATFORM_DRIVER_NAME);
        return -ENODEV;
    }

    // Matching the label.
    if(device_property_match_string(dev, "label", DEV_LABEL) < 0) {
        pr_err("%s: Device label does not match.", PLATFORM_DRIVER_NAME);
        return -ENODEV;
    }
   
    // Obtaining the defined PWM channels.
    FOR_EACH_CHANNEL(i,
        pwms[i] = pwm_get(dev, labels[i]);

        if(IS_ERR(pwms)) {
            int err = ERR_PTR(pwms[i]);
            if(PTR_ERR(pwms[i]) != -EPROBE_DEFER) {
                pr_err("%s: ERROR: Requesting PWM channel: %d failed: %d", PLATFORM_DRIVER_NAME, i, err);
            }
            return err;
        }

        pwm_apply_state(pwms[i], pwm_s);
    );

    return 0;
}

/* Platform driver destructor. Only handles the freeing. */
static int rpi_fan_pwm_remove(struct platform_device *pdev) {
    pr_info("%s: Platform driver removed. PWM is freed.\n", PLATFORM_DRIVER_NAME); 
    FOR_EACH_CHANNEL(i, pwm_put(pwms[i]););       // Freeing the device.
    return 0;
}
#endif

/* Sets a new PWM to a certain GPIO based on the fan configuration */
int set_fan_pwm(union fan_config *config, u64 extra) {
    FOR_EACH_CHANNEL(i, 
        if(config->gpio_num == __GET_PWM_VAL(i, PWM_PINS)) {
            // The PWM was not initialized. Maybe the pwm-bcm2835 is not probed.
            if(try_then_request_module(pwms[i], "pwm-bcm2835") < 0) {
                pr_warn("%s: WARN: PWM channel %d is not initialized. Make sure the pwm-bcm2835 driver is probed", THIS_MODULE->name, i); 
                return -ENXIO;
            }
            // Bad pointer address.
            if(IS_ERR(pwms[i])) {
                int err = ERR_CAST(pwms[i]);
                pr_warn("%s: WARN: PWM channel: %d is unavailable, error code: %d", THIS_MODULE->name, i, err);
                return err;
            }

            // Adaptive PWM is reserved for IOCTL use.
            if(config->pwm_mode == PWM_ADP) {
                pr_info("WHY %ld", pwm_s->duty_cycle);
                pwm_s->duty_cycle = extra;
                pwm_apply_state(pwms[i], pwm_s);
            } else {
                pwm_s->duty_cycle = (PWM_PERIOD / PWM_OFF) * config->pwm_mode;
                pwm_apply_state(pwms[i], pwm_s);
            }
        }
    );
    pr_info("%s: PWM configuration changed successfully.", THIS_MODULE->name);
    return 0;
}

/* Obtains the PWM device. */
int init_fan_pwm(void) {
    // We require the pwm driver to be available for obtaining the PWM channels.
    if(request_module("pwm-bcm2835")) {
        pr_err("%s: ERROR: Unable to request the PWM driver. Please check that \"pwm-bcm2835\" driver exists.", THIS_MODULE->name);
        return -ENODEV;
    }
#ifndef LEGACY
    if(platform_driver_register(&rpi_platform_driver)) {
        pr_err("%s: ERROR: Unable to load platform driver.", THIS_MODULE->name);
        return -EPROBE_DEFER;
    }

#else
    FOR_EACH_CHANNEL(i,
        pwms[i] = pwm_request(i, NULL);
        if(IS_ERR(pwms[i])) {
                pr_err("%s: ERROR: Requesting PWM channel: %d failed: %d", THIS_MODULE->name, i, err);
            return -EIO; 
        }

        pwm_apply_state(pwms[i], pwm_s);
    );
#endif    

    return 0;
}

/* Frees the PWM device. */
void free_fan_pwm(void) {    
#ifndef LEGACY
    platform_driver_unregister(&rpi_platform_driver);    // Platform driver destructor handles the freeing.
#else    
    FOR_EACH_CHANNEL(i, pwm_free(pwms[i]););                                  // Legacy freeing.
#endif    
}
