#include "bluetooth.h"
#include <zephyr/settings/settings.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/drivers/gpio.h>


#include <bluetooth/services/lbs.h>

enum motor_command_t {
    START_MOTOR = 0x01,
    MOTOR_READY = 0x02,
};

static struct bt_conn *default_conn;
static struct bt_gatt_notify_params notify_params;
const struct device *gpio_dev;

static struct gpio_callback gpio_cb;

#define INPUT_PIN 11  // Example input pin
#define INPUT_PIN_FLAGS (GPIO_INT_EDGE_TO_ACTIVE)

#define CUSTOM_SERVICE_UUID BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x9ABC, 0xDEF123456789))

#define START_MOTOR_CHAR_UUID BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x9ABC, 0xDEF123456780))
#define MOTOR_READY_CHAR_UUID BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x9ABC, 0xDEF123456781))

#define NOTIFICATION_INTERVAL K_SECONDS(1)


// Interrupt handler for input pin
void gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    uint8_t data = 1;  // Example data to signal the receiver

    if (default_conn) {
        notify_params.data = &data;
        notify_params.len = sizeof(data);
        notify_params.uuid = CUSTOM_SERVICE_UUID;
        notify_params.attr = NULL;
        bt_gatt_notify_cb(default_conn, &notify_params);
    }
}

void configure_input_pin()
{
    gpio_dev = device_get_binding("GPIO_0");
    gpio_pin_configure(gpio_dev, INPUT_PIN, GPIO_INPUT | INPUT_PIN_FLAGS);
    gpio_pin_interrupt_configure(gpio_dev, INPUT_PIN, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_init_callback(&gpio_cb, gpio_callback, BIT(INPUT_PIN));
    gpio_add_callback(gpio_dev, &gpio_cb);
}

#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)

static struct bt_conn *connection_handle = NULL;  // Global handle for BLE connection

uint16_t start_motor_char_handle = NULL;

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LBS_VAL),
};

void on_motor_command_received(uint8_t command) {
    if (command == START_MOTOR) {
        //start_motor();
        // Once motor operation is finished:
        //send_motor_ready_notification();
    }
}

void send_start_motor_command() {
    uint8_t command = START_MOTOR;
    bt_gatt_write_without_response(&connection_handle, start_motor_char_handle, &command, sizeof(command), false);
}

void connected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        printk("Failed to connect (err %u)\n", err);
    } else {
        printk("Connected\n");
        connection_handle = bt_conn_ref(conn);  // Store the connection handle
    }
}

void disconnected(struct bt_conn *conn, uint8_t reason) {
    printk("Disconnected (reason %u)\n", reason);
    if (connection_handle) {
        bt_conn_unref(connection_handle);  // Dereference the connection
        connection_handle = NULL;
    }
}

#ifdef CONFIG_BT_LBS_SECURITY_ENABLED
static void security_changed(struct bt_conn *conn, bt_security_t level,
                             enum bt_security_err err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (!err) {
        printk("Security changed: %s level %u\n", addr, level);
    } else {
        printk("Security failed: %s level %u err %d\n", addr, level, err);
    }
}
#endif

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected        = connected,
    .disconnected     = disconnected,
#ifdef CONFIG_BT_LBS_SECURITY_ENABLED
    .security_changed = security_changed,
#endif
};

static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
};

#if defined(CONFIG_BT_LBS_SECURITY_ENABLED)
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Pairing cancelled: %s\n", addr);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Pairing completed: %s, bonded: %d\n", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Pairing failed conn: %s, reason %d\n", addr, reason);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
    .passkey_display = auth_passkey_display,
    .cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
    .pairing_complete = pairing_complete,
    .pairing_failed = pairing_failed
};
#else
static struct bt_conn_auth_cb conn_auth_callbacks;
static struct bt_conn_auth_info_cb conn_auth_info_callbacks;
#endif

void bluetooth_init(void)
{
    int err;

    if (IS_ENABLED(CONFIG_BT_LBS_SECURITY_ENABLED)) {
        err = bt_conn_auth_cb_register(&conn_auth_callbacks);
        if (err) {
            printk("Failed to register authorization callbacks.\n");
            return;
        }

        err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
        if (err) {
            printk("Failed to register authorization info callbacks.\n");
            return;
        }
    }

    err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    printk("Bluetooth initialized\n");

    // Register connection callbacks
    bt_conn_cb_register(&conn_callbacks);

    if (IS_ENABLED(CONFIG_SETTINGS)) {
        settings_load();
    }
}

void bluetooth_start_advertising(void)
{
    int err;

    err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
                          sd, ARRAY_SIZE(sd));
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }

    printk("Advertising successfully started\n");
}
