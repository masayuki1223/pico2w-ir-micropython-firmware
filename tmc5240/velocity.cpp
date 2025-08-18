/*
 * Copyright (c) 2025 Indoor Corgi
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "tmc5240_selector.h"

// RPZ-Stepper用サンプルコード
// モーターを指定速度(回転数)で回転させる

// モーター制御クラスのインスタンスを作成
// 第1引数: 使用モーターの1回転のフルステップ数. 省略すると200
// 第2引数: 基板のモーター選択. 0ならモーター#1, 1ならモーター#2
TMC5240Selector m1(200, 0);

int main() {
  stdio_init_all();
  sleep_ms(3000);  // シリアルモニターを接続するための待機時間
  printf("\nProgram start\n");
  m1.init();  // GPIO/SPIを初期化

  // 必ず行う設定
  m1.set_rampmode(TMC5240::RAMPMODE_VELOCITY_POSITIVE);  // 速度指定モードにする
  m1.set_ifs(0.5);                                       // モーターの定格に合わせて電流値を設定 (例:0.5A)
  m1.set_vmax_rpm(0);                                    // 回転速度. 最初は停止させておくため0にする
  m1.set_amax(500);                                      // 加速, 減速
  m1.enable();                                           // ドライバーの出力をONにしてモーターに電圧印加

  // 速度を設定するとモーターが回転しはじめる
  m1.set_vmax_rpm(20);

  // 指定速度に到達するまで待機
  // velocity_reachedが0: 指定速度に未到達
  // velocity_reachedが1: 指定速度に到達
  while (0 == m1.get_velocity_reached()) {
    sleep_ms(1000);
  }
  sleep_ms(3000);  // しばらく指定速度で回転

  m1.set_vmax_rpm(60);  // 速度を変更
  sleep_ms(5000);

  m1.set_vmax_rpm(0);  // 停止

  // 停止するまで待機
  while (0 == m1.get_velocity_reached()) {
    sleep_ms(1000);
  }

  m1.set_rampmode(TMC5240::RAMPMODE_VELOCITY_NEGATIVE);  // 回転方向を逆にする

  m1.set_vmax_rpm(30);  // 回転
  sleep_ms(5000);

  m1.set_vmax_rpm(0);  // 停止

  // 停止するまで待機
  while (0 == m1.get_velocity_reached()) {
    sleep_ms(1000);
  }

  m1.set_rampmode(TMC5240::RAMPMODE_POSITIONING);  // 位置指定モードに戻す

  // 速度指定モードで回転中も現在位置xactualは変動する
  // 位置指定モードで動作させる場合, 必要に応じて現在位置を0に戻す
  m1.set_xactual(0);

  printf("Finish\n");
  while (1) {
    sleep_ms(1000);
  }
}
