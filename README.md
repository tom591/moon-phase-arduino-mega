# 🌙 moon-phase-arduino-mega  
### Moon phase visualization on TFT display + day phase indication using RGB LED and RTC module  
by Tom Salaj  

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

## 📸 Photos

![Moon Phase Project](photo1.jpg)

---

## 🔌 Fritzing

Module layout and wiring:  
https://fritzing.org/projects/xxx

---

## 🛠 Hardware Used

- Arduino MEGA  
- 2.0" TFT Display (176x220)  
- DS3231 RTC Module  
- RGB LED (+ 220Ω resistors)  
- Power supply of your choice  

---

## 📜 License

This project is released under the MIT License.  
Use freely, modify as needed. No warranty provided.

---

## ✨ Final Words

Learn, code, enjoy — good luck!  

**Tom Salaj**
