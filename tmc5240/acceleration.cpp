/*
 * Copyright (c) 2025 Indoor Corgi
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "tmc5240_selector.h"

// RPZ-Stepper用サンプルコード
// 加減速を細かく指定する

// モーター制御クラスのインスタンスを作成
// 第1引数: 使用モーターの1回転のフルステップ数. 省略すると200
// 第2引数: 基板のモーター選択. 0ならモーター#1, 1ならモーター#2
TMC5240Selector m1(200, 0);

int main() {
  stdio_init_all();
  sleep_ms(3000);  // シリアルモニターを接続するための待機時間
  printf("\nProgram start\n");
  m1.init();  // GPIO/SPIを初期化

  // 加減速を細かく指定する
  m1.set_v1_rpm(30);    // 1つめの速度域しきい値
  m1.set_a1(200);       // 速度が0〜v1の範囲の加速
  m1.set_d1(200);       // 速度が0〜v1の範囲の減速
  m1.set_v2_rpm(60);    // 2つめの速度域しきい値
  m1.set_a2(1000);      // 速度がv1〜v2の範囲の加速
  m1.set_d2(1000);      // 速度がv1〜v2の範囲の減速
  m1.set_vmax_rpm(80);  // 回転速度
  m1.set_amax(200);     // 速度がv2〜vmaxの範囲の加速
  m1.set_dmax(200);     // 速度がv2〜vmaxの範囲の減速

  // 必ず行う設定
  m1.set_ifs(0.5);  // モーターの定格に合わせて電流値を設定 (例:0.5A)
  m1.enable();      // ドライバーの出力をONにしてモーターに電圧印加

  m1.moveto(256000);  // 200ステップ/回転のモーターで5回転
  m1.moveto(0);       // 元の位置に戻す

  printf("Finish\n");
  while (1) {
    sleep_ms(1000);
  }
}
