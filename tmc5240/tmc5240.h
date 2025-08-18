/*
 * Copyright (c) 2025 Indoor Corgi
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef TMC5240_H
#define TMC5240_H

#include "hardware/spi.h"
#include "pico/stdlib.h"

// TMC5240ステッピングモータードライバー制御クラス
class TMC5240 {
 public:
  // RAMPMODE指定用
  typedef enum {
    RAMPMODE_POSITIONING = 0,
    RAMPMODE_VELOCITY_POSITIVE = 1,
    RAMPMODE_VELOCITY_NEGATIVE = 2,
    RAMPMODE_HOLD = 3,
  } rampmode_type_t;

  // クロック周波数
  static constexpr double DEFAULT_FCLK = 12500000.0;
  double fclk;

  // SPI
  static constexpr uint DEFAULT_SPI_BAUD = 1000000;
  uint spi_baud;
  uint spi_csn_pin;
  spi_inst_t* spi;
  uint spi_sck_pin;
  uint spi_tx_pin;
  uint spi_rx_pin;

  // モーター1回転あたりのフルステップ数
  static constexpr uint DEFAULT_STEPS_PER_REV = 200;
  uint steps_per_rev;

  TMC5240(uint steps_per_rev = DEFAULT_STEPS_PER_REV,
          uint spi_csn_pin = PICO_DEFAULT_SPI_CSN_PIN,
          double fclk = DEFAULT_FCLK,
          uint spi_baud = DEFAULT_SPI_BAUD,
          spi_inst_t* spi = spi_default,
          uint spi_sck_pin = PICO_DEFAULT_SPI_SCK_PIN,
          uint spi_tx_pin = PICO_DEFAULT_SPI_TX_PIN,
          uint spi_rx_pin = PICO_DEFAULT_SPI_RX_PIN);

  void init();
  void chip_enable(bool enable);

  //---------------------------------
  // Direct Register Access

  virtual uint32_t read_register(uint8_t reg_addr);
  virtual void write_register(uint8_t reg_addr, uint32_t data);
  uint32_t get_bits(uint32_t data, uint msb, uint lsb);
  uint32_t set_bits(uint32_t data, uint msb, uint lsb, uint32_t value);
  uint32_t get_register_bits(uint8_t reg_addr, uint msb, uint lsb);
  void set_register_bits(uint8_t reg_addr, uint msb, uint lsb, uint32_t value);

  //---------------------------------
  // Global Configuration Registers

  // GCONF 0x0
  uint32_t get_fast_standstill();
  void set_fast_standstill(uint32_t value);
  uint32_t get_en_pwm_mode();
  void set_en_pwm_mode(uint32_t value);
  uint32_t get_shaft();
  void set_shaft(uint32_t value);
  uint32_t get_stop_enable();
  void set_stop_enable(uint32_t value);
  uint32_t get_direct_mode();
  void set_direct_mode(uint32_t value);

  // GSTAT 0x1
  uint32_t get_reset();
  void set_reset(uint32_t value);
  uint32_t get_drv_err();
  void set_drv_err(uint32_t value);
  uint32_t get_register_reset();
  void set_register_reset(uint32_t value);
  uint32_t get_vm_uvlo();
  void set_vm_uvlo(uint32_t value);

  // IOIN 0x4
  uint32_t get_ext_clk();
  uint32_t get_adc_err();
  uint32_t get_silicon_rv();
  uint32_t get_version();

  // DRV_CONF 0xA
  uint32_t get_current_range();
  void set_current_range(uint32_t value);
  uint32_t get_slope_control();
  void set_slope_control(uint32_t value);

  // GLOBAL_SCALER 0xB
  uint32_t get_global_scaler();
  void set_global_scaler(uint32_t value);

  //---------------------------------
  // Velocity Dependent Configuration Registers

  // IHOLD_IRUN 0x10
  uint32_t get_ihold();
  void set_ihold(uint32_t value);
  uint32_t get_irun();
  void set_irun(uint32_t value);
  uint32_t get_iholddelay();
  void set_iholddelay(uint32_t value);
  uint32_t get_irundelay();
  void set_irundelay(uint32_t value);

  // TPOWERDOWN 0x11
  uint32_t get_tpowerdown();
  void set_tpowerdown(uint32_t value);

  // TSTEP 0x12
  uint32_t get_tstep();

  // TPWMTHRS 0x13
  uint32_t get_tpwmthrs();
  void set_tpwmthrs(uint32_t value);

  // TCOOLTHRS 0x14
  uint32_t get_tcoolthrs();
  void set_tcoolthrs(uint32_t value);

  // THIGH 0x15
  uint32_t get_thigh();
  void set_thigh(uint32_t value);

  // RAMPMODE 0x20
  rampmode_type_t get_rampmode();
  void set_rampmode(rampmode_type_t value);

  // XACTUAL 0x21
  uint32_t get_xactual();
  void set_xactual(uint32_t value);

  // VACTUAL 0x22
  int32_t get_vactual();

  // VSTART 0x23
  uint32_t get_vstart();
  void set_vstart(uint32_t value);

  // A1 0x24
  uint32_t get_a1();
  void set_a1(uint32_t value);

  // V1 0x25
  uint32_t get_v1();
  void set_v1(uint32_t value);

  // AMAX 0x26
  uint32_t get_amax();
  void set_amax(uint32_t value);

  // VMAX 0x27
  uint32_t get_vmax();
  void set_vmax(uint32_t value);

  // DMAX 0x28
  uint32_t get_dmax();
  void set_dmax(uint32_t value);

  // TVMAX 0x29
  uint32_t get_tvmax();
  void set_tvmax(uint32_t value);

  // D1 0x2A
  uint32_t get_d1();
  void set_d1(uint32_t value);

  // VSTOP 0x2B
  uint32_t get_vstop();
  void set_vstop(uint32_t value);

  // TZEROWAIT 0x2C
  uint32_t get_tzerowait();
  void set_tzerowait(uint32_t value);

  // XTARGET 0x2D
  uint32_t get_xtarget();
  void set_xtarget(uint32_t value);

  // V2 0x2E
  uint32_t get_v2();
  void set_v2(uint32_t value);

  // A2 0x2F
  uint32_t get_a2();
  void set_a2(uint32_t value);

  // D2 0x30
  uint32_t get_d2();
  void set_d2(uint32_t value);

  //---------------------------------
  // Ramp Generator Driver Feature Control Registers

  // VDCMIN 0x33
  uint32_t get_vdcmin();
  void set_vdcmin(uint32_t value);

  // SW_MODE 0x34
  uint32_t get_sg_stop();
  void set_sg_stop(uint32_t value);
  uint32_t get_en_softstop();
  void set_en_softstop(uint32_t value);
  uint32_t get_en_virtual_stop_l();
  void set_en_virtual_stop_l(uint32_t value);
  uint32_t get_en_virtual_stop_r();
  void set_en_virtual_stop_r(uint32_t value);

  // RAMP_STAT 0x35
  uint32_t get_velocity_reached();
  uint32_t get_position_reached();
  uint32_t get_vzero();

  // VIRTUAL_STOP_L 0x3E
  uint32_t get_virtual_stop_l();
  void set_virtual_stop_l(uint32_t value);

  // VIRTUAL_STOP_R 0x3F
  uint32_t get_virtual_stop_r();
  void set_virtual_stop_r(uint32_t value);

  //---------------------------------
  // ADC Registers

  // ADC_VSUPPLY_AIN 0x50
  double get_adc_ain();
  double get_adc_vsupply();

  // ADC_TEMP 0x51
  double get_adc_temp();

  //---------------------------------
  // Motor Driver Registers

  // CHOPCONF 0x6C
  uint32_t get_toff();
  void set_toff(uint32_t value);
  uint32_t get_disfdcc();
  void set_disfdcc(uint32_t value);
  uint32_t get_chm();
  void set_chm(uint32_t value);
  uint32_t get_vhighfs();
  void set_vhighfs(uint32_t value);
  uint32_t get_vhighchm();
  void set_vhighchm(uint32_t value);
  uint32_t get_mres();
  void set_mres(uint32_t value);
  uint32_t get_intpol();
  void set_intpol(uint32_t value);
  uint32_t get_diss2g();
  void set_diss2g(uint32_t value);
  uint32_t get_diss2vs();
  void set_diss2vs(uint32_t value);

  // COOLCONF 0x6D
  uint32_t get_semin();
  void set_semin(uint32_t value);
  uint32_t get_seup();
  void set_seup(uint32_t value);
  uint32_t get_semax();
  void set_semax(uint32_t value);
  uint32_t get_sedn();
  void set_sedn(uint32_t value);
  uint32_t get_seimin();
  void set_seimin(uint32_t value);
  uint32_t get_sgt();
  void set_sgt(uint32_t value);
  uint32_t get_sfilt();
  void set_sfilt(uint32_t value);

  // DCCTRL 0x6E
  uint32_t get_dc_time();
  void set_dc_time(uint32_t value);
  uint32_t get_dc_sg();
  void set_dc_sg(uint32_t value);

  // DRV_STATUS 0x6F
  uint32_t get_sg_result();
  uint32_t get_s2vsa();
  uint32_t get_s2vsb();
  uint32_t get_stealth();
  uint32_t get_fsactive();
  uint32_t get_cs_actual();
  uint32_t get_stallguard();
  uint32_t get_ot();
  uint32_t get_otpw();
  uint32_t get_s2ga();
  uint32_t get_s2gb();
  uint32_t get_ola();
  uint32_t get_olb();
  uint32_t get_stst();

  // PWMCONF 0x70
  uint32_t get_pwm_grad();
  void set_pwm_grad(uint32_t value);
  uint32_t get_pwm_freq();
  void set_pwm_freq(uint32_t value);
  uint32_t get_pwm_autoscale();
  void set_pwm_autoscale(uint32_t value);
  uint32_t get_pwm_autograd();
  void set_pwm_autograd(uint32_t value);
  uint32_t get_freewheel();
  void set_freewheel(uint32_t value);
  uint32_t get_pwm_meas_sd_enable();
  void set_pwm_meas_sd_enable(uint32_t value);
  uint32_t get_pwm_dis_reg_stst();
  void set_pwm_dis_reg_stst(uint32_t value);
  uint32_t get_pwm_reg();
  void set_pwm_reg(uint32_t value);
  uint32_t get_pwm_lim();
  void set_pwm_lim(uint32_t value);

  // PWM_SCALE 0x71
  uint32_t get_pwm_scale_sum();
  uint32_t get_pwm_scale_auto();

  // PWM_AUTO 0x72
  uint32_t get_pwm_ofs_auto();
  void set_pwm_ofs_auto(uint32_t value);
  uint32_t get_pwm_grad_auto();
  void set_pwm_grad_auto(uint32_t value);

  // SG4_THRS 0x74
  uint32_t get_sg4_thrs();
  void set_sg4_thrs(uint32_t value);
  uint32_t get_sg4_filt_en();
  void set_sg4_filt_en(uint32_t value);

  // SG4_RESULT 0x75
  uint32_t get_sg4_result();

  // SG4_IND 0x76
  uint32_t get_sg4_ind_0();
  uint32_t get_sg4_ind_1();
  uint32_t get_sg4_ind_2();
  uint32_t get_sg4_ind_3();

  //---------------------------------
  // Converted Parameters

  double get_ifs();
  void set_ifs(double value);
  double get_vactual_rpm();
  double get_vmax_rpm();
  void set_vmax_rpm(double value);
  double get_v1_rpm();
  void set_v1_rpm(double value);
  double get_v2_rpm();
  void set_v2_rpm(double value);
  double get_tpwmthrs_rpm();
  void set_tpwmthrs_rpm(double value);
  double get_thigh_rpm();
  void set_thigh_rpm(double value);

  //---------------------------------
  // Functions

  void enable();
  void disable();
  void moveto(uint32_t xtarget, uint32_t polling_interval=100);
  uint32_t rpm2v(double rpm);
  double v2rpm(int32_t v);
  uint32_t rpm2tstep(double rpm);
  double tstep2rpm(uint32_t tstep);
};

#endif
