/* 
 *  file: pwm.c
 *
 *  Module for handling PWM management. Based on the provided configuration it initializes the PWM
 *  on the embedded target device via pwm.h API. The PWM interfaces are parsed from the device tree file,
 *  which must be appended to the root folder of this driver.
 *
 *  Tested with Linux raspberry pi 4, kernel built with buildroot: 6.1.61-v8
 * */

#include<linux/mod_devicetable.h>
#include<linux/property.h>
#include<linux/of_device.h>
#include<linux/version.h>
#include<linux/module.h>
#include<linux/pwm.h>

#include "rpi.h"

#define PWM_PERIOD 50000000

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 5, 0)
#define LEGACY  // Using legacy 'pwm_request' for older kernels.
#endif

/**************** Driver data fields ****************/
static struct pwm_state pwm_s = {
    .period = PWM_PERIOD,
    .duty_cycle = PWM_PERIOD,
    .polarity = PWM_POLARITY_NORMAL,
    .enabled = true,
};

// Obtained pointer to the PWM device.
static struct pwm_device *pwm ;
/****************************************************/

#ifndef LEGACY

/******** Must be configured per overlay ********/
#ifndef DEV_LABEL           // PWM label within the device-tree overlay.
#define DEV_LABEL "rpifan"
#endif
/************************************************/


#define PLATFORM_DRIVER_NAME "rpifan_pwm"
#define DT_DEV_COMP PLATFORM_DRIVER_NAME


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

    // Using NULL since our device has only one pwm phandle. For custom configs, pwm-names property
    // must be provided.
    pwm = pwm_get(dev, NULL);

    if(IS_ERR(pwm)) {
        if(PTR_ERR(pwm) != -EPROBE_DEFER) {
            pr_err("%s: ERROR: Requesting PWM failed: %d", PLATFORM_DRIVER_NAME, ERR_CAST(pwm));
            return ERR_PTR(pwm);
        }
    }

    return 0;
}

/* Platform driver destructor. Only handles the freeing. */
static int rpi_fan_pwm_remove(struct platform_device *pdev) {
    pr_info("%s: Platform driver removed. PWM is freed.\n", PLATFORM_DRIVER_NAME); 
    pwm_put(pwm);       // Freeing the device.
    return 0;
}
#else

#ifndef PWM_INDEX       // Legacy only!: PWM index. Zero is being used by default.
#define PWM_INDEX 0
#endif

#endif

/* Sets a new PWM to a certain GPIO based on the fan configuration */
int set_fan_pwm(union fan_config *config) {
    // Check for an adaptive PWM configuration.
    if(config->gpio_num == PWM_ADP) {
        goto _pwms_ok;
    }
    
    pwm_s.duty_cycle = (PWM_PERIOD / PWM_OFF) * config->pwm_mode;
    pwm_apply_state(pwm, &pwm_s);
_pwms_ok:
    pr_info("%s: PWM configuration changed successfully.", THIS_MODULE->name);
    return 0;
}

/* Obtains the PWM device. */
int init_fan_pwm(void) {
#ifndef LEGACY
    if(platform_driver_register(&rpi_platform_driver)) {
        pr_err("%s: ERROR: Unable to load platform driver.", THIS_MODULE->name);
        pwm = NULL;
        return -EPROBE_DEFER;
    }
#else
    pwm = pwm_request(PWM_INDEX, PWM_LABEL);
    if(IS_ERR(pwm)) {
        pr_err("%s: ERROR: Unable to obtain PWM device by index.", THIS_MODULE->name);
        pwm = NULL;
        return -EIO; 
    }
#endif    
    pwm_apply_state(pwm, &pwm_s);

    return 0;
}

/* Frees the PWM device. */
void free_fan_pwm(void) {    
#ifndef LEGACY
    platform_driver_unregister(&rpi_platform_driver);    // Platform driver destructor handles the freeing.
#else    
    pwm_free(pwm);                                  // Legacy freeing.
#endif    
}
