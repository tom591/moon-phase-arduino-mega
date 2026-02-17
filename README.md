# 🌙 moon-phase-arduino-mega  
### Moon phase visualization on TFT display + day phase indication using RGB LED and RTC module  

A simple Arduino-based moon phase display built for makers and electronics enthusiasts.  

by Tom Salaj  

![Arduino](https://img.shields.io/badge/Arduino-MEGA-00979D?logo=arduino&logoColor=white)

---

**Status:** Tested and running on Arduino MEGA + 2.0" TFT (176x220)

---

## 🚀 What is it?

**moon-phase-arduino-mega** is an Arduino MEGA project (it does not run on UNO).  
After setting the current time in the RTC module, the TFT display renders the moon phase in eight steps:

- New Moon  
- Waxing Crescent  
- First Quarter  
- Waxing Gibbous  
- Full Moon  
- Waning Gibbous  
- Last Quarter  
- Waning Crescent  

The phases are not loaded from an SD card.  
They are rendered directly from bitmap data generated in a separate file: **`phases.h`**.

The project also includes day phase indication using an RGB LED.  
The **`led_sun.h`** module checks the RTC time every 15 minutes and evaluates one hour before and after specific times to determine:

- Night  
- Sunrise  
- Day  
- Sunset  

---

## 🧠 Main File – moon_phase.ino

`moon_phase.ino` is the main control file of the project.

It is responsible for:

- Initializing the TFT display  
- Initializing the RTC module (DS3231)  
- Reading the current date and time  
- Calculating the day-of-year index  
- Determining the correct moon phase from the lookup table  
- Redrawing the display when the day changes  
- Controlling RGB LED day-phase indication  
- Handling special calendar days  

### 🖥 Display Layout

The TFT display contains three text rows at the bottom:

1. **First row** – Moon age in days / Moon illumination in percentage  
   (updated once per day at midnight)

2. **Second row** – Current moon phase name  

3. **Third row** – Current date in format `DD.MM.YYYY`

The screen is not continuously recalculated.  
A full redraw happens only when the day changes, which keeps the system efficient.

---

## 📂 Details – phases.h

- Moon phase bitmaps were generated using:  
  https://javl.github.io/image2cpp/  
- Code settings match my TFT display (2.0" 176x220).  
- You can adjust dimensions to match your own TFT.  
- Bitmaps were generated 20px shorter in height to leave space for bottom text on the display (optional).  

---

## 💡 Details – led_sun.h

Four day phases: Night, Sunrise, Day, Sunset.

Example brightness setup:

```cpp
// Night
static const uint8_t BR_NIGHT_R = 0;
static const uint8_t BR_NIGHT_G = 0;
static const uint8_t BR_NIGHT_B = 20;
```

Each RGB LED or RGB module behaves differently in terms of brightness.  
In my case, I also used 220Ω resistors (based on testing).  
Adjust values according to your LED and desired brightness.

The logic for when to switch phases and how long the LED stays active before/after a defined time can be modified.  
In this implementation, it is set to one hour before and one hour after (see `sunrise.h`).

---

## 🎂 Special Days – moon_phase.ino

The code supports special calendar days.  
Instead of showing the moon phase name, you can display custom text (within TFT character limits).

Example:

```cpp
static const SpecialDay SPECIAL_DAYS[] = {
  {22, 2, "MOMS BIRTHDAY :)"},
  {14, 6, "TOMS BIRTHDAY :)"}
};
```

To add more dates, simply follow the same pattern.  
If you do not need this feature, remove the entries.

---

## 📅 Year Handling & Time Accuracy

The project currently uses a precomputed moon phase table for the year **2026 (365 days)**.

For leap years:
- February 29 is internally skipped to keep alignment with the 365-day table.

This means:

- The moon phase visualization is aligned to the 2026 cycle.  
- It is not based on real-time astronomical calculations.  
- In other years, minor shifts may appear.  

The same applies to sunrise and sunset indication times:
- The defined time windows may shift slightly forward or backward by a few days in different years.  
- This is expected behavior and does not affect the overall functionality.  

This project is **not an astronomical precision instrument.**  
It is a maker-oriented Arduino build — created for learning, experimentation, and visual enjoyment.

---

## 🌍 Daylight Saving Time (DST)

Daylight Saving Time is not handled automatically.

If DST changes in your region:
- Update the RTC module manually.

The RTC keeps raw time and does not adjust itself.

---

## ⚠️ Important – RTC Setup Required

This project is fully dependent on the RTC module (DS3231).

Before running the project, make sure to upload the current date and time to the RTC module.

If the RTC is not set correctly, moon phases, date display, and day-phase LED indication will not work as expected.

The RTC keeps time independently, so it only needs to be set once (unless power is lost or the backup battery is removed).

---

## 📸 Photos

<p align="center">
  <a href="GALLERY.md">
    <img src="photos/00.jpg" width="600">
  </a>
</p>

Full build photo gallery → [View Gallery](GALLERY.md)

---

## 🔌 Fritzing

Module layout and wiring:  
[https://forum.fritzing.org/](https://forum.fritzing.org/t/moon-phase-display-arduino-mega-tft-rgb-led/33797)

Fritzing project file (.fzz) is also available directly in this repository:  
[fritzing/moon-phase-arduino-mega.fzz](fritzing/moon-phase-arduino-mega.fzz)

---

## 🛠 Hardware Used

- Arduino MEGA  

- **RTC Module – DS3231 AT24C32**  
  https://www.laskakit.cz/en/arduino-rtc-hodiny-realneho-casu-ds3231-at24c32/

- **2.0" TFT Display – 176x220, ILI9225, SPI**  
  https://www.laskakit.cz/en/176x220-barevny-lcd-tft-displej-2-0--spi/

- **Keyes KY-009 RGB SMD LED 5050 Module**  
  https://www.laskakit.cz/en/keyes--ky-009-rgb-smd-led-5050-modul/

- 220Ω resistors (for RGB LED current limiting)  
- 5V Power Supply (USB or external source)  

(Used modules are examples — equivalents should work as well.)

---

## 📜 License

This project is released under the MIT License.  
Use freely, modify as needed. No warranty provided.

---

## 🚀 Final Words

**Learn, code, enjoy — good luck!**  
*Tom Salaj*

<a href="https://www.buymeacoffee.com/tomsalaj" target="_blank">
  <img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" width="120" alt="Buy Me a Coffee">
</a>
