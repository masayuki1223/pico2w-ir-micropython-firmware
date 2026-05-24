/*
 * Copyright (c) 2025 Indoor Corgi
 *
 * SPDX-License-Identifier: MIT
 */

#include "infrared.h"

#include "hardware/clocks.h"
#include "infrared.pio.h"

PIO send_pio;
uint send_sm;
uint send_offset;

PIO receive_pio;
uint receive_sm;
uint receive_offset;

// 送信用PIOの割り当て, 初期化
bool infrared_send_init() {
  if (!pio_claim_free_sm_and_add_program_for_gpio_range(
          &infrared_send_program, &send_pio, &send_sm, &send_offset, INFRARED_SEND_PIN, 1, true)) {
    return false;
  }
  infrared_send_program_init(send_pio, send_sm, send_offset, INFRARED_SEND_PIN);
  return true;
}

// 送信用PIOプログラム初期化
void infrared_send_program_init(PIO pio, uint sm, uint offset, uint pin) {
  pio_gpio_init(pio, pin);
  pio_sm_config c = infrared_send_program_get_default_config(offset);
  sm_config_set_sideset_pins(&c, pin);
  sm_config_set_out_shift(&c, false, true, 32);
  sm_config_set_clkdiv(&c, ((float)clock_get_hz(clk_sys)) / 1000000);  // 1us/命令
  sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
  pio_sm_init(pio, sm, offset, &c);
  pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);
  pio_sm_set_enabled(pio, sm, true);
}

// 送信用PIOの開放
void infrared_send_deinit() {
  pio_remove_program_and_unclaim_sm(&infrared_send_program, send_pio, send_sm, send_offset);
}

// 赤外線送信
//
// Args:
//   data: 偶数要素は38KHz信号を送信(ON)する時間[us]. 奇数要素は間の信号停止(OFF)時間[us].
//   length: 送信する要素数
//   wait_complete: FIFOのデータが全て送信されるまで待機
//
// Returns: 送信成功でtrue. PIO初期化失敗でfalse.
void infrared_send(uint32_t* data, uint length, bool wait_complete) {
  uint i = 0;  // 送信中のインデックス
  while (1) {
    // 偶数要素
    if (i >= length) break;

    uint burst_loop = data[i] / 26;
    uint mod = data[i] % 26;  // 余りは次のOFF時間に加算する
    if (burst_loop == 0) {
      burst_loop++;  // 最低1周期はON送信
      mod = 0;
    }
    pio_sm_put_blocking(send_pio, send_sm, burst_loop - 1);  // PIO仕様により繰り返し回数-1を入力
    i++;

    // 奇数要素
    if (i >= length) {
      pio_sm_put_blocking(send_pio, send_sm, 0);  // 合計が偶数になるように調整して送信終了
      break;
    }
    uint space = data[i] + mod;
    if (space == 0) space++;                            // 最低1[us]はOFF時間が必要
    pio_sm_put_blocking(send_pio, send_sm, space - 1);  // PIO仕様によりOFF時間[us]-1を入力

    i++;
  }

  while (1) {
    if (pio_sm_is_tx_fifo_empty(send_pio, send_sm)) break;
    infrared_delay(1);
  }
}

// 受信用PIOの割り当て
bool infrared_receive_init() {
  if (!pio_claim_free_sm_and_add_program_for_gpio_range(
          &infrared_receive_program, &receive_pio, &receive_sm, &receive_offset, INFRARED_RECEIVE_PIN, 1, true)) {
    return false;
  }
  return true;
}

// 受信用PIOプログラム初期化
void infrared_receive_program_init(PIO pio, uint sm, uint offset, uint pin) {
  gpio_pull_up(pin);  // デフォルトでプルダウンされているので必須
  pio_gpio_init(pio, pin);

  pio_sm_config c = infrared_receive_program_get_default_config(offset);
  sm_config_set_in_pins(&c, pin);
  sm_config_set_jmp_pin(&c, pin);
  sm_config_set_out_shift(&c, false, true, 32);
  sm_config_set_in_shift(&c, false, true, 32);
  sm_config_set_clkdiv(&c, ((float)clock_get_hz(clk_sys)) / 10 / 1000000);  // 0.1us/命令
  pio_sm_init(pio, sm, offset, &c);
  pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, false);
  pio_sm_set_enabled(pio, sm, true);
}

// 受信用PIOの開放
void infrared_receive_deinit() {
  pio_remove_program_and_unclaim_sm(&infrared_receive_program, receive_pio, receive_sm, receive_offset);
}

// 赤外線受信を開始し, 1回分のデータを受信すると制御を返す
//
// Args:
//   data: 受信データ格納バッファー. 偶数要素は38KHz信号を送信(ON)する時間[us]. 奇数要素は間の信号停止(OFF)時間[us].
//   length: バッファーの最大要素数.
//
// Returns: 受信した要素数.
int infrared_receive_blocking(uint32_t* data, uint length) {
  // 1回分のデータを受信するとPIOプログラムが停止する仕様のため, 再度初期化する
  infrared_receive_program_init(receive_pio, receive_sm, receive_offset, INFRARED_RECEIVE_PIN);

  pio_sm_put_blocking(receive_pio, receive_sm, INFRARED_RECEIVE_THRESHOLD);  // しきい値をPIOプログラムへ送信
  uint i = 0;
  while (1) {
    uint32_t rec = pio_sm_get_blocking(receive_pio, receive_sm);
    if (rec == 0) break;
    data[i++] = rec;
    if (i >= length) break;
  }
  return i;
}
