#include "application.h"
#include "../bluetooth/bluetooth.h"
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/settings/settings.h>
#include <bluetooth/services/lbs.h>

typedef enum {
    CW,   // 0
    CCW   // 1
} direction;

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(redled), gpios);

/* Declare GPIO device structs */
static const struct gpio_dt_spec gate_pin_1 = GPIO_DT_SPEC_GET(DT_NODELABEL(gate_1), gpios);
static const struct gpio_dt_spec gate_pin_2 = GPIO_DT_SPEC_GET(DT_NODELABEL(gate_2), gpios);
static const struct gpio_dt_spec sw_pin_1 = GPIO_DT_SPEC_GET(DT_NODELABEL(sw_1), gpios);
static const struct gpio_dt_spec sw_pin_2 = GPIO_DT_SPEC_GET(DT_NODELABEL(sw_2), gpios);
static const struct gpio_dt_spec pir_pin_1 = GPIO_DT_SPEC_GET(DT_NODELABEL(pir_1), gpios);
static const struct gpio_dt_spec sw_pwr_pin_1 = GPIO_DT_SPEC_GET(DT_NODELABEL(sw_pwr_1), gpios);
static const struct gpio_dt_spec sw_pwr_pin_2 = GPIO_DT_SPEC_GET(DT_NODELABEL(sw_pwr_2), gpios);

void init_pins()
{
    gpio_pin_configure_dt(&led, GPIO_OUTPUT);
    gpio_pin_configure_dt(&gate_pin_1, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&gate_pin_2, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&sw_pwr_pin_1, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&sw_pwr_pin_2, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&sw_pin_1, GPIO_INPUT);
    gpio_pin_configure_dt(&sw_pin_2, GPIO_INPUT);
    gpio_pin_configure_dt(&pir_pin_1, GPIO_INPUT);
}

void run_zipline(int direction)
{
    switch (direction)
    {
        case CW:
            gpio_pin_set_dt(&gate_pin_2, 0);
            gpio_pin_set_dt(&gate_pin_1, 1);
            break;
        case CCW:
            gpio_pin_set_dt(&gate_pin_1, 0);
            gpio_pin_set_dt(&gate_pin_2, 1);
            break;
        default:
            gpio_pin_set_dt(&gate_pin_1, 0);
            gpio_pin_set_dt(&gate_pin_2, 0);
            break;
    }
}

void run_application()
{
    printk("Starting application\n");

    init_pins();

    bluetooth_init();

    bluetooth_start_advertising();

    while (1) 
    {
        if (gpio_pin_get_dt(&sw_pin_1))
        {
            gpio_pin_set_dt(&led, 0);
        }
        else
        {
            gpio_pin_set_dt(&led, 1);
        }
        k_msleep(1000);
    }
}
    