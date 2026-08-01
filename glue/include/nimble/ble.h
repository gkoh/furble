#ifndef NIMBLE_STUB_BLE_H_
#define NIMBLE_STUB_BLE_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BLE_DEV_ADDR_LEN
#define BLE_DEV_ADDR_LEN (6)
#endif

#ifndef BLE_ERR_REM_USER_CONN_TERM
#define BLE_ERR_REM_USER_CONN_TERM (0x13)
#endif

#ifndef BLE_HS_CONN_HANDLE_NONE
#define BLE_HS_CONN_HANDLE_NONE (0xffff)
#endif

#ifndef BLE_OWN_ADDR_PUBLIC
#define BLE_OWN_ADDR_PUBLIC (0x00)
#endif
#ifndef BLE_OWN_ADDR_RANDOM
#define BLE_OWN_ADDR_RANDOM (0x01)
#endif
#ifndef BLE_OWN_ADDR_RPA_PUBLIC_DEFAULT
#define BLE_OWN_ADDR_RPA_PUBLIC_DEFAULT (0x02)
#endif
#ifndef BLE_OWN_ADDR_RPA_RANDOM_DEFAULT
#define BLE_OWN_ADDR_RPA_RANDOM_DEFAULT (0x03)
#endif

#ifndef BLE_ADDR_PUBLIC
#define BLE_ADDR_PUBLIC (0x00)
#endif
#ifndef BLE_ADDR_RANDOM
#define BLE_ADDR_RANDOM (0x01)
#endif

#ifndef BLE_HS_IO_DISPLAY_ONLY
#define BLE_HS_IO_DISPLAY_ONLY 0
#endif
#ifndef BLE_HS_IO_DISPLAY_YESNO
#define BLE_HS_IO_DISPLAY_YESNO 1
#endif
#ifndef BLE_HS_IO_KEYBOARD_ONLY
#define BLE_HS_IO_KEYBOARD_ONLY 2
#endif
#ifndef BLE_HS_IO_NO_INPUT_OUTPUT
#define BLE_HS_IO_NO_INPUT_OUTPUT 3
#endif
#ifndef BLE_HS_IO_KEYBOARD_DISPLAY
#define BLE_HS_IO_KEYBOARD_DISPLAY 4
#endif

#ifndef BLE_SM_PAIR_KEY_DIST_ENC
#define BLE_SM_PAIR_KEY_DIST_ENC 0x01
#endif
#ifndef BLE_SM_PAIR_KEY_DIST_ID
#define BLE_SM_PAIR_KEY_DIST_ID 0x02
#endif
#ifndef BLE_SM_PAIR_KEY_DIST_SIGN
#define BLE_SM_PAIR_KEY_DIST_SIGN 0x04
#endif
#ifndef BLE_SM_PAIR_KEY_DIST_LINK
#define BLE_SM_PAIR_KEY_DIST_LINK 0x08
#endif

#ifndef BLE_ATT_ATTR_MAX_LEN
#define BLE_ATT_ATTR_MAX_LEN 512
#endif

#ifndef BLE_GAP_ROLE_MASTER
#define BLE_GAP_ROLE_MASTER 0
#endif
#ifndef BLE_GAP_ROLE_SLAVE
#define BLE_GAP_ROLE_SLAVE 1
#endif

#ifndef BLE_GAP_LE_PHY_1M
#define BLE_GAP_LE_PHY_1M 0x01
#endif
#ifndef BLE_GAP_LE_PHY_2M
#define BLE_GAP_LE_PHY_2M 0x02
#endif
#ifndef BLE_GAP_LE_PHY_CODED
#define BLE_GAP_LE_PHY_CODED 0x03
#endif

#ifndef BLE_HCI_LE_PHY_1M
#define BLE_HCI_LE_PHY_1M 0x01
#endif

#ifndef BLE_GATT_CHR_F_BROADCAST
#define BLE_GATT_CHR_F_BROADCAST 0x0001
#endif
#ifndef BLE_GATT_CHR_F_READ
#define BLE_GATT_CHR_F_READ 0x0002
#endif
#ifndef BLE_GATT_CHR_F_WRITE_NO_RSP
#define BLE_GATT_CHR_F_WRITE_NO_RSP 0x0004
#endif
#ifndef BLE_GATT_CHR_F_WRITE
#define BLE_GATT_CHR_F_WRITE 0x0008
#endif
#ifndef BLE_GATT_CHR_F_NOTIFY
#define BLE_GATT_CHR_F_NOTIFY 0x0010
#endif
#ifndef BLE_GATT_CHR_F_INDICATE
#define BLE_GATT_CHR_F_INDICATE 0x0020
#endif
#ifndef BLE_GATT_CHR_F_AUTH_SIGN_WRITE
#define BLE_GATT_CHR_F_AUTH_SIGN_WRITE 0x0040
#endif
#ifndef BLE_GATT_CHR_F_EXTENDED
#define BLE_GATT_CHR_F_EXTENDED 0x0080
#endif
#ifndef BLE_GATT_CHR_F_READ_ENC
#define BLE_GATT_CHR_F_READ_ENC 0x0200
#endif
#ifndef BLE_GATT_CHR_F_READ_AUTHEN
#define BLE_GATT_CHR_F_READ_AUTHEN 0x0400
#endif
#ifndef BLE_GATT_CHR_F_READ_AUTHOR
#define BLE_GATT_CHR_F_READ_AUTHOR 0x0800
#endif
#ifndef BLE_GATT_CHR_F_WRITE_ENC
#define BLE_GATT_CHR_F_WRITE_ENC 0x1000
#endif
#ifndef BLE_GATT_CHR_F_WRITE_AUTHEN
#define BLE_GATT_CHR_F_WRITE_AUTHEN 0x2000
#endif
#ifndef BLE_GATT_CHR_F_WRITE_AUTHOR
#define BLE_GATT_CHR_F_WRITE_AUTHOR 0x4000
#endif

typedef struct {
    uint8_t type;
    uint8_t val[BLE_DEV_ADDR_LEN];
} ble_addr_t;

#ifdef __cplusplus
}
#endif

#endif
