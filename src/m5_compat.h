#pragma once

// Shim that bridges the M5StickCPlus-style API to M5Unified so existing
// sources can include this header in place of <M5StickCPlus.h>. M5Unified
// drops `M5.Axp` and `M5.Beep`, so those call sites are routed through
// compat::xxx() helpers below; everything else (LCD, IMU, buttons) keeps
// working with the original API surface.

#include <M5Unified.h>

// ---- Legacy LCD / Sprite type aliases ---------------------------------
// The original code passes both `M5.Lcd` and `TFT_eSprite` as `TFT_eSPI*`.
// M5StickCPlus had `TFT_eSprite : public TFT_eSPI`, but in M5Unified the
// equivalents (M5GFX and M5Canvas) sit in separate hierarchies. Aliasing
// to their common base `LovyanGFX` (= `lgfx::LGFXBase`) lets the same
// function signatures accept either.
using TFT_eSPI = LovyanGFX;
using TFT_eSprite = M5Canvas;

// ---- RTC type aliases -------------------------------------------------
// The original code builds RTC_TimeTypeDef / RTC_DateTypeDef literals
// from the BM8563 driver. M5Unified's m5::rtc_time_t / m5::rtc_date_t
// share the same field order, so a type alias is enough.
using RTC_TimeTypeDef = m5::rtc_time_t;
using RTC_DateTypeDef = m5::rtc_date_t;

namespace compat {

// ---- Power ------------------------------------------------------------
// Old: M5.Axp.ScreenBreath(20..100) drove the AXP192 LDO directly.
// New: M5Unified's M5.Display.setBrightness(0..255).
inline void setScreenBrightness0_100(uint8_t v) {
  if (v > 100) v = 100;
  M5.Display.setBrightness((uint8_t)((uint16_t)v * 255 / 100));
}

// Old: M5.Axp.SetLDO2(bool) toggled the display rail. M5StickS3 has no
// equivalent rail, so emulate "off" by setting brightness to 0. "On"
// is a no-op because the caller restores brightness explicitly via
// applyBrightness() — matching the original flow.
inline void setAuxiliaryRail(bool on) {
  if (!on) M5.Display.setBrightness(0);
}

// Old: M5.Axp.PowerOff(). M5Unified equivalent is M5.Power.powerOff().
inline void powerOff() { M5.Power.powerOff(); }

// Old: M5.Axp.GetBatVoltage() returned volts (float).
// M5.Power.getBatteryVoltage() returns millivolts (int), so convert.
inline float getBatteryVoltageV() {
  return M5.Power.getBatteryVoltage() / 1000.0f;
}

// Old: M5.Axp.GetBatCurrent() returned milliamps (float). M5StickS3 may
// lack a current-sense path, in which case this returns 0 — only used for
// the info screen, so the impact is cosmetic.
inline float getBatteryCurrentMa() {
  return (float)M5.Power.getBatteryCurrent();
}

// Old: M5.Axp.GetVBusVoltage() returned volts; callers compare against
// `> 4.0f` to detect USB power. Map M5.Power.isCharging() to 5.0/0.0.
inline float getVBusVoltageV() {
  return M5.Power.isCharging() ? 5.0f : 0.0f;
}

// Old: M5.Axp.GetTempInAXP192() exposed the AXP192's die temperature.
// M5StickS3 has no equivalent sensor.
inline float getInternalTempC() { return 0.0f; }

// Old: M5.Axp.GetBtnPress() returned 0x02 for a power-button click.
// Map M5.BtnPWR.wasClicked() to the same encoding.
inline uint8_t getPowerBtnPress() {
  return M5.BtnPWR.wasClicked() ? 0x02 : 0x00;
}

// ---- LED --------------------------------------------------------------
// Old: digitalWrite on GPIO10 (active-low) drove the StickC Plus red LED.
// New: M5.Power.setLed(brightness) lets M5Unified pick the right pin per
// board. M5StickS3 has no user-controllable LED, so this is currently a
// no-op there; kept in place so the call site works on future hardware
// where M5Unified adds support.
inline void setLed(bool on) {
  M5.Power.setLed(on ? 64 : 0);
}

// ---- IMU --------------------------------------------------------------
// Same signature as the old M5.Imu.getAccelData(float*, float*, float*).
inline void getAccel(float* ax, float* ay, float* az) {
  M5.Imu.getAccelData(ax, ay, az);
}

// ---- Speaker ----------------------------------------------------------
// Old: M5.Beep.tone(freq, dur) with a paired update() in loop().
// M5Unified's Speaker plays asynchronously, so update() is gone. begin()
// must be called explicitly after M5.begin() or tone() stays silent.
inline void beepBegin() { M5.Speaker.begin(); }
inline void beepTone(uint16_t freq, uint16_t dur) {
  M5.Speaker.tone(freq, dur);
}

}  // namespace compat
