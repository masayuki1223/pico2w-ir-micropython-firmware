/*
 * Copyright (c) 2025 Indoor Corgi
 *
 * SPDX-License-Identifier: MIT
 */

#include "tmc5240.h"

#include <math.h>

// コンストラクタ
//
// Args:
//   steps_per_rev: モーター1回転あたりのフルステップ数
//   spi_csn_pin: CSピン
//   fclk: クロック周波数
//   spi_baud: SPI周波数
//   spi: 使用するSPIインスタンス. 例:spi0
//   spi_sck_pin: SCKピン
//   spi_tx_pin: MOSIピン
//   spi_rx_pin: MISOピン
TMC5240::TMC5240(uint steps_per_rev,
                 uint spi_csn_pin,
                 double fclk,
                 uint spi_baud,
                 spi_inst_t* spi,
                 uint spi_sck_pin,
                 uint spi_tx_pin,
                 uint spi_rx_pin)
    : steps_per_rev(steps_per_rev),
      spi_csn_pin(spi_csn_pin),
      fclk(fclk),
      spi_baud(spi_baud),
      spi(spi),
      spi_sck_pin(spi_sck_pin),
      spi_tx_pin(spi_tx_pin),
      spi_rx_pin(spi_rx_pin) {
}

// SPIを初期化
void TMC5240::init() {
  gpio_init(spi_csn_pin);
  gpio_set_dir(spi_csn_pin, GPIO_OUT);
  gpio_put(spi_csn_pin, 1);
  gpio_init(spi_sck_pin);
  gpio_set_function(spi_sck_pin, GPIO_FUNC_SPI);
  gpio_init(spi_tx_pin);
  gpio_set_function(spi_tx_pin, GPIO_FUNC_SPI);
  gpio_init(spi_rx_pin);
  gpio_set_function(spi_rx_pin, GPIO_FUNC_SPI);
  spi_init(spi, spi_baud);
  spi_set_format(spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
}

// CS制御
//
// Args:
//   enable: trueならCS Low, falseならCS High
void TMC5240::chip_enable(bool enable) {
  gpio_put(spi_csn_pin, !enable);
}

//---------------------------------
// Direct Register Access

// レジスターのデータを読み出す
//
// Args:
//   reg_addr: レジスターアドレス
//
// Returns: データ
uint32_t TMC5240::read_register(uint8_t reg_addr) {
  uint8_t write_data[5] = {reg_addr, 0, 0, 0, 0};
  uint8_t read_data[5] = {};
  chip_enable(true);
  spi_write_blocking(spi, write_data, 5);
  chip_enable(false);
  busy_wait_at_least_cycles(10);
  chip_enable(true);
  spi_write_read_blocking(spi, write_data, read_data, 5);
  chip_enable(false);

  uint32_t data = 0;
  for (int i = 0; i < 4; i++) {
    data += ((uint32_t)(read_data[i + 1]) << ((3 - i) * 8));
  }
  return data;
}

// レジスターにデータを書き込む
//
// Args:
//   reg_addr: レジスターアドレス
//   data: データ
void TMC5240::write_register(uint8_t reg_addr, uint32_t data) {
  uint8_t write_data[5] = {};
  write_data[0] = reg_addr | 0x80;
  for (int i = 0; i < 4; i++) {
    write_data[i + 1] = (data >> ((3 - i) * 8)) & 0xFF;
  }
  chip_enable(true);
  spi_write_blocking(spi, write_data, 5);
  chip_enable(false);
}

// 32bit幅のdataの指定ビットの値を返す
uint32_t TMC5240::get_bits(uint32_t data, uint msb, uint lsb) {
  if (msb < lsb) panic("lsb is larger than msb");
  uint length = msb - lsb + 1;
  data >>= lsb;
  data &= (uint32_t)(pow(2, length) - 1);
  return data;
}

// 32bit幅のdataの指定ビットにvalueをセットした値を返す
uint32_t TMC5240::set_bits(uint32_t data, uint msb, uint lsb, uint32_t value) {
  if (msb < lsb) panic("lsb is larger than msb");
  uint length = msb - lsb + 1;

  // 指定ビットを0にする
  for (uint i = lsb; i <= msb; i++) {
    data &= (uint32_t)(0xFFFFFFFF - (1 << i));
  }
  data ^= (value << lsb);
  return data;
}

// 指定レジスターの指定ビットを返す
uint32_t TMC5240::get_register_bits(uint8_t reg_addr, uint msb, uint lsb) {
  return get_bits(read_register(reg_addr), msb, lsb);
}

// 指定レジスターの指定ビットにvalueをセットして書き込む
// 一度読み出して, レジスターの未指定ビットは変更前と同じ値を書き込む
void TMC5240::set_register_bits(uint8_t reg_addr, uint msb, uint lsb, uint32_t value) {
  uint32_t data = read_register(reg_addr);
  data = set_bits(data, msb, lsb, value);
  write_register(reg_addr, data);
}

//---------------------------------
// Global Configuration Registers

//---------------
// GCONF 0x0

uint32_t TMC5240::get_fast_standstill() {
  return get_register_bits(0x0, 1, 1);
}

void TMC5240::set_fast_standstill(uint32_t value) {
  set_register_bits(0x0, 1, 1, value);
}

uint32_t TMC5240::get_en_pwm_mode() {
  return get_register_bits(0x0, 2, 2);
}

void TMC5240::set_en_pwm_mode(uint32_t value) {
  set_register_bits(0x0, 2, 2, value);
}

uint32_t TMC5240::get_shaft() {
  return get_register_bits(0x0, 4, 4);
}

void TMC5240::set_shaft(uint32_t value) {
  set_register_bits(0x0, 4, 4, value);
}

uint32_t TMC5240::get_stop_enable() {
  return get_register_bits(0x0, 15, 15);
}

void TMC5240::set_stop_enable(uint32_t value) {
  set_register_bits(0x0, 15, 15, value);
}

uint32_t TMC5240::get_direct_mode() {
  return get_register_bits(0x0, 16, 16);
}

void TMC5240::set_direct_mode(uint32_t value) {
  set_register_bits(0x0, 16, 16, value);
}

//---------------
// GSTAT 0x1

uint32_t TMC5240::get_reset() {
  return get_register_bits(0x1, 0, 0);
}

void TMC5240::set_reset(uint32_t value) {
  set_register_bits(0x1, 0, 0, value);
}

uint32_t TMC5240::get_drv_err() {
  return get_register_bits(0x1, 1, 1);
}

void TMC5240::set_drv_err(uint32_t value) {
  set_register_bits(0x1, 1, 1, value);
}

uint32_t TMC5240::get_register_reset() {
  return get_register_bits(0x1, 3, 3);
}

void TMC5240::set_register_reset(uint32_t value) {
  set_register_bits(0x1, 3, 3, value);
}

uint32_t TMC5240::get_vm_uvlo() {
  return get_register_bits(0x1, 4, 4);
}

void TMC5240::set_vm_uvlo(uint32_t value) {
  set_register_bits(0x1, 4, 4, value);
}

//---------------
// IOIN 0x4

uint32_t TMC5240::get_ext_clk() {
  return get_register_bits(0x4, 14, 14);
}

uint32_t TMC5240::get_adc_err() {
  return get_register_bits(0x4, 15, 15);
}

uint32_t TMC5240::get_silicon_rv() {
  return get_register_bits(0x4, 18, 16);
}

uint32_t TMC5240::get_version() {
  return get_register_bits(0x4, 31, 24);
}

//---------------
// DRV_CONF 0xA

uint32_t TMC5240::get_current_range() {
  return get_register_bits(0xA, 1, 0);
}

void TMC5240::set_current_range(uint32_t value) {
  set_register_bits(0xA, 1, 0, value);
}

uint32_t TMC5240::get_slope_control() {
  return get_register_bits(0xA, 5, 4);
}

void TMC5240::set_slope_control(uint32_t value) {
  set_register_bits(0xA, 5, 4, value);
}

//---------------
// GLOBAL_SCALER 0xB

uint32_t TMC5240::get_global_scaler() {
  return get_register_bits(0xB, 7, 0);
}

// value must be >=31 and <=256
void TMC5240::set_global_scaler(uint32_t value) {
  if (value < 31 || value > 256) panic("TMC5240::set_global_scaler value out of range");
  if (value == 256) value = 0;
  set_register_bits(0xB, 7, 0, value);
}

//---------------------------------
// Velocity Dependent Configuration Registers

//---------------
// IHOLD_IRUN 0x10

uint32_t TMC5240::get_ihold() {
  return get_register_bits(0x10, 4, 0);
}

void TMC5240::set_ihold(uint32_t value) {
  set_register_bits(0x10, 4, 0, value);
}

uint32_t TMC5240::get_irun() {
  return get_register_bits(0x10, 12, 8);
}

void TMC5240::set_irun(uint32_t value) {
  set_register_bits(0x10, 12, 8, value);
}
uint32_t TMC5240::get_iholddelay() {
  return get_register_bits(0x10, 19, 16);
}

void TMC5240::set_iholddelay(uint32_t value) {
  set_register_bits(0x10, 19, 16, value);
}

uint32_t TMC5240::get_irundelay() {
  return get_register_bits(0x10, 27, 24);
}

void TMC5240::set_irundelay(uint32_t value) {
  set_register_bits(0x10, 27, 24, value);
}

//---------------
// TPOWERDOWN 0x11

uint32_t TMC5240::get_tpowerdown() {
  return get_register_bits(0x11, 7, 0);
}

void TMC5240::set_tpowerdown(uint32_t value) {
  set_register_bits(0x11, 7, 0, value);
}

//---------------
// TSTEP 0x12

uint32_t TMC5240::get_tstep() {
  return read_register(0x12);
}

//---------------
// TPWMTHRS 0x13

uint32_t TMC5240::get_tpwmthrs() {
  return read_register(0x13);
}

void TMC5240::set_tpwmthrs(uint32_t value) {
  write_register(0x13, value);
}

//---------------
// TCOOLTHRS 0x14

uint32_t TMC5240::get_tcoolthrs() {
  return read_register(0x14);
}

void TMC5240::set_tcoolthrs(uint32_t value) {
  write_register(0x14, value);
}

//---------------
// THIGH 0x15

uint32_t TMC5240::get_thigh() {
  return read_register(0x15);
}

void TMC5240::set_thigh(uint32_t value) {
  write_register(0x15, value);
}

//---------------------------------
// Ramp Generator Registers

//---------------
// RAMPMODE 0x20

TMC5240::rampmode_type_t TMC5240::get_rampmode() {
  return (rampmode_type_t)get_register_bits(0x20, 1, 0);
}

void TMC5240::set_rampmode(rampmode_type_t value) {
  set_register_bits(0x20, 1, 0, value);
}

//---------------
// XACTUAL 0x21

uint32_t TMC5240::get_xactual() {
  return read_register(0x21);
}

void TMC5240::set_xactual(uint32_t value) {
  write_register(0x21, value);
}

//---------------
// VACTUAL 0x22

int32_t TMC5240::get_vactual() {
  int32_t vactual = read_register(0x22);
  if (vactual >= 8388608) {
    vactual -= 16777216;
  }
  return vactual;
}

//---------------
// VSTART 0x23

uint32_t TMC5240::get_vstart() {
  return read_register(0x23);
}

void TMC5240::set_vstart(uint32_t value) {
  write_register(0x23, value);
}

//---------------
// A1 0x24

uint32_t TMC5240::get_a1() {
  return read_register(0x24);
}

void TMC5240::set_a1(uint32_t value) {
  write_register(0x24, value);
}

//---------------
// V1 0x25

uint32_t TMC5240::get_v1() {
  return read_register(0x25);
}

void TMC5240::set_v1(uint32_t value) {
  write_register(0x25, value);
}

//---------------
// AMAX 0x26

uint32_t TMC5240::get_amax() {
  return read_register(0x26);
}

void TMC5240::set_amax(uint32_t value) {
  write_register(0x26, value);
}

//---------------
// VMAX 0x27

uint32_t TMC5240::get_vmax() {
  return read_register(0x27);
}

void TMC5240::set_vmax(uint32_t value) {
  write_register(0x27, value);
}

//---------------
// DMAX 0x28

uint32_t TMC5240::get_dmax() {
  return read_register(0x28);
}

void TMC5240::set_dmax(uint32_t value) {
  write_register(0x28, value);
}

//---------------
// TVMAX 0x29

uint32_t TMC5240::get_tvmax() {
  return read_register(0x29);
}

void TMC5240::set_tvmax(uint32_t value) {
  write_register(0x29, value);
}

//---------------
// D1 0x2A

uint32_t TMC5240::get_d1() {
  return read_register(0x2A);
}

void TMC5240::set_d1(uint32_t value) {
  write_register(0x2A, value);
}

//---------------
// VSTOP 0x2B

uint32_t TMC5240::get_vstop() {
  return read_register(0x2B);
}

void TMC5240::set_vstop(uint32_t value) {
  write_register(0x2B, value);
}

//---------------
// TZEROWAIT 0x2C

uint32_t TMC5240::get_tzerowait() {
  return read_register(0x2C);
}

void TMC5240::set_tzerowait(uint32_t value) {
  write_register(0x2C, value);
}

//---------------
// XTARGET 0x2D

uint32_t TMC5240::get_xtarget() {
  return read_register(0x2D);
}

void TMC5240::set_xtarget(uint32_t value) {
  write_register(0x2D, value);
}

//---------------
// V2 0x2E

uint32_t TMC5240::get_v2() {
  return read_register(0x2E);
}

void TMC5240::set_v2(uint32_t value) {
  write_register(0x2E, value);
}

//---------------
// A2 0x2F

uint32_t TMC5240::get_a2() {
  return read_register(0x2F);
}

void TMC5240::set_a2(uint32_t value) {
  write_register(0x2F, value);
}

//---------------
// D2 0x30

uint32_t TMC5240::get_d2() {
  return read_register(0x30);
}

void TMC5240::set_d2(uint32_t value) {
  write_register(0x30, value);
}

//---------------------------------
// Ramp Generator Driver Feature Control Registers

//---------------
// VDCMIN 0x33

uint32_t TMC5240::get_vdcmin() {
  return get_register_bits(0x33, 22, 8);
}

void TMC5240::set_vdcmin(uint32_t value) {
  set_register_bits(0x33, 22, 8, value);
}

//---------------
// SW_MODE 0x34

uint32_t TMC5240::get_sg_stop() {
  return get_register_bits(0x34, 10, 10);
}

void TMC5240::set_sg_stop(uint32_t value) {
  set_register_bits(0x34, 10, 10, value);
}

uint32_t TMC5240::get_en_softstop() {
  return get_register_bits(0x34, 11, 11);
}

void TMC5240::set_en_softstop(uint32_t value) {
  set_register_bits(0x34, 11, 11, value);
}

uint32_t TMC5240::get_en_virtual_stop_l() {
  return get_register_bits(0x34, 12, 12);
}

void TMC5240::set_en_virtual_stop_l(uint32_t value) {
  set_register_bits(0x34, 12, 12, value);
}

uint32_t TMC5240::get_en_virtual_stop_r() {
  return get_register_bits(0x34, 13, 13);
}

void TMC5240::set_en_virtual_stop_r(uint32_t value) {
  set_register_bits(0x34, 13, 13, value);
}

//---------------
// RAMP_STAT 0x35

uint32_t TMC5240::get_velocity_reached() {
  return get_register_bits(0x35, 8, 8);
}

uint32_t TMC5240::get_position_reached() {
  return get_register_bits(0x35, 9, 9);
}

uint32_t TMC5240::get_vzero() {
  return get_register_bits(0x35, 10, 10);
}

//---------------
// VIRTUAL_STOP_L 0x3E

uint32_t TMC5240::get_virtual_stop_l() {
  return read_register(0x3E);
}

void TMC5240::set_virtual_stop_l(uint32_t value) {
  write_register(0x3E, value);
}

//---------------
// VIRTUAL_STOP_R 0x3F

uint32_t TMC5240::get_virtual_stop_r() {
  return read_register(0x3F);
}

void TMC5240::set_virtual_stop_r(uint32_t value) {
  write_register(0x3F, value);
}

//---------------------------------
// ADC Registers

//---------------
// ADC_VSUPPLY_AIN 0x50

// AINの電圧[mV]を返す
double TMC5240::get_adc_ain() {
  int32_t data = get_register_bits(0x50, 28, 16);
  if (data >= 4096) {
    data -= 8192;
  }
  return (double)data * 0.3502;
}

// 電源電圧[V]を返す
double TMC5240::get_adc_vsupply() {
  int32_t data = get_register_bits(0x50, 12, 0);
  return (double)data * 9.732 / 1000.0;
}

//---------------
// ADC_TEMP 0x51

// 温度[℃]を返す
double TMC5240::get_adc_temp() {
  int32_t data = get_register_bits(0x51, 12, 0);
  return (double)(data - 2038) / 7.7;
}

//---------------------------------
// Motor Driver Registers

//---------------
// CHOPCONF 0x6C

uint32_t TMC5240::get_toff() {
  return get_register_bits(0x6C, 3, 0);
}

void TMC5240::set_toff(uint32_t value) {
  set_register_bits(0x6C, 3, 0, value);
}

uint32_t TMC5240::get_disfdcc() {
  return get_register_bits(0x6C, 12, 12);
}

void TMC5240::set_disfdcc(uint32_t value) {
  set_register_bits(0x6C, 12, 12, value);
}

uint32_t TMC5240::get_chm() {
  return get_register_bits(0x6C, 14, 14);
}

void TMC5240::set_chm(uint32_t value) {
  set_register_bits(0x6C, 14, 14, value);
}

uint32_t TMC5240::get_vhighfs() {
  return get_register_bits(0x6C, 18, 18);
}

void TMC5240::set_vhighfs(uint32_t value) {
  set_register_bits(0x6C, 18, 18, value);
}

uint32_t TMC5240::get_vhighchm() {
  return get_register_bits(0x6C, 19, 19);
}

void TMC5240::set_vhighchm(uint32_t value) {
  set_register_bits(0x6C, 19, 19, value);
}

uint32_t TMC5240::get_mres() {
  return get_register_bits(0x6C, 27, 24);
}

void TMC5240::set_mres(uint32_t value) {
  set_register_bits(0x6C, 27, 24, value);
}

uint32_t TMC5240::get_intpol() {
  return get_register_bits(0x6C, 28, 28);
}

void TMC5240::set_intpol(uint32_t value) {
  set_register_bits(0x6C, 28, 28, value);
}

uint32_t TMC5240::get_diss2g() {
  return get_register_bits(0x6C, 30, 30);
}

void TMC5240::set_diss2g(uint32_t value) {
  set_register_bits(0x6C, 30, 30, value);
}

uint32_t TMC5240::get_diss2vs() {
  return get_register_bits(0x6C, 31, 31);
}

void TMC5240::set_diss2vs(uint32_t value) {
  set_register_bits(0x6C, 31, 31, value);
}

//---------------
// COOLCONF 0x6D

uint32_t TMC5240::get_semin() {
  return get_register_bits(0x6D, 3, 0);
}

void TMC5240::set_semin(uint32_t value) {
  set_register_bits(0x6D, 3, 0, value);
}

uint32_t TMC5240::get_seup() {
  return get_register_bits(0x6D, 6, 5);
}

void TMC5240::set_seup(uint32_t value) {
  set_register_bits(0x6D, 6, 5, value);
}

uint32_t TMC5240::get_semax() {
  return get_register_bits(0x6D, 11, 8);
}

void TMC5240::set_semax(uint32_t value) {
  set_register_bits(0x6D, 11, 8, value);
}

uint32_t TMC5240::get_sedn() {
  return get_register_bits(0x6D, 14, 13);
}

void TMC5240::set_sedn(uint32_t value) {
  set_register_bits(0x6D, 14, 13, value);
}

uint32_t TMC5240::get_seimin() {
  return get_register_bits(0x6D, 15, 15);
}

void TMC5240::set_seimin(uint32_t value) {
  set_register_bits(0x6D, 15, 15, value);
}

uint32_t TMC5240::get_sgt() {
  return get_register_bits(0x6D, 22, 16);
}

void TMC5240::set_sgt(uint32_t value) {
  set_register_bits(0x6D, 22, 16, value);
}

uint32_t TMC5240::get_sfilt() {
  return get_register_bits(0x6D, 24, 24);
}

void TMC5240::set_sfilt(uint32_t value) {
  set_register_bits(0x6D, 24, 24, value);
}

//---------------
// DCCTRL 0x6E

uint32_t TMC5240::get_dc_time() {
  return get_register_bits(0x6E, 9, 0);
}

void TMC5240::set_dc_time(uint32_t value) {
  set_register_bits(0x6E, 9, 0, value);
}

uint32_t TMC5240::get_dc_sg() {
  return get_register_bits(0x6E, 23, 16);
}

void TMC5240::set_dc_sg(uint32_t value) {
  set_register_bits(0x6E, 23, 16, value);
}

//---------------
// DRV_STATUS 0x6F

uint32_t TMC5240::get_sg_result() {
  return get_register_bits(0x6F, 9, 0);
}

uint32_t TMC5240::get_s2vsa() {
  return get_register_bits(0x6F, 12, 12);
}

uint32_t TMC5240::get_s2vsb() {
  return get_register_bits(0x6F, 13, 13);
}

uint32_t TMC5240::get_stealth() {
  return get_register_bits(0x6F, 14, 14);
}

uint32_t TMC5240::get_fsactive() {
  return get_register_bits(0x6F, 15, 15);
}

uint32_t TMC5240::get_cs_actual() {
  return get_register_bits(0x6F, 20, 16);
}

uint32_t TMC5240::get_stallguard() {
  return get_register_bits(0x6F, 24, 24);
}

uint32_t TMC5240::get_ot() {
  return get_register_bits(0x6F, 25, 25);
}

uint32_t TMC5240::get_otpw() {
  return get_register_bits(0x6F, 26, 26);
}

uint32_t TMC5240::get_s2ga() {
  return get_register_bits(0x6F, 27, 27);
}

uint32_t TMC5240::get_s2gb() {
  return get_register_bits(0x6F, 28, 28);
}

uint32_t TMC5240::get_ola() {
  return get_register_bits(0x6F, 29, 29);
}

uint32_t TMC5240::get_olb() {
  return get_register_bits(0x6F, 30, 30);
}

uint32_t TMC5240::get_stst() {
  return get_register_bits(0x6F, 31, 31);
}

//---------------
// PWMCONF 0x70

uint32_t TMC5240::get_pwm_grad() {
  return get_register_bits(0x70, 7, 0);
}

void TMC5240::set_pwm_grad(uint32_t value) {
  set_register_bits(0x70, 7, 0, value);
}

uint32_t TMC5240::get_pwm_freq() {
  return get_register_bits(0x70, 17, 16);
}

void TMC5240::set_pwm_freq(uint32_t value) {
  set_register_bits(0x70, 17, 16, value);
}

uint32_t TMC5240::get_pwm_autoscale() {
  return get_register_bits(0x70, 18, 18);
}

void TMC5240::set_pwm_autoscale(uint32_t value) {
  set_register_bits(0x70, 18, 18, value);
}

uint32_t TMC5240::get_pwm_autograd() {
  return get_register_bits(0x70, 19, 19);
}

void TMC5240::set_pwm_autograd(uint32_t value) {
  set_register_bits(0x70, 19, 19, value);
}

uint32_t TMC5240::get_freewheel() {
  return get_register_bits(0x70, 21, 20);
}

void TMC5240::set_freewheel(uint32_t value) {
  set_register_bits(0x70, 21, 20, value);
}

uint32_t TMC5240::get_pwm_meas_sd_enable() {
  return get_register_bits(0x70, 22, 22);
}

void TMC5240::set_pwm_meas_sd_enable(uint32_t value) {
  set_register_bits(0x70, 22, 22, value);
}

uint32_t TMC5240::get_pwm_dis_reg_stst() {
  return get_register_bits(0x70, 23, 23);
}

void TMC5240::set_pwm_dis_reg_stst(uint32_t value) {
  set_register_bits(0x70, 23, 23, value);
}

uint32_t TMC5240::get_pwm_reg() {
  return get_register_bits(0x70, 27, 24);
}

void TMC5240::set_pwm_reg(uint32_t value) {
  set_register_bits(0x70, 27, 24, value);
}

uint32_t TMC5240::get_pwm_lim() {
  return get_register_bits(0x70, 31, 28);
}

void TMC5240::set_pwm_lim(uint32_t value) {
  set_register_bits(0x70, 31, 28, value);
}

//---------------
// PWM_SCALE 0x71

uint32_t TMC5240::get_pwm_scale_sum() {
  return get_register_bits(0x71, 9, 0);
}

uint32_t TMC5240::get_pwm_scale_auto() {
  return get_register_bits(0x71, 24, 16);
}

//---------------
// PWM_AUTO 0x72

uint32_t TMC5240::get_pwm_ofs_auto() {
  return get_register_bits(0x72, 7, 0);
}

void TMC5240::set_pwm_ofs_auto(uint32_t value) {
  set_register_bits(0x72, 7, 0, value);
}

uint32_t TMC5240::get_pwm_grad_auto() {
  return get_register_bits(0x72, 23, 16);
}

void TMC5240::set_pwm_grad_auto(uint32_t value) {
  set_register_bits(0x72, 23, 16, value);
}

//---------------
// SG4_THRS 0x74

uint32_t TMC5240::get_sg4_thrs() {
  return get_register_bits(0x74, 7, 0);
}

void TMC5240::set_sg4_thrs(uint32_t value) {
  set_register_bits(0x74, 7, 0, value);
}

uint32_t TMC5240::get_sg4_filt_en() {
  return get_register_bits(0x74, 8, 8);
}

void TMC5240::set_sg4_filt_en(uint32_t value) {
  set_register_bits(0x74, 8, 8, value);
}

//---------------
// SG4_RESULT 0x75

uint32_t TMC5240::get_sg4_result() {
  return get_register_bits(0x75, 9, 0);
}

//---------------
// SG4_IND 0x76

uint32_t TMC5240::get_sg4_ind_0() {
  return get_register_bits(0x76, 7, 0);
}

uint32_t TMC5240::get_sg4_ind_1() {
  return get_register_bits(0x76, 15, 8);
}

uint32_t TMC5240::get_sg4_ind_2() {
  return get_register_bits(0x76, 23, 16);
}

uint32_t TMC5240::get_sg4_ind_3() {
  return get_register_bits(0x76, 31, 24);
}

//---------------------------------
// Converted Parameters

// current_range, global_scalerから電流の最大値[A]を計算して返す
double TMC5240::get_ifs() {
  uint32_t global_scaler = get_global_scaler();
  if (global_scaler == 0) {
    global_scaler = 256;
  }
  return (double)(get_current_range() + 1) * (double)global_scaler / 256.0;
}

// 電流の最大値[A]からcurrent_range, global_scalerを計算して設定
//
// Args:
//   value: 電流の最大値(IFS) 0.125 - 3.0
void TMC5240::set_ifs(double value) {
  if (value < 0.125 || value > 3.0) panic("TMC5240::set_ifs: value out of range");
  uint32_t current_range = 2;
  if (value <= 1.0)
    current_range = 0;
  else if (value <= 2.0)
    current_range = 1;
  set_current_range(current_range);

  set_global_scaler((uint32_t)round(value * 256.0 / (current_range + 1)));
}

// 回転数パラメータをrpmでget/set

double TMC5240::get_vactual_rpm() {
  return v2rpm(get_vactual());
}

double TMC5240::get_vmax_rpm() {
  return v2rpm(get_vmax());
}

void TMC5240::set_vmax_rpm(double value) {
  set_vmax(rpm2v(value));
}

double TMC5240::get_v1_rpm() {
  return v2rpm(get_v1());
}

void TMC5240::set_v1_rpm(double value) {
  set_v1(rpm2v(value));
}

double TMC5240::get_v2_rpm() {
  return v2rpm(get_v2());
}

void TMC5240::set_v2_rpm(double value) {
  set_v2(rpm2v(value));
}

// ステップ数パラメータをrpmでget/set

double TMC5240::get_tpwmthrs_rpm() {
  uint32_t tpwmthrs = get_tpwmthrs();
  if (tpwmthrs == 0) return 0;
  return tstep2rpm(tpwmthrs);
}

void TMC5240::set_tpwmthrs_rpm(double value) {
  if (value == 0) {
    set_tpwmthrs(0);
  } else {
    set_tpwmthrs(rpm2tstep(value));
  }
}

double TMC5240::get_thigh_rpm() {
  uint32_t thigh = get_thigh();
  if (thigh == 0) return 0;
  return tstep2rpm(thigh);
}

void TMC5240::set_thigh_rpm(double value) {
  if (value == 0) {
    set_thigh(0);
  } else {
    set_thigh(rpm2tstep(value));
  }
}

//---------------------------------
// Functions

// インバーターをONしてモーターに電圧印加
// デフォルトのTOFFの値を使用する
void TMC5240::enable() {
  set_toff(3);
}

// インバーターをOFFしてモーターに電圧を印加しない
void TMC5240::disable() {
  set_toff(0);
}

// xtargetまで移動し, 目標位置に到達したら戻る
//
// Args:
//   xtarget: 目標位置
//   polling_interval: 到達したか確認する頻度[ms]
void TMC5240::moveto(uint32_t xtarget, uint32_t polling_interval) {
  if (get_vmax() < 10) {
    panic("moveto: vmax is too low");
  }
  if (get_amax() == 0) {
    panic("moveto: amax is too low");
  }
  if (get_dmax() == 0) {
    panic("moveto: dmax is too low");
  }
  set_rampmode(RAMPMODE_POSITIONING);
  set_xtarget(xtarget);
  while (get_position_reached() == 0) {
    sleep_ms(polling_interval);
  }
}

// rpmからTMC5240で指定する速度vを計算して返す
uint32_t TMC5240::rpm2v(double rpm) {
  return round(rpm / 60 * steps_per_rev * 256 / fclk * (1 << 24));
}

// TMC5240で指定する速度vからrpmを計算して返す
double TMC5240::v2rpm(int32_t v) {
  return ((double)v) * 60 / steps_per_rev / 256 * fclk / (1 << 24);
}

// rpmからTMC5240で指定するステップ時間tstepを計算して返す
uint32_t TMC5240::rpm2tstep(double rpm) {
  return round(fclk / (rpm / 60 * steps_per_rev * 256));
}

// TMC5240で指定するステップ時間tstepからrpmを計算して返す
double TMC5240::tstep2rpm(uint32_t tstep) {
  return fclk / ((double)tstep / 60 * steps_per_rev * 256);
}
