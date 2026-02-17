#include <SPI.h>
#include <TFT_22_ILI9225.h>
#include <avr/pgmspace.h>
#include <Wire.h>
#include <RTClib.h>
#include <math.h>

#include "sunrise.h"
#include "led_sun.h"

// ================== TFT pins ==================
#define TFT_RST 8
#define TFT_RS  9
#define TFT_CS  10
#define TFT_LED 0

TFT_22_ILI9225 tft(TFT_RST, TFT_RS, TFT_CS, TFT_LED);

// ================== RTC ==================
RTC_DS3231 rtc;

// ================== Bitmap dimensions ==================
#define IMG_W      176
#define IMG_H_SRC  200

// ================== Bottom panel (3 rows) ==================
#define INFO_H     28
#define IMG_H_DRAW (220 - INFO_H)   // 176x220 => 192 px for picture

#define ENABLE_HOURLY_REFRESH 0

// ===== Moon bitmaps (.h) =====
#include "phases.h"

static int lastY = -1;
static int lastM = -1;
static int lastD = -1;

// ================== SUN LED cache ==================
static SunState g_lastSunState = (SunState)255;
static int g_lastQuarter = -1;

// -------------------- Phase names (without diacritics) --------------------
static const char* PHASE_EN[MOON_PHASE_COUNT] = {
  "NEW MOON",
  "WAXING CRESCENT",
  "FIRST QUARTER",
  "WAXING GIBBOUS",
  "FULL MOON",
  "WANING GIBBOUS",
  "LAST QUARTER",
  "WANING CRESCENT"
};

// -------------------- Special days --------------------
struct SpecialDay {
  uint8_t day;
  uint8_t month;
  const char* text;
};

static const SpecialDay SPECIAL_DAYS[] = {
  {22, 2, "MOMS BIRTHDAY :)"},
  {14, 6, "TOMS BIRTHDAY :)"}
};

#define SPECIAL_DAY_COUNT (sizeof(SPECIAL_DAYS) / sizeof(SPECIAL_DAYS[0]))

// ================== CACHE ==================
static uint8_t g_phaseIdx = 0;
static bool    g_waxing   = true;
static uint8_t g_illumPct = 0;
static float   g_ageDays  = 0.0f;

static char g_line1[32];     // "^ Age 0.0d | Ill 0%"
static char g_phaseLine[32]; // phase name or special text (limited length)
static char g_dateLine[11];  // "DD.MM.YYYY"

// ================== 1bpp bitmap draw ==================
static void drawBW_RowMajor(
  int16_t x0, int16_t y0,
  uint16_t w, uint16_t h,
  const uint8_t* data
) {
  uint16_t bytesPerRow = (w + 7) / 8;
  uint32_t idx = 0;

  for (uint16_t y = 0; y < h; y++) {
    for (uint16_t bx = 0; bx < bytesPerRow; bx++) {
      uint8_t b = pgm_read_byte(&data[idx++]);

      for (uint8_t bit = 0; bit < 8; bit++) {
        uint16_t x = bx * 8 + bit;
        if (x >= w) break;

        bool on = (b >> (7 - bit)) & 1;
        tft.drawPixel(x0 + x, y0 + y, on ? COLOR_WHITE : COLOR_BLACK);
      }
    }
  }
}

// ================== Moon calculations (only midnight) ==================
static float moonAgeDays_approx(const DateTime& now) {
  int y = now.year();
  int m = now.month();
  int d = now.day();

  int r = y % 100;
  r %= 19;
  if (r > 9) r -= 19;

  float rf = (float)(((r * 11) % 30) + m + d);
  if (m < 3) rf += 2.0f;
  rf += (y < 2000) ? -4.0f : -8.3f;

  const float SYNODIC = 29.5305889f;

  float age = fmod(rf, SYNODIC);
  if (age < 0) age += SYNODIC;

  return age; // 0..29.53
}

static uint8_t phaseIndex8_fromAge(float ageDays) {
  int ageRounded = (int)(ageDays + 0.5f);
  ageRounded %= 30;
  if (ageRounded < 0) ageRounded += 30;

  uint8_t idx;
  if (ageRounded == 0 || ageRounded == 29) idx = 0;
  else if (ageRounded >= 1  && ageRounded <= 6)  idx = 1;
  else if (ageRounded >= 7  && ageRounded <= 8)  idx = 2;
  else if (ageRounded >= 9  && ageRounded <= 13) idx = 3;
  else if (ageRounded >= 14 && ageRounded <= 15) idx = 4;
  else if (ageRounded >= 16 && ageRounded <= 20) idx = 5;
  else if (ageRounded >= 21 && ageRounded <= 22) idx = 6;
  else idx = 7;

  if (idx >= MOON_PHASE_COUNT) idx = 0;
  return idx;
}

static uint8_t illumPercent_fromAge(float ageDays) {
  const float SYNODIC = 29.5305889f;
  float phase = 2.0f * 3.14159265f * (ageDays / SYNODIC);
  float illum = (1.0f - cos(phase)) * 0.5f; // 0..1

  int pct = (int)(illum * 100.0f + 0.5f);
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return (uint8_t)pct;
}

static bool isWaxing_fromAge(float ageDays) {
  const float HALF = 29.5305889f / 2.0f; // ~14.765
  return (ageDays < HALF);
}

static void recomputeDailyCache(const DateTime& now) {
  g_ageDays  = moonAgeDays_approx(now);
  g_illumPct = illumPercent_fromAge(g_ageDays);
  g_waxing   = isWaxing_fromAge(g_ageDays);
  g_phaseIdx = phaseIndex8_fromAge(g_ageDays);

  char ageStr[8];
  dtostrf(g_ageDays, 4, 1, ageStr);

  char arrow = g_waxing ? '^' : 'v';
  snprintf(g_line1, sizeof(g_line1), "%c Age %sd | Ill %u%%", arrow, ageStr, g_illumPct);

  const char* phaseText = PHASE_EN[g_phaseIdx];
  for (uint8_t i = 0; i < SPECIAL_DAY_COUNT; i++) {
    if (now.day() == SPECIAL_DAYS[i].day && now.month() == SPECIAL_DAYS[i].month) {
      phaseText = SPECIAL_DAYS[i].text;
      break;
    }
  }
  strncpy(g_phaseLine, phaseText, sizeof(g_phaseLine) - 1);
  g_phaseLine[sizeof(g_phaseLine) - 1] = '\0';

  snprintf(g_dateLine, sizeof(g_dateLine), "%02d.%02d.%04d", now.day(), now.month(), now.year());
}

// ================== Drawing the bottom panel (without calculations) ==================
static void drawBottomPanelFromCache() {
  uint16_t y0 = (tft.maxY() + 1) - INFO_H;

  tft.fillRectangle(0, y0, tft.maxX(), tft.maxY(), COLOR_BLACK);
  tft.setFont(Terminal6x8);

  const uint8_t cw = 6;

  uint16_t x1 = (tft.maxX() + 1 - (uint16_t)strlen(g_line1) * cw) / 2;
  uint16_t x2 = (tft.maxX() + 1 - (uint16_t)strlen(g_phaseLine) * cw) / 2;
  uint16_t x3 = (tft.maxX() + 1 - (uint16_t)strlen(g_dateLine) * cw) / 2;

  tft.drawText(x1, y0 + 1,  g_line1);
  tft.drawText(x2, y0 + 10, g_phaseLine);
  tft.drawText(x3, y0 + 19, g_dateLine);
}

// ================== Redrawing the entire screen from cache (without calculations) ==================
static void redrawWholeScreenFromCache() {
  const uint8_t* bmp = (const uint8_t*)epd_bitmap_allArray[g_phaseIdx];

  tft.fillRectangle(0, 0, tft.maxX(), tft.maxY(), COLOR_BLACK);
  drawBW_RowMajor(0, 0, IMG_W, IMG_H_DRAW, bmp);
  drawBottomPanelFromCache();
}

// ================== DAY-OF-YEAR + SUN LOOKUP ==================
static bool isLeapYear(int y) {
  // gregoriánský kalendář
  if (y % 400 == 0) return true;
  if (y % 100 == 0) return false;
  return (y % 4 == 0);
}

static int dayOfYearIndex(int y, int m, int d) {
// returns 0..364 (for a non-leap year), 0..365 (for a leap year)
  static const uint8_t mdays_norm[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

  int idx = 0;
  for (int mm = 1; mm < m; mm++) {
    idx += mdays_norm[mm - 1];
    if (mm == 2 && isLeapYear(y)) idx += 1;
  }
  idx += (d - 1);
  return idx;
}

static void getSunriseSunsetForDate(const DateTime& now, int &sunriseMin, int &sunsetMin) {
// We are using the 2026 table (365 days). If it is a leap year,
// after February 28th we shift the index by -1 (skip February 29th) to keep it aligned.
  int y = now.year();
  int m = now.month();
  int d = now.day();

  int idx = dayOfYearIndex(y, m, d);

  if (isLeapYear(y) && m > 2) {
    idx -= 1;
  }

  if (idx < 0) idx = 0;
  if (idx > 364) idx = 364;

  sunriseMin = (int)pgm_read_word(&SUN_TIMES_2026[idx][0]);
  sunsetMin  = (int)pgm_read_word(&SUN_TIMES_2026[idx][1]);
}

// ================== LED update (every 15 minutes) ==================
static void updateSunLedIfNeeded(const DateTime& now) {
  int nowMin = now.hour() * 60 + now.minute();
  int quarter = nowMin / 15; // 0..95

  if (quarter == g_lastQuarter) return;
  g_lastQuarter = quarter;

  int sunriseMin, sunsetMin;
  getSunriseSunsetForDate(now, sunriseMin, sunsetMin);

  SunState s = computeSunState(nowMin, sunriseMin, sunsetMin);
  if (s == g_lastSunState) return;

  g_lastSunState = s;
  applySunState(s);
}

void setup() {
  Wire.begin();

  // LED pins
  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);

  tft.begin();
  tft.setOrientation(0);

  if (!rtc.begin()) {
    tft.fillRectangle(0, 0, tft.maxX(), tft.maxY(), COLOR_BLACK);
    tft.setFont(Terminal6x8);
    tft.drawText(10, 10, "RTC FAIL");
    while (1) {}
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  DateTime now = rtc.now();
  lastY = now.year();
  lastM = now.month();
  lastD = now.day();

  recomputeDailyCache(now);
  redrawWholeScreenFromCache();

  g_lastQuarter = -1;
  g_lastSunState = (SunState)255;
  updateSunLedIfNeeded(now);
}

void loop() {
  static unsigned long lastCheckMs = 0;
  const unsigned long CHECK_INTERVAL_MS = 10000;

#if ENABLE_HOURLY_REFRESH
  static int lastHour = -1;
#endif

  unsigned long ms = millis();
  if (ms - lastCheckMs < CHECK_INTERVAL_MS) return;
  lastCheckMs = ms;

  DateTime now = rtc.now();

// LED: check for changes every 15 minutes (in practice, it is evaluated when the "quarter" changes)
  updateSunLedIfNeeded(now);

// 1) Day change => recalculate + redraw
  if (now.year() != lastY || now.month() != lastM || now.day() != lastD) {
    lastY = now.year();
    lastM = now.month();
    lastD = now.day();

#if ENABLE_HOURLY_REFRESH
    lastHour = now.hour();
#endif

    recomputeDailyCache(now);
    redrawWholeScreenFromCache();
    return;
  }

#if ENABLE_HOURLY_REFRESH
  if (lastHour < 0) lastHour = now.hour();

  if (now.hour() != lastHour) {
    lastHour = now.hour();
    redrawWholeScreenFromCache();
    return;
  }
#endif
}
