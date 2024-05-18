/* 
 *  file: gpio.c
 *
 *  Module for parsing GPIO requests and PWM management.
 *
 *  Tested with Linux raspberry pi 4, kernel built with buildroot: 6.1.61-v8
 * */

#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/fs.h>
#include<linux/gpio.h>
#include<linux/pwm.h>
#include"rpi.h"

/* sets the new GPIO while parsing values and returning obtained errors */
int set_gpio(union fan_config *config, uint8_t gpio) {
    uint8_t new_gpio = config->gpio_num;
    // Parsing the configuration value.
    switch (new_gpio) {
        case RESERVED:
            pr_warn("%s: WARN: GPIO_%d is reserved for advanced use and is not recomended to use (ID_EEPROM) pins.\n", 
                THIS_MODULE->name, new_gpio); 
            break;
        case OOR:
            pr_err("%s: ERROR: GPIO_%d is not a proper pin, ignoring...\n", THIS_MODULE->name, new_gpio);
            return -EFAULT;
        case PWM_GPIOS:
            // TODO!
            break;
        default:
            pr_warn("%s: WARN: GPIO_%d is not a PWM pin. PWM configuration will be ignored.\n", 
                THIS_MODULE->name, new_gpio);
    }

    // Changing the current GPIO state
    if(gpio != new_gpio) { 
        if(gpio_request(new_gpio, GPIO_NAME) < 0) {
            pr_err("%s: ERROR: GPIO_%d request failed.\n", THIS_MODULE->name, new_gpio);
            return -EFAULT;
        }

        gpio_direction_output(new_gpio, OUT);
        gpio_set_value(new_gpio, HIGH);

        gpio_set_value_cansleep(gpio, LOW);
        gpio_free(gpio);
    }

    pr_info("%s: New configuration is provided: GPIO_%d, PWM_MODE_%d\n", 
            THIS_MODULE->name, new_gpio, config->pwm_mode);

    return 0;
}
