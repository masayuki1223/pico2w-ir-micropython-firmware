/*
 * Copyright (c) 2025 Indoor Corgi
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "tmc5240_selector.h"

// RPZ-Stepper用サンプルコード
// 静音モード

// モーター制御クラスのインスタンスを作成
// 第1引数: 使用モーターの1回転のフルステップ数. 省略すると200
// 第2引数: 基板のモーター選択. 0ならモーター#1, 1ならモーター#2
TMC5240Selector m1(200, 0);

int main() {
  stdio_init_all();
  sleep_ms(3000);  // シリアルモニターを接続するための待機時間
  printf("\nProgram start\n");
  m1.init();  // GPIO/SPIを初期化

  // 静音モード有効
  m1.set_en_pwm_mode(1);

  // PWM周波数(通常は初期値の0で問題ない)
  //  0: 24.4kHz
  //  1: 36.6kHz
  //  2: 48.8kHz
  m1.set_pwm_freq(0);

  // 必ず行う設定
  m1.set_ifs(0.5);      // モーターの定格に合わせて電流値を設定 (例:0.5A)
  m1.set_vmax_rpm(90);  // 回転速度
  m1.set_amax(500);     // 加速
  m1.set_dmax(500);     // 減速
  m1.enable();          // ドライバーの出力をONにしてモーターに電圧印加

  m1.set_xtarget(153600);  // 回転
  sleep_ms(5000);

  // オプション機能
  // tpwmthrsで回転数が一定以上になったら静音モードを解除できる
  m1.set_tpwmthrs_rpm(60);  // 約60rpmを超えると静音モード解除

  m1.set_xtarget(0);  // 元の位置に戻す
  sleep_ms(5000);

  m1.set_tpwmthrs(0);  // 静音モードを回転数に関わらず有効に戻す

  // オプション機能
  // freewheelで停止時の動作を変更可能(停止時電流ihold=0のときに有効)
  //  0: 通常の動作. モーター位置を保持する
  //  1: フリーホイール. 電流を流さず, モーターが外力で回転する
  //  2: パッシブブレーキ. 電流を流さず, モーターが外力で回転するがブレーキがかかる
  m1.set_freewheel(1);
  m1.set_ihold(0);

  printf("Finish\n");
  while (1) {
    sleep_ms(1000);
  }
}
