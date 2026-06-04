/*
 * Copyright (c) 2025 Indoor Corgi
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef INFRARED_H
#define INFRARED_H

#include "pico/stdlib.h"
#include "hardware/pio.h"

// -----------------
// Configurations

#define INFRARED_SEND_PIN 10    // 送信ピン番号
#define INFRARED_RECEIVE_PIN 2  // 受信ピン番号

// 受信時のしきい値[us]. この時間以上ONまたはOFFが続く場合は受信終了
#define INFRARED_RECEIVE_THRESHOLD 100000

#define infrared_delay(x) sleep_ms(x)  // xミリ秒待機

// -----------------

bool infrared_send_init();
void infrared_send_program_init(PIO pio, uint sm, uint offset, uint pin);
void infrared_send_deinit();
void infrared_send(uint32_t* data, uint length, bool wait_complete);

bool infrared_receive_init();
void infrared_receive_program_init(PIO pio, uint sm, uint offset, uint pin);
void infrared_receive_deinit();
int infrared_receive_blocking(uint32_t* data, uint length);

#endif
