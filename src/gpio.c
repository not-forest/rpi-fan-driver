/* 
 *  file: gpio.c
 *
 *  Module for parsing GPIO requests.
 *
 *  Tested with Linux raspberry pi 4, kernel built with buildroot: 6.1.61-v8
 * */

#include<linux/module.h>
#include<linux/gpio.h>
#include"rpi.h"

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
#define GPIO_NAME "FAN_GPIO"

/* sets the new GPIO while parsing values and returning obtained errors */
int set_gpio(union fan_config *config, uint8_t old_gpio) {
    uint8_t new_gpio = config->gpio_num;
    // Parsing the configuration value.
    switch (new_gpio) {
        case OOR:
            pr_err("%s: ERROR: GPIO_%d is not a proper pin, ignoring...\n", THIS_MODULE->name, new_gpio);
            return -EINVAL;
        case RESERVED:
            pr_warn("%s: WARN: GPIO_%d is reserved for advanced use and is not recomended to use (ID_EEPROM) pins.\n", 
                    THIS_MODULE->name, new_gpio);
        case PWM_GPIOS:
            if(set_fan_pwm(config, 0) == 0) goto _gpio; else break;
        default:
            pr_warn("%s: WARN: GPIO_%d is not a PWM pin. PWM configuration will be ignored.\n", 
                THIS_MODULE->name, new_gpio);

    }
    // Changing the current GPIO state
    if(old_gpio != new_gpio) { 
        if(gpio_request(new_gpio, GPIO_NAME) < 0) {
            pr_err("%s: ERROR: GPIO_%d request failed.\n", THIS_MODULE->name, new_gpio);
            return -EACCES;
        }

        gpio_set_value(old_gpio, LOW);
        gpio_free(old_gpio);

        gpio_direction_output(new_gpio, OUT);
        gpio_set_value(new_gpio, HIGH);
    }
_gpio:
    pr_info("%s: New configuration is provided: GPIO_%d, PWM_MODE_%d\n", 
            THIS_MODULE->name, new_gpio, config->pwm_mode);

    return 0;
}

/* Initializes the first available GPIO in the system. */
int init_gpio(union fan_config *config) {
    uint8_t gpio = GPIO_MIN;
    for(; gpio < GPIO_AMOUNT; ++gpio) {
        if(gpio_is_valid(gpio)) { 
            // When a valid GPIO is found, requesting it.
            if(gpio_request(gpio, GPIO_NAME) < 0) {
                pr_err("%s: ERROR: GPIO_%d request failed...\n", THIS_MODULE->name, gpio);
                gpio_free(gpio);
            } else break;
        }
    }

    // This check is true if all GPIO requests would fail or all GPIOs are not valid.
    if(gpio == GPIO_AMOUNT) {
        pr_err("%s: Requests failed. All pins are in use, aborting..\n", THIS_MODULE->name);
        return -EACCES;
    }   

    gpio_direction_output(gpio, OUT);
    gpio_set_value(gpio, HIGH);

    config->pwm_mode = PWM_OFF;
    config->gpio_num = gpio;

    return 0;
}

/* Frees the current GPIO */
void free_cgpio(union fan_config *config) {
    gpio_set_value(config->gpio_num, LOW);
    gpio_free(config->gpio_num);
}
