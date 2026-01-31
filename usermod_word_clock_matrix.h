#pragma once
#include "wled.h"

#define USERMOD_ID_WORD_CLOCK_MATRIX 1010

class WordClockMatrix : public Usermod {
private:
  unsigned long lastTick   = 0;
  uint8_t       lastMinute = 99;

  int dayBrightness   = 128;
  int nightBrightness = 16;

  // chime settings
  int        chimePresetId  = 16;     // user preset id
  uint32_t   chimeDuration  = 60000;  // ms
  bool       chimeActive    = false;
  uint32_t   chimeStopAt    = 0;

  uint32_t   lastCol = 0xFFFFFFFF;    // cache of IT IS color

  // clear a segment
  void clearSegment(uint8_t segId) {
    strip.setSegment(segId, 0, 0);
  }

  // force solid effect and default palette
  void lockStatic(uint8_t segId) {
    Segment& s = strip.getSegment(segId);
    s.setMode(FX_MODE_STATIC);
    s.setPalette(0);
  }

  // set a word segment using inclusive last index, follow IT IS color
  void setWordSegment(uint8_t segId, uint16_t first, uint16_t lastInclusive) {
    strip.setSegment(segId, first, lastInclusive + 1);  // stop is exclusive
    lockStatic(segId);
    Segment& seg = strip.getSegment(segId);
    seg.colors[0] = strip.getSegment(1).colors[0];
    seg.setOption(SEG_OPTION_ON, true);
    seg.setOption(SEG_OPTION_SELECTED, false);
  }

  // **UPDATED/REFACTORED** keep 2..6 in solid mode and match IT IS color
  void enforceWordAppearance() {
    lastCol = 0xFFFFFFFF; // Set lastCol to an invalid value to force an update
    syncWordColorsIfNeeded();
  }

  // push IT IS color to words if changed
  void syncWordColorsIfNeeded() {
    uint32_t c = strip.getSegment(1).colors[0];
    if (c == lastCol) return;
    lastCol = c;
    for (uint8_t i = 2; i <= 6; i++) {
      strip.getSegment(i).colors[0] = c;
      lockStatic(i);
    }
    colorUpdated(CALL_MODE_DIRECT_CHANGE);
  }

  // hour word mapping into segment 2
  void mapHourToSeg2(uint8_t h) {
    switch (h) {
      case 1:  setWordSegment(2, 27, 28); break;  // ONE
      case 2:  setWordSegment(2, 35, 36); break;  // TWO
      case 3:  setWordSegment(2, 29, 31); break;  // THREE
      case 4:  setWordSegment(2, 32, 34); break;  // FOUR
      case 5:  setWordSegment(2, 37, 39); break;  // FIVE
      case 6:  setWordSegment(2, 43, 44); break;  // SIX
      case 7:  setWordSegment(2, 40, 42); break;  // SEVEN
      case 8:  setWordSegment(2, 45, 47); break;  // EIGHT
      case 9:  setWordSegment(2, 48, 49); break;  // NINE
      case 10: setWordSegment(2, 54, 55); break;  // TEN
      case 11: setWordSegment(2, 50, 53); break;  // ELEVEN
      case 12: setWordSegment(2, 56, 59); break;  // TWELVE
    }
  }

  // apply day or night brightness
  void applyTimeBasedBrightness(uint8_t hourNow) {
    bool night = (hourNow < 8 || hourNow >= 22);
    strip.setBrightness(night ? nightBrightness : dayBrightness);
  }

  // run chime on segment 0, keep words above it, honor brightness
  void startChime(uint8_t hourNow) {
    if (chimeActive || chimePresetId <= 0) return;

    // select only segment 0 for preset apply
    strip.setSegment(0, 0, 64);
    for (uint16_t i = 0; i < strip.getMaxSegments(); i++) {
      strip.getSegment(i).setOption(SEG_OPTION_SELECTED, false);
    }
    strip.getSegment(0).setOption(SEG_OPTION_SELECTED, true);
    strip.getSegment(0).setOption(SEG_OPTION_ON, true);

    // apply user preset, then enforce brightness
    applyPreset((uint16_t)chimePresetId, CALL_MODE_DIRECT_CHANGE);
    applyTimeBasedBrightness(hourNow);

    // repaint words with OCLOCK visible at top of hour
    displayTime(hourNow, 0);
    enforceWordAppearance();
    colorUpdated(CALL_MODE_DIRECT_CHANGE);

    chimeActive = true;
    chimeStopAt = millis() + chimeDuration;
  }

  // **UPDATED** stop chime and clear segment 0
  void stopChime() {
    if (!chimeActive) return;
    Segment& bg = strip.getSegment(0);
    bg.setMode(FX_MODE_STATIC);
    bg.setPalette(0);
    bg.colors[0] = RGBW32(0,0,0,0);
    bg.setOption(SEG_OPTION_ON, false);
    bg.setOption(SEG_OPTION_SELECTED, false);
    strip.getSegment(1).setOption(SEG_OPTION_SELECTED, true);
    chimeActive = false;

    // **FIX ADDED** Update the time display immediately to remove "OCLOCK"
    updateLocalTime();
    displayTime(hour(localTime), minute(localTime));

    colorUpdated(CALL_MODE_DIRECT_CHANGE);
  }

public:
  WordClockMatrix() {}

  void setup() override {
    // 0: background off by default
    strip.setSegment(0, 0, 64);
    {
      Segment& s = strip.getSegment(0);
      s.colors[0] = RGBW32(0,0,0,0);
      s.setOption(SEG_OPTION_ON, false);
      s.setOption(SEG_OPTION_SELECTED, false);
      s.setMode(FX_MODE_STATIC);
      s.setPalette(0);
    }

    // 1: IT IS [0..1]
    strip.setSegment(1, 0, 2);
    {
      Segment& s = strip.getSegment(1);
      s.setOption(SEG_OPTION_ON, true);
      s.setOption(SEG_OPTION_SELECTED, true);
      s.setMode(FX_MODE_STATIC);
      s.setPalette(0);
    }

    // 2..6: word segments
    for (uint8_t i = 2; i <= 6; i++) {
      strip.setSegment(i, 0, 0);
      Segment& s = strip.getSegment(i);
      s.setOption(SEG_OPTION_ON, true);
      s.setOption(SEG_OPTION_SELECTED, false);
      s.setMode(FX_MODE_STATIC);
      s.setPalette(0);
    }

    updateLocalTime();
    applyTimeBasedBrightness(hour(localTime));
    displayTime(hour(localTime), minute(localTime));
    lastMinute = minute(localTime);
    lastCol = strip.getSegment(1).colors[0];
    enforceWordAppearance();
    colorUpdated(CALL_MODE_DIRECT_CHANGE);
  }

  // **UPDATED** loop()
  void loop() override {
    // fast path
    if (millis() - lastTick < 250) {
      if (chimeActive && millis() >= chimeStopAt) stopChime();
      syncWordColorsIfNeeded();
      return;
    }
    lastTick = millis();

    updateLocalTime();
    uint8_t m = minute(localTime);
    uint8_t h = hour(localTime);

    // Redundant check was removed from here

    if (m == lastMinute) return;
    lastMinute = m;

    applyTimeBasedBrightness(h);
    displayTime(h, m);
    enforceWordAppearance();
    colorUpdated(CALL_MODE_DIRECT_CHANGE);

    if (m == 0) startChime(h);
    else        stopChime(); // Note: This will call stopChime every minute if not on the hour
  }

  // seg 2 hour, seg 3 number word, seg 4 PAST or TO, seg 5 OCLOCK, seg 6 MINUTES
  void displayTime(uint8_t hourVal, uint8_t minuteVal) {
    clearSegment(2);
    clearSegment(3);
    clearSegment(4);
    clearSegment(5);
    clearSegment(6);

    bool toNext = false;

    // OCLOCK for 0..4
    if (minuteVal <= 4) {
      setWordSegment(5, 60, 63);
    }

    if (minuteVal <= 4) {
      // hour only with OCLOCK already set
    } else if (minuteVal <= 9) {
      setWordSegment(3, 16, 17);              // FIVE
      setWordSegment(4, 24, 26);              // PAST
      setWordSegment(6, 18, 21);              // MINUTES
    } else if (minuteVal <= 14) {
      setWordSegment(3, 6, 7);                // TEN
      setWordSegment(4, 24, 26);              // PAST
      setWordSegment(6, 18, 21);              // MINUTES
    } else if (minuteVal <= 19) {
      setWordSegment(3, 8, 11);               // QUARTER
      setWordSegment(4, 24, 26);              // PAST
    } else if (minuteVal <= 24) {
      setWordSegment(3, 12, 15);              // TWENTY
      setWordSegment(4, 24, 26);              // PAST
      setWordSegment(6, 18, 21);              // MINUTES
    } else if (minuteVal <= 29) {
      setWordSegment(3, 12, 17);              // TWENTY FIVE
      setWordSegment(4, 24, 26);              // PAST
      setWordSegment(6, 18, 21);              // MINUTES
    } else if (minuteVal <= 34) {
      setWordSegment(3, 2, 5);                // HALF
      setWordSegment(4, 24, 26);              // PAST
    } else if (minuteVal <= 39) {
      setWordSegment(3, 12, 17);              // TWENTY FIVE
      setWordSegment(4, 22, 23);              // TO
      setWordSegment(6, 18, 21);              // MINUTES
      toNext = true;
    } else if (minuteVal <= 44) {
      setWordSegment(3, 12, 15);              // TWENTY
      setWordSegment(4, 22, 23);              // TO
      setWordSegment(6, 18, 21);              // MINUTES
      toNext = true;
    } else if (minuteVal <= 49) {
      setWordSegment(3, 8, 11);               // QUARTER
      setWordSegment(4, 22, 23);              // TO
      toNext = true;
    } else if (minuteVal <= 54) {
      setWordSegment(3, 6, 7);                // TEN
      setWordSegment(4, 22, 23);              // TO
      setWordSegment(6, 18, 21);              // MINUTES
      toNext = true;
    } else {
      setWordSegment(3, 16, 17);              // FIVE
      setWordSegment(4, 22, 23);              // TO
      setWordSegment(6, 18, 21);              // MINUTES
      toNext = true;
    }

    uint8_t dispHour = hourVal;
    if (toNext) dispHour++;
    dispHour %= 12;
    if (dispHour == 0) dispHour = 12;

    mapHourToSeg2(dispHour);
  }

  void addToConfig(JsonObject &root) override {
    JsonObject o = root.createNestedObject("WordClockMatrix");
    o["dayBrightness"]   = dayBrightness;
    o["nightBrightness"] = nightBrightness;
    o["chimePresetId"]   = chimePresetId;
    o["chimeDuration"]   = (int)chimeDuration;
  }

  bool readFromConfig(JsonObject &root) override {
    JsonObject top = root["WordClockMatrix"];
    if (top.isNull()) return false;
    dayBrightness   = top["dayBrightness"]   | dayBrightness;
    nightBrightness = top["nightBrightness"] | nightBrightness;
    chimePresetId   = top["chimePresetId"]   | chimePresetId;
    chimeDuration   = top["chimeDuration"]   | (int)chimeDuration;
    return true;
  }

  uint16_t getId() override {
    return USERMOD_ID_WORD_CLOCK_MATRIX;
  }
};