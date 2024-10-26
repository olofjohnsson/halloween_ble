#include "application.h"
#include "../bluetooth/bluetooth.h"
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/settings/settings.h>

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(redled), gpios);

/* Declare GPIO device structs */
static const struct gpio_dt_spec gate_pin_1 = GPIO_DT_SPEC_GET(DT_NODELABEL(gate_1), gpios);
static const struct gpio_dt_spec gate_pin_2 = GPIO_DT_SPEC_GET(DT_NODELABEL(gate_2), gpios);

void run_application()
{
    printk("Starting application\n");

    gpio_pin_configure_dt(&led, GPIO_OUTPUT);

    bluetooth_init();

    bluetooth_start_advertising();

    if (gate_pin_1.port)
    {
         gpio_pin_configure_dt(&gate_pin_1, GPIO_OUTPUT_INACTIVE);
    }
    if (gate_pin_2.port)
    {
         gpio_pin_configure_dt(&gate_pin_2, GPIO_OUTPUT_INACTIVE);
    }

    while (1) {
        k_msleep(1000);
        gpio_pin_set_dt(&led, 1);
        gpio_pin_set_dt(&gate_pin_1, 1);
        //send_start_motor_command();
        k_msleep(1000);
        gpio_pin_set_dt(&gate_pin_1, 0);
        gpio_pin_set_dt(&led, 0);
        k_msleep(1000);

        gpio_pin_set_dt(&led, 1);
        gpio_pin_set_dt(&gate_pin_2, 1);
        //send_start_motor_command();
        k_msleep(1000);
        gpio_pin_set_dt(&gate_pin_2, 0);
        gpio_pin_set_dt(&led, 0);
    }
}
    