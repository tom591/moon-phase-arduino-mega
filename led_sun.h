#pragma once
#include <Arduino.h>
#include <avr/pgmspace.h>

enum SunState : uint8_t {
  SUN_NIGHT = 0,
  SUN_SUNRISE = 1,
  SUN_DAY = 2,
  SUN_SUNSET = 3
};

// -------------------- Set your pins for the RGB LED (common cathode) --------------------
static const uint8_t PIN_R = 12;
static const uint8_t PIN_G = 13;
static const uint8_t PIN_B = 11;

// If you find out that the module is "common anode" (which is typically less common),
// just switch this to true and the logic will be inverted.
static const bool COMMON_ANODE = false;

static const uint8_t GLOBAL_BRIGHTNESS = 40; // 0..255

// -------------------- Color (0..255) --------------------
static const uint8_t BR_NIGHT_R = 0; // blue - Night
static const uint8_t BR_NIGHT_G = 0;
static const uint8_t BR_NIGHT_B = 20;   

static const uint8_t BR_SUNRISE_R = 255; // orange - Sunrise
static const uint8_t BR_SUNRISE_G = 70;
static const uint8_t BR_SUNRISE_B = 0;

static const uint8_t BR_DAY_R = 155; // white - Day
static const uint8_t BR_DAY_G = 155;
static const uint8_t BR_DAY_B = 155;      

static const uint8_t BR_SUNSET_R = 155; // red - Sunset
static const uint8_t BR_SUNSET_G = 0;
static const uint8_t BR_SUNSET_B = 0;

static inline uint8_t pwmVal(uint8_t v) {
  return COMMON_ANODE ? (uint8_t)(255 - v) : v;
}

static inline uint8_t scale(uint8_t v) {
  return (uint8_t)((uint16_t)v * GLOBAL_BRIGHTNESS / 255);
}

static inline void setRGB(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(PIN_R, pwmVal(scale(r)));
  analogWrite(PIN_G, pwmVal(scale(g)));
  analogWrite(PIN_B, pwmVal(scale(b)));
}

static inline bool inWrappedRange(int now, int start, int end) {
  auto norm = [](int x) -> int {
    while (x < 0) x += 1440;
    while (x >= 1440) x -= 1440;
    return x;
  };

  now = norm(now);
  start = norm(start);
  end = norm(end);

  if (start <= end) {
    return (now >= start && now <= end);
  } else {
    return (now >= start || now <= end);
  }
}

static inline SunState computeSunState(int nowMin, int sunriseMin, int sunsetMin) {
  const int W = 60;

  const int srStart = sunriseMin - W;
  const int srEnd   = sunriseMin + W;

  const int ssStart = sunsetMin - W;
  const int ssEnd   = sunsetMin + W;

  if (inWrappedRange(nowMin, srStart, srEnd)) return SUN_SUNRISE;
  if (inWrappedRange(nowMin, ssStart, ssEnd)) return SUN_SUNSET;

  if (nowMin > srEnd && nowMin < ssStart) return SUN_DAY;

  return SUN_NIGHT;
}

static inline void applySunState(SunState s) {
  switch (s) {
    case SUN_SUNRISE: setRGB(BR_SUNRISE_R, BR_SUNRISE_G, BR_SUNRISE_B); break;
    case SUN_DAY:     setRGB(BR_DAY_R,     BR_DAY_G,     BR_DAY_B);     break;
    case SUN_SUNSET:  setRGB(BR_SUNSET_R,  BR_SUNSET_G,  BR_SUNSET_B);  break;
    case SUN_NIGHT:
    default:          setRGB(BR_NIGHT_R,   BR_NIGHT_G,   BR_NIGHT_B);   break;
  }
}