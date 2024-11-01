#include "application.h"
#include "../bluetooth/bluetooth.h"
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/settings/settings.h>
#include <bluetooth/services/lbs.h>
#include <hal/nrf_power.h>

typedef enum {
    CW,
    CCW
} motor_direction;

typedef enum {
    NO_MOTION_DETECTED,
    MOTION_DETECTED,
    MOTION_CYCLE_ENDED
} pir_detection;

typedef enum {
    MOTOR_NOT_RUNNING,
    MOTOR_RUNNING,
    MOTOR_REWIND
} motor_activity;

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(redled), gpios);

/* Declare GPIO device structs */
static const struct gpio_dt_spec gate_pin_1 = GPIO_DT_SPEC_GET(DT_NODELABEL(gate_1), gpios);
static const struct gpio_dt_spec gate_pin_2 = GPIO_DT_SPEC_GET(DT_NODELABEL(gate_2), gpios);
static const struct gpio_dt_spec sw_pin_1 = GPIO_DT_SPEC_GET(DT_NODELABEL(sw_1), gpios);
static const struct gpio_dt_spec sw_pin_2 = GPIO_DT_SPEC_GET(DT_NODELABEL(sw_2), gpios);
static const struct gpio_dt_spec pir_pin_1 = GPIO_DT_SPEC_GET(DT_NODELABEL(pir_1), gpios);
static const struct gpio_dt_spec button_1 = GPIO_DT_SPEC_GET(DT_NODELABEL(button_1), gpios);
static const struct gpio_dt_spec sw_pin_light = GPIO_DT_SPEC_GET(DT_NODELABEL(sw_light), gpios);
static const struct gpio_dt_spec sw_pin_mp3 = GPIO_DT_SPEC_GET(DT_NODELABEL(sw_mp3), gpios);

/* Declare callbacks */
static struct gpio_callback sw_1_cb_data;
static struct gpio_callback sw_2_cb_data;
static struct gpio_callback pir_1_cb_data;

static uint8_t pir_motion = MOTION_CYCLE_ENDED;
static uint8_t motor = MOTOR_NOT_RUNNING;


 void light_off()
 {
    gpio_pin_set_dt(&sw_pin_light, 0);
 }

 void light_on()
 {
    gpio_pin_set_dt(&sw_pin_light, 1);
 }

 void mp3_on()
 {
    gpio_pin_set_dt(&sw_pin_mp3, 0);
    k_msleep(500);
    gpio_pin_set_dt(&sw_pin_mp3, 1);
 }

void run_zipline(int direction)
{
    motor = MOTOR_RUNNING;
    switch (direction)
    {
        case CW:
            gpio_pin_set_dt(&gate_pin_2, 1);
            break;
        case CCW:
            gpio_pin_set_dt(&gate_pin_1, 1);
            break;
        default:
            gpio_pin_set_dt(&gate_pin_1, 0);
            gpio_pin_set_dt(&gate_pin_2, 0);
            break;
    }
}

/* Interrupt callback function for zip line limit switch 1 */
void sw_1_activated(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    gpio_pin_set_dt(&gate_pin_1, 0);
    gpio_pin_set_dt(&gate_pin_2, 0);
    gpio_pin_set_dt(&led, 0);
    motor = MOTOR_REWIND;
}

/* Interrupt callback function for zip line limit switch 2 */
void sw_2_activated(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    gpio_pin_set_dt(&gate_pin_1, 0);
    gpio_pin_set_dt(&gate_pin_2, 0);
    gpio_pin_set_dt(&led, 1);
    motor = MOTOR_NOT_RUNNING;
    pir_motion = MOTION_CYCLE_ENDED;
}

/* Interrupt callback function for PIR motion sensor */
void pir_activated(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    if (pir_motion == MOTION_CYCLE_ENDED)
    {
        pir_motion = MOTION_DETECTED;
    }
}

void init_pins()
{
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&gate_pin_1, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&gate_pin_2, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&sw_pin_light, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&sw_pin_mp3, GPIO_OUTPUT_ACTIVE);
    //gpio_pin_configure_dt(&sw_pwr_1, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&sw_pin_1, GPIO_PULL_UP);
    gpio_pin_configure_dt(&sw_pin_2, GPIO_PULL_UP);
    gpio_pin_configure_dt(&pir_pin_1, GPIO_PULL_DOWN);
    gpio_pin_configure_dt(&button_1, GPIO_PULL_UP);

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

    while (1) 
    {
        if(pir_motion == MOTION_DETECTED)
        {
            mp3_on();
            light_on();
            run_zipline(CW);
            pir_motion = NO_MOTION_DETECTED;
        }

        if(motor == MOTOR_REWIND)
        {
            k_msleep(2000);
            light_off();
            k_msleep(10000);
            run_zipline(CCW);
            
        }
        k_msleep(1000);
    }
}
    