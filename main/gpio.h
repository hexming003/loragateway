/*
 * gpio.h
 *
 *  Created on: May 8, 2019
 *      Author: root
 */

#ifndef GPIO_H_
#define GPIO_H_

int dio0_read_state(void);
int gpioInitialise(void);
int gpio_to_mem(void);
int set_lora_reset_pin_high(void);
int set_lora_reset_pin_low(void);

#endif /* GPIO_H_ */
