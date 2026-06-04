📘 About the MicroPython Firmware with Infrared Remote Control Support for Pico2W

This project provides a custom MicroPython firmware for the Raspberry Pi Pico2W that enables infrared (IR) transmission and reception — functionality not available in standard MicroPython builds.

Hardware Used 

・RPZ-IR-Sensor Rev2 (Indoor Corgi)

・RPP-HAT-Adapter

Libraries Used This firmware incorporates the following files published by Indoor Corgi:

・infrared.h (used as the original reference; the actual build uses infrared_ext.h)

・infrared.c (same as above; the actual build uses infrared_ext.c)

・infrared.pio.h (generated during build)

All copyrights belong to the original creators.

📁 File Placement

Prepare a MicroPython build environment and create the following directory:

/micropython/ports/rp2/modules/ir

Place all infrared_* files inside this directory.

🔧 Build Instructions

Run the following commands in your terminal to build a firmware image with IR functionality:

cd ~/micropython/ports/rp2

make BOARD=RPI_PICO2_W USER_C_MODULES=modules/ir -j4

Flash the generated firmware.uf2 onto your Pico2W to enable IR transmission and reception from MicroPython.

📡 Usage

Refer to the sample scripts in the sample folder:

・ir_receive.py — IR signal reception

・ir_send.py — IR signal transmission

・ir_learning_remote.py — Learning remote controller

SW1: Receive mode

Blue LED lights on successful reception

SW2: Transmit stored data

Data is saved as JSON

Useful for collecting raw pulse data

・aircon_ir_node

Wi-Fi–controlled air conditioner remote

See the README inside the aircon_ir_node folder for details

🎥 Demo Video (YouTube) This video demonstrates simultaneous IR transmission and Wi‑Fi communication using this custom firmware and sample code. You can see the Pico2W actually controlling an air conditioner.

https://youtu.be/CiHwa2uEGL8?si=j78sp6HAiGvLg5oV

🧩 About the Firmware

This firmware is based on MicroPython v1.29.0-preview.259, with the IR transmission/reception module (infrared.c / infrared.h) integrated for the Raspberry Pi Pico2W (RP2350).

By enabling direct IR control from MicroPython, implementing IR remote functions becomes significantly easier.

📝 Update History 

2026/6/4

Added support for selecting GPIO pin numbers during initialization.

Recommended pins for both TX and RX: GPIO 0–22, 25, 29.

When using the original HAT from the library author, use TX=10, RX=2.

2026/5/29

ir_send now accepts tuples for pulse data.

This improves performance and reduces overhead.
