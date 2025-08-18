/*
 * Copyright (c) 2025 Indoor Corgi
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "tmc5240_selector.h"

// RPZ-Stepper用サンプルコード
// モーターを指定位置まで回転させる

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
  m1.set_ifs(0.5);      // モーターの定格に合わせて電流値を設定 (例:0.5A)
  m1.set_vmax_rpm(60);  // 回転速度をrpmで指定
  m1.set_amax(500);     // 加速の速さ 500だと1秒で速度v=47684(200ステップ/回転のモーターで41.6rpm)に達する加速
  m1.set_dmax(500);     // 減速の速さ 500だと1秒で速度v=47684(200ステップ/回転のモーターで41.6rpm)から停止する減速
  m1.enable();          // ドライバーの出力をONにしてモーターに電圧印加

  // 目標位置をxtargetに書き込むとモーターが回転しはじめる
  // 200ステップ/回転のモーターなら, 200x256マイクロステップ=51200で1回転
  // +/-2147483647の範囲で指定 マイナスだと反対方向に回転
  // この例では3回転
  m1.set_xtarget(153600);

  // モーター回転中は, xactualで現在位置, vactual_rpmで現在の回転数,
  // position_reachedで目標到達したか確認できる. 1なら到達.
  sleep_ms(2000);
  printf("----------\n");
  printf("Current position:%d\n", m1.get_xactual());
  printf("Current rpm:%f\n", m1.get_vactual_rpm());
  printf("Is position reached:%u\n", m1.get_position_reached());

  sleep_ms(3000);
  printf("----------\n");
  printf("Current position:%d\n", m1.get_xactual());
  printf("Current rpm:%f\n", m1.get_vactual_rpm());
  printf("Is position reached:%u\n", m1.get_position_reached());

  // 目標座標に0を設定して最初の位置に戻す
  printf("Set xtarget 0\n");
  m1.set_xtarget(0);
  sleep_ms(5000);

  // moveto関数を使うと, 目標に到達するまで自動で待機する
  printf("Move to 153600\n");
  m1.moveto(153600);
  printf("Move to 0\n");
  m1.moveto(0);

  m1.disable();  // ドライバーの出力をOFFにしてモーターへの電圧印加停止

  printf("Finish\n");
  while (1) {
    sleep_ms(1000);
  }
}
