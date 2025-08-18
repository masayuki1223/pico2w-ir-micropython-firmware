/*
 * Copyright (c) 2025 Indoor Corgi
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef TMC5240_SELECTOR_H
#define TMC5240_SELECTOR_H

#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "tmc5240.h"

// TMC5240にRPZ-Stepper拡張基板のモーター選択, ボード選択機能を追加
class TMC5240Selector : public TMC5240 {
 public:
  // GPIO
  static constexpr uint MOTOR1_CSN_PIN = 17;
  static constexpr uint MOTOR2_CSN_PIN = 21;
  static constexpr uint BOARD_SELECT_PIN = 22;

  // モーター, 基板の指定
  uint motor_id;
  int board_id;

  TMC5240Selector(uint steps_per_rev = DEFAULT_STEPS_PER_REV,
                  uint motor_id = 0,
                  int board_id = -1);
  void init();
  void select_board();
  void chip_enable(bool enable);
  double board_current(uint meas_count = 1000);

  //---------------------------------
  // Direct Register Access

  uint32_t read_register(uint8_t reg_addr);
  void write_register(uint8_t reg_addr, uint32_t data);
};

#endif
