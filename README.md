📘 Pico2W 用 赤外線リモコン機能付き MicroPython ファームウェアについて

Raspberry Pi Pico2W で赤外線リモコンを扱うため、
通常の MicroPython では利用できない 赤外線送受信処理をファームウェアに組み込みました。

ハードウェアには以下を使用しています：


・RPZ-IR-Sensor Rev2（Indoor Corgi 様）

・RPP-HAT-Adapter


ライブラリは、Indoor Corgi 様が公開されている以下のファイルを利用しています：

・infrared.h

・infrared.c

・infrared.pio.h（ビルド時に生成される）


これらのファイルの著作権は、すべて製作元様に帰属します。

📁ファイル配置

MicroPython のビルド環境を用意し、以下のディレクトリに ir フォルダを作成します：

/micropython/ports/rp2/modules/ir

その中に、上記の infrared.* ファイル一式を配置してください。

🔧 ビルド方法

ターミナルで以下を実行すると、赤外線機能を組み込んだ firmware.uf2 が生成されます。

cd ~/micropython/ports/rp2

make BOARD=RPI_PICO2_W USER_C_MODULES=modules/ir -j4

生成された firmware.uf2 を Pico2W に書き込むことで、
MicroPython から赤外線送受信が利用可能になります。

📡 使い方

sample フォルダにある以下のサンプルを参考にしてください：

・ir_receive.py（赤外線受信）

・ir_send.py（赤外線送信）

・aircon_ir_node、pulses.py（Wi-Fi 経由で動作するエアコンリモコンです。詳細は aircon_ir_node フォルダ内の README を参照してください）

MicroPython から操作できるため、
赤外線リモコンの扱いが大幅に簡単になります。

本ファームウェアは MicroPython v1.29.0-preview.259 をベースに、
赤外線送受信モジュール（infrared.c / infrared.h）を組み込んだ
Pico2W（RP2350）向けのカスタムビルドです。
