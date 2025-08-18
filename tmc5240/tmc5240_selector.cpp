/*
 * Copyright (c) 2025 Indoor Corgi
 *
 * SPDX-License-Identifier: MIT
 */

#include "tmc5240_selector.h"

// コンストラクタ
//
// Args:
//   motor_id: モーター選択. 0:モーター#1. 1:モーター#2.
//   board_id: 基板選択. -1:基板1枚のみ. 0:基板#1. 1:基板#2.
TMC5240Selector::TMC5240Selector(uint steps_per_rev,
                                 uint motor_id,
                                 int board_id)
    : motor_id(motor_id),
      board_id(board_id) {
  TMC5240::steps_per_rev = steps_per_rev;
  if (motor_id == 0) {
    spi_csn_pin = MOTOR1_CSN_PIN;
  } else if (motor_id == 1) {
    spi_csn_pin = MOTOR2_CSN_PIN;
  } else {
    panic("TMC5240Selector: motor_id is out of range");
  }
  if (board_id < -1 || board_id > 1) panic("TMC5240Selector: board_id is out of range");
}

// SPI/GPIOを初期化
void TMC5240Selector::init() {
  if (board_id == 0 || board_id == 1) {
    gpio_init(BOARD_SELECT_PIN);
    gpio_set_dir(BOARD_SELECT_PIN, GPIO_OUT);
  }

  // 混信を避けるため, もう一方のmotor_idのCSも初期化が必要
  if (motor_id == 0) {
    gpio_init(MOTOR2_CSN_PIN);
    gpio_set_dir(MOTOR2_CSN_PIN, GPIO_OUT);
    gpio_put(MOTOR2_CSN_PIN, 1);
  } else if (motor_id == 1) {
    gpio_init(MOTOR1_CSN_PIN);
    gpio_set_dir(MOTOR1_CSN_PIN, GPIO_OUT);
    gpio_put(MOTOR1_CSN_PIN, 1);
  }

  TMC5240::init();
}

// 基板選択信号制御
// board_idが0ならboard_select_gpioをLow
// board_idが1ならboard_select_gpioをHigh
void TMC5240Selector::select_board() {
  if (board_id == 0 || board_id == 1) {
    gpio_put(BOARD_SELECT_PIN, board_id);
  }
}

// 基板の消費電流[A]を返す
// モーター制御中電流は変化しているので, 複数回測定して平均を計算
//
// Args:
//   meas_count: 測定回数
double TMC5240Selector::board_current(uint meas_count) {
  uint ain_sum = 0;
  for (uint i = 0; i < meas_count; i++) {
    ain_sum += get_adc_ain();
  }
  return (double)ain_sum / 27.9 / 7.5 / meas_count;
}

//---------------------------------
// Direct Register Access

// レジスターのデータを読み出す
//
// Args:
//   reg_addr: レジスターアドレス
//
// Returns: データ
uint32_t TMC5240Selector::read_register(uint8_t reg_addr) {
  select_board();
  return TMC5240::read_register(reg_addr);
}

// レジスターにデータを書き込む
//
// Args:
//   reg_addr: レジスターアドレス
//   data: データ
void TMC5240Selector::write_register(uint8_t reg_addr, uint32_t data) {
  select_board();
  TMC5240::write_register(reg_addr, data);
}
