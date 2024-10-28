#include "application.h"
#include "../bluetooth/bluetooth.h"
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/settings/settings.h>
#include <bluetooth/services/lbs.h>
#include <hal/nrf_power.h>

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
//static const struct gpio_dt_spec sw_pwr_1 = GPIO_DT_SPEC_GET(DT_NODELABEL(sw_pwr_1), gpios);

static struct gpio_callback sw_1_cb_data;
static struct gpio_callback sw_2_cb_data;
static struct gpio_callback pir_1_cb_data;

static uint8_t dir = CW;

void run_zipline(int direction)
{
    switch (direction)
    {
        case 0:
            gpio_pin_set_dt(&gate_pin_1, 0);
            k_msleep(50);
            gpio_pin_set_dt(&gate_pin_2, 1);
            break;
        case 1:
            gpio_pin_set_dt(&gate_pin_2, 0);
            k_msleep(50);
            gpio_pin_set_dt(&gate_pin_1, 1);
            
            break;
        default:
            gpio_pin_set_dt(&gate_pin_1, 0);
            gpio_pin_set_dt(&gate_pin_2, 0);
            break;
    }
}

/* Interrupt callback function */
void sw_1_activated(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    gpio_pin_set_dt(&gate_pin_1, 0);
    gpio_pin_set_dt(&gate_pin_2, 0);
    gpio_pin_set_dt(&gate_pin_1, 1);
    dir = CCW;
}

/* Interrupt callback function */
void sw_2_activated(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    gpio_pin_set_dt(&gate_pin_1, 0);
    gpio_pin_set_dt(&gate_pin_2, 0);
    gpio_pin_set_dt(&gate_pin_2, 1);
    dir = CW;
}

// Shared callback function
void pir_activated(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    //gpio_pin_set_dt(&led, 0);
}

void init_pins()
{
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&gate_pin_1, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&gate_pin_2, GPIO_OUTPUT_INACTIVE);
    //gpio_pin_configure_dt(&sw_pwr_1, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&sw_pin_1, GPIO_INPUT);
    gpio_pin_configure_dt(&sw_pin_2, GPIO_INPUT);
    gpio_pin_configure_dt(&pir_pin_1, GPIO_PULL_DOWN);

    /* Configure sw_pin_1 as interrupt on falling edge */
    gpio_pin_interrupt_configure_dt(&sw_pin_1, GPIO_INT_EDGE_FALLING);

    /* Initialize GPIO callback */
    gpio_init_callback(&sw_1_cb_data, sw_1_activated, BIT(sw_pin_1.pin));
    gpio_add_callback(sw_pin_1.port, &sw_1_cb_data);

    /* Configure sw_pin_2 as interrupt on falling edge */
    gpio_pin_interrupt_configure_dt(&sw_pin_2, GPIO_INT_EDGE_FALLING);

    /* Initialize GPIO callback */
    gpio_init_callback(&sw_2_cb_data, sw_2_activated, BIT(sw_pin_2.pin));
    gpio_add_callback(sw_pin_2.port, &sw_2_cb_data);

    /* Configure sw_pin_1 as interrupt*/
    gpio_pin_interrupt_configure_dt(&pir_pin_1, GPIO_INT_EDGE_RISING);

    /* Initialize GPIO callback */
    gpio_init_callback(&pir_1_cb_data, pir_activated, BIT(pir_pin_1.pin));
    gpio_add_callback(pir_pin_1.port, &pir_1_cb_data);
}

void run_application()
{
    init_pins();

    bluetooth_init();

    bluetooth_start_advertising();

    gpio_pin_set_dt(&led, 1);

    run_zipline(CW);

    while (1) 
    {
        k_msleep(1000);
    }
}
    