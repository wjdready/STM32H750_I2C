#pragma once

#include <stdint.h>

void usb_process_data(uint8_t *dat, size_t len);

int usb_read_from_rb(uint8_t *dat, size_t len);

void usb_com_init(void);

int usb_write(void *dat, size_t len, int timeout_ms);
