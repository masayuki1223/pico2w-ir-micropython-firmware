/*
 * Copyright (c) 2025 Indoor Corgi
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "tmc5240_selector.h"

// RPZ-Stepper用サンプルコード
// 電流設定, 電流値とドライバーIC温度測定

// モーター制御クラスのインスタンスを作成
// 第1引数: 使用モーターの1回転のフルステップ数. 省略すると200
// 第2引数: 基板のモーター選択. 0ならモーター#1, 1ならモーター#2
TMC5240Selector m1(200, 0);

int main() {
  stdio_init_all();
  sleep_ms(3000);  // シリアルモニターを接続するための待機時間
  printf("\nProgram start\n");
  m1.init();  // GPIO/SPIを初期化

  // 電流の設定
  m1.set_ifs(0.5);          // モーターの定格に合わせて電流値を設定 (例:0.5A)
  m1.set_irun(31);        // 回転時電流を0-31で指定. 初期値31
  m1.set_ihold(31);       // 停止時電流を0-31で指定. 初期値8
  m1.set_tpowerdown(10);  // 停止後irunを保持する時間 = tpowerdown x 21[ms]. 0-255で指定. 初期値10.

  // 必ず行う設定
  m1.set_vmax_rpm(90);  // 回転速度
  m1.set_amax(500);     // 加速
  m1.set_dmax(500);     // 減速
  m1.enable();          // ドライバーの出力をONにしてモーターに電圧印加

  sleep_ms(3000);
  printf("Current(ihold=31): %fA\n", m1.board_current());  // board_currentで基板の電流合計を取得して表示

  m1.set_ihold(4);  // 停止時の電流を4に下げる
  sleep_ms(3000);
  printf("Current(ihold=4): %fA\n", m1.board_current());

  m1.set_rampmode(TMC5240::RAMPMODE_VELOCITY_POSITIVE);  // 定速で回転
  sleep_ms(3000);
  printf("Current(irun=31): %fA\n", m1.board_current());

  m1.set_irun(16);  // 回転中の電流を16に下げる
  sleep_ms(3000);
  printf("Current(irun=16): %fA\n", m1.board_current());

  printf("Driver temp: %fC\n", m1.get_adc_temp());  // adc_tempでドライバーチップ温度を取得して表示

  m1.set_rampmode(TMC5240::RAMPMODE_POSITIONING);  // 位置設定モード, 初期位置に戻す

  printf("Finish\n");
  while (1) {
    sleep_ms(1000);
  }
}
