/*
 * Copyright (c) 2025 Indoor Corgi
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "tmc5240_selector.h"

// RPZ-Stepper用サンプルコード
// 2つのモーターを制御する

// モーター制御クラスのインスタンスを作成
// 第1引数: 使用モーターの1回転のフルステップ数. 省略すると200
// 第2引数: 基板のモーター選択. 0ならモーター#1, 1ならモーター#2
TMC5240Selector m1(200, 0);  // モーター1 (J3 MOTOR1端子)
TMC5240Selector m2(200, 1);  // モーター2 (J4 MOTOR2端子)

int main() {
  stdio_init_all();
  sleep_ms(3000);  // シリアルモニターを接続するための待機時間
  printf("\nProgram start\n");
  m1.init();  // GPIO/SPIを初期化 (どれか1つのモーターでinitすればよい)

  // モーター1の設定
  m1.set_ifs(0.5);      // モーターの定格に合わせて電流値を設定 (例:0.5A)
  m1.set_vmax_rpm(60);  // 回転速度
  m1.set_amax(500);     // 加速
  m1.set_dmax(500);     // 減速
  m1.enable();          // ドライバーの出力をONにしてモーターに電圧印加

  // モーター2の設定
  m2.set_ifs(0.5);      // モーターの定格に合わせて電流値を設定 (例:0.5A)
  m2.set_vmax_rpm(30);  // 回転速度
  m2.set_amax(300);     // 加速
  m2.set_dmax(300);     // 減速
  m2.enable();          // ドライバーの出力をONにしてモーターに電圧印加

  // モーター1を3回転, モーター2を1回転
  // xtargetを送信したらすぐに制御が戻るので, 両モーターが同時に回転する
  m1.set_xtarget(153600);
  m2.set_xtarget(51200);
  sleep_ms(5000);

  // モーター1, 2を元の位置に戻す
  m1.set_xtarget(0);
  m2.set_xtarget(0);
  sleep_ms(5000);

  printf("Finish\n");
  while (1) {
    sleep_ms(1000);
  }
}
