#pragma once
#include "wled.h"

#define USERMOD_ID_WORD_CLOCK_MATRIX 1010

/*
 * Word Clock Matrix Usermod
 * By @bwente - https://github.com/bwente/8x8_word_clock
 *
 * Displays time as words on an 8x8 LED matrix with word stencil overlay.
 * This header is the canonical WLED usermod implementation for this repo.
 * The earlier overlay-based implementation is preserved under legacy/.
 *
 * Hardware: 64 LEDs in a 1D strip (non-serpentine), arranged as 8x8 grid
 * Configure WLED as: 64 LEDs, 1D strip (NOT 2D matrix, NOT serpentine)
 *
 * LED Layout (64 LEDs, 1D strip config):
 * Row 0: IT IS (0-1), HALF (2-5), TEN (6-7)
 * Row 1: QUARTER (8-11), TWENTY (12-15)
 * Row 2: FIVE (16-17), MINUTES (18-21), TO (22-23)
 * Row 3: PAST (24-26), ONE (27-28), THREE (29-31)
 * Row 4: FOUR (32-34), TWO (35-36), FIVE (37-39)
 * Row 5: SEVEN (40-42), SIX (43-44), EIGHT (45-47)
 * Row 6: NINE (48-49), ELEVEN (50-53), TEN (54-55)
 * Row 7: TWELVE (56-59), O'CLOCK (60-63)
 */

class WordClockMatrix : public Usermod {
private:
  // State
  bool     enabled      = true;
  bool     initDone     = false;
  unsigned long lastTick = 0;
  uint8_t  lastMinute   = 99;
  uint32_t lastCol      = 0xFFFFFFFF;

  // Configuration
  uint8_t  dayBrightness   = 180;
  uint8_t  nightBrightness = 32;
  uint8_t  nightStartHour  = 22;
  uint8_t  nightEndHour    = 7;

  // Preset IDs
  int16_t  backgroundPresetId = 1;    // Segment 0 state (normally off/black)
  int16_t  foregroundPresetId = 2;    // Time word colors
  int16_t  chimePresetId      = 3;    // Hourly chime effect (0 = disabled)
  uint32_t chimeDuration      = 60000; // ms
  bool     chimeActive        = false;
  uint32_t chimeStopAt        = 0;
  bool     presetsCreated     = false;
  uint32_t wordColorBeforeChime = 0;
  bool     postChimeRestorePending = false;
  uint32_t postChimeRestoreAt      = 0;
  uint8_t  postChimeRestoreRepeats = 0;

  // PROGMEM strings
  static const char _name[];
  static const char _enabled[];

  void clearSegment(uint8_t segId) {
    if (segId == 6) {
      // Keep last segment present to preserve IDs, park on MINUTES area.
      strip.setSegment(segId, 18, 19);
      Segment& s = strip.getSegment(segId);
      s.setOption(SEG_OPTION_ON, false);
      s.setOption(SEG_OPTION_SELECTED, false);
      s.setMode(FX_MODE_STATIC);
      s.setPalette(0);
      s.colors[0] = RGBW32(0,0,0,0);
      return;
    }
    strip.setSegment(segId, 0, 0);
  }

  void ensureWordSegments() {
    if (strip.getSegmentsNum() >= 7) return;

    // Ensure segment 1 exists and is static
    strip.setSegment(1, 0, 2);
    {
      Segment& s1 = strip.getSegment(1);
      s1.setOption(SEG_OPTION_ON, true);
      s1.setOption(SEG_OPTION_SELECTED, true);
      s1.setMode(FX_MODE_STATIC);
      s1.setPalette(0);
    }

    // Ensure segments 2-6 exist (reserve IDs)
    for (uint8_t i = 2; i <= 6; i++) {
      strip.setSegment(i, 0, 1); // non-empty so segment is created
      Segment& s = strip.getSegment(i);
      s.setOption(SEG_OPTION_ON, false);
      s.setOption(SEG_OPTION_SELECTED, false);
      s.setMode(FX_MODE_STATIC);
      s.setPalette(0);
    }
  }

  void lockStatic(uint8_t segId) {
    Segment& s = strip.getSegment(segId);
    s.setMode(FX_MODE_STATIC);
    s.setPalette(0);
  }

  void setWordSegment(uint8_t segId, uint16_t first, uint16_t lastInclusive) {
    strip.setSegment(segId, first, lastInclusive + 1);
    lockStatic(segId);
    Segment& seg = strip.getSegment(segId);
    seg.colors[0] = strip.getSegment(1).colors[0];
    seg.setOption(SEG_OPTION_ON, true);
    seg.setOption(SEG_OPTION_SELECTED, false);
  }

  void enforceWordAppearance() {
    lastCol = 0xFFFFFFFF;
    syncWordColorsIfNeeded();
  }

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

  bool isNightTime(uint8_t hourNow) {
    if (nightStartHour > nightEndHour) {
      return (hourNow >= nightStartHour || hourNow < nightEndHour);
    } else {
      return (hourNow >= nightStartHour && hourNow < nightEndHour);
    }
  }

  void applyTimeBasedBrightness(uint8_t hourNow) {
    strip.setBrightness(isNightTime(hourNow) ? nightBrightness : dayBrightness);
  }

  void createDefaultPresets() {
    if (presetsCreated) return;

    // Note: WLED's savePreset API varies by version
    // This is a simplified approach - actual implementation may need adjustment

    DEBUG_PRINTLN(F("WordClock: Creating default presets..."));

    // We'll set the segments manually since preset saving API is complex
    // Users can save these configurations as presets manually

    presetsCreated = true;
    DEBUG_PRINTLN(F("WordClock: Default configuration ready"));
  }

  void applyNormalMode() {
    // Apply background preset (segment 0 off/black)
    if (backgroundPresetId > 0 && backgroundPresetId <= 250) {
      applyPreset((uint16_t)backgroundPresetId, CALL_MODE_DIRECT_CHANGE);
    } else {
      // Fallback: turn off segment 0
      Segment& bg = strip.getSegment(0);
      bg.setOption(SEG_OPTION_ON, false);
      bg.colors[0] = RGBW32(0,0,0,0);
    }

    // Apply foreground preset (time word colors)
    if (foregroundPresetId > 0 && foregroundPresetId <= 250) {
      applyPreset((uint16_t)foregroundPresetId, CALL_MODE_DIRECT_CHANGE);
    }
  }

  void startChime(uint8_t hourNow) {
    if (chimeActive || chimePresetId <= 0) return;

    // Capture current word color so we can restore after chime
    wordColorBeforeChime = strip.getSegment(1).colors[0];
    postChimeRestorePending = false;

    // Apply chime preset
    applyPreset((uint16_t)chimePresetId, CALL_MODE_DIRECT_CHANGE);

    // Override brightness based on time of day
    applyTimeBasedBrightness(hourNow);

    // Display current time (will show over background effect)
    displayTime(hourNow, 0);
    enforceWordAppearance();
    colorUpdated(CALL_MODE_DIRECT_CHANGE);

    chimeActive = true;
    chimeStopAt = millis() + chimeDuration;
  }

  void stopChime() {
    if (!chimeActive) return;

    // Restore background preset (segment 0)
    if (backgroundPresetId > 0 && backgroundPresetId <= 250) {
      applyPreset((uint16_t)backgroundPresetId, CALL_MODE_DIRECT_CHANGE);
    } else {
      // Fallback: turn off segment 0
      Segment& bg = strip.getSegment(0);
      bg.setOption(SEG_OPTION_ON, false);
      bg.colors[0] = RGBW32(0,0,0,0);
    }

    chimeActive = false;

    // Defer word restore to allow preset to apply
    postChimeRestorePending = true;
    postChimeRestoreRepeats = 3;
    postChimeRestoreAt = millis() + 250;
  }

  void restoreAfterChime() {
    ensureWordSegments();

    // Restore word segments to a known layout
    strip.setSegment(1, 0, 2);
    {
      Segment& s1 = strip.getSegment(1);
      s1.setOption(SEG_OPTION_ON, true);
      s1.setOption(SEG_OPTION_SELECTED, true);
      s1.setMode(FX_MODE_STATIC);
      s1.setPalette(0);
      s1.colors[0] = wordColorBeforeChime;
    }
    for (uint8_t i = 2; i <= 6; i++) {
      strip.setSegment(i, 0, 0);
      Segment& s = strip.getSegment(i);
      s.setOption(SEG_OPTION_ON, true);
      s.setOption(SEG_OPTION_SELECTED, false);
      s.setMode(FX_MODE_STATIC);
      s.setPalette(0);
    }

    // Update to current time
    updateLocalTime();
    applyTimeBasedBrightness(hour(localTime));
    displayTime(hour(localTime), minute(localTime));
    enforceWordAppearance();
    colorUpdated(CALL_MODE_DIRECT_CHANGE);
  }

  void displayTime(uint8_t hourVal, uint8_t minuteVal) {
    ensureWordSegments();

    clearSegment(2);
    clearSegment(3);
    clearSegment(4);
    clearSegment(5);
    clearSegment(6);

    bool toNext = false;

    if (minuteVal <= 4) {
      setWordSegment(5, 60, 63);  // OCLOCK
    } else if (minuteVal <= 9) {
      setWordSegment(3, 16, 17);  // FIVE
      setWordSegment(4, 24, 26);  // PAST
      setWordSegment(6, 18, 21);  // MINUTES
    } else if (minuteVal <= 14) {
      setWordSegment(3, 6, 7);    // TEN
      setWordSegment(4, 24, 26);  // PAST
      setWordSegment(6, 18, 21);  // MINUTES
    } else if (minuteVal <= 19) {
      setWordSegment(3, 8, 11);   // QUARTER
      setWordSegment(4, 24, 26);  // PAST
    } else if (minuteVal <= 24) {
      setWordSegment(3, 12, 15);  // TWENTY
      setWordSegment(4, 24, 26);  // PAST
      setWordSegment(6, 18, 21);  // MINUTES
    } else if (minuteVal <= 29) {
      setWordSegment(3, 12, 17);  // TWENTY FIVE
      setWordSegment(4, 24, 26);  // PAST
      setWordSegment(6, 18, 21);  // MINUTES
    } else if (minuteVal <= 34) {
      setWordSegment(3, 2, 5);    // HALF
      setWordSegment(4, 24, 26);  // PAST
    } else if (minuteVal <= 39) {
      setWordSegment(3, 12, 17);  // TWENTY FIVE
      setWordSegment(4, 22, 23);  // TO
      setWordSegment(6, 18, 21);  // MINUTES
      toNext = true;
    } else if (minuteVal <= 44) {
      setWordSegment(3, 12, 15);  // TWENTY
      setWordSegment(4, 22, 23);  // TO
      setWordSegment(6, 18, 21);  // MINUTES
      toNext = true;
    } else if (minuteVal <= 49) {
      setWordSegment(3, 8, 11);   // QUARTER
      setWordSegment(4, 22, 23);  // TO
      toNext = true;
    } else if (minuteVal <= 54) {
      setWordSegment(3, 6, 7);    // TEN
      setWordSegment(4, 22, 23);  // TO
      setWordSegment(6, 18, 21);  // MINUTES
      toNext = true;
    } else {
      setWordSegment(3, 16, 17);  // FIVE
      setWordSegment(4, 22, 23);  // TO
      setWordSegment(6, 18, 21);  // MINUTES
      toNext = true;
    }

    uint8_t dispHour = hourVal;
    if (toNext) dispHour++;
    dispHour %= 12;
    if (dispHour == 0) dispHour = 12;

    mapHourToSeg2(dispHour);
  }

public:
  inline void enable(bool en) {
    if (enabled == en) return;

    if (enabled && !en) {
      // Disabling
      for (uint8_t i = 2; i <= 6; i++) {
        clearSegment(i);
      }
      if (chimeActive) {
        stopChime();
        postChimeRestorePending = false;
        postChimeRestoreRepeats = 0;
      }
    }

    enabled = en;

    if (enabled && initDone) {
      // Re-enabling
      applyNormalMode();
      updateLocalTime();
      lastMinute = 99; // Force update
      applyTimeBasedBrightness(hour(localTime));
      displayTime(hour(localTime), minute(localTime));
      enforceWordAppearance();
    }
  }

  inline bool isEnabled() { return enabled; }

  void setup() override {
    // Create default presets on first run
    createDefaultPresets();

    // Segment 0: background (off by default, or use backgroundPreset)
    strip.setSegment(0, 0, 64);
    {
      Segment& s = strip.getSegment(0);
      s.colors[0] = RGBW32(0,0,0,0);
      s.setOption(SEG_OPTION_ON, false);
      s.setOption(SEG_OPTION_SELECTED, false);
      s.setMode(FX_MODE_STATIC);
      s.setPalette(0);
    }

    // Segment 1: IT IS (always on, user can change color)
    strip.setSegment(1, 0, 2);
    {
      Segment& s = strip.getSegment(1);
      // Set default warm amber color
      if (s.colors[0] == 0) {
        s.colors[0] = RGBW32(255, 180, 0, 0);
      }
      s.setOption(SEG_OPTION_ON, true);
      s.setOption(SEG_OPTION_SELECTED, true);
      s.setMode(FX_MODE_STATIC);
      s.setPalette(0);
    }

    // Segments 2-6: word segments
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

    initDone = true;
  }

  void loop() override {
    if (!enabled || strip.isUpdating()) return;

    // Fast path: check every 250ms
    if (millis() - lastTick < 250) {
      if (chimeActive && millis() >= chimeStopAt) stopChime();
      if (postChimeRestorePending && millis() >= postChimeRestoreAt) {
        restoreAfterChime();
        if (postChimeRestoreRepeats > 1) {
          postChimeRestoreRepeats--;
          postChimeRestoreAt = millis() + 250;
        } else {
          postChimeRestorePending = false;
          postChimeRestoreRepeats = 0;
        }
      }
      syncWordColorsIfNeeded();
      return;
    }
    lastTick = millis();

    updateLocalTime();
    uint8_t m = minute(localTime);
    uint8_t h = hour(localTime);

    if (m == lastMinute) return;
    lastMinute = m;

    applyTimeBasedBrightness(h);
    displayTime(h, m);
    enforceWordAppearance();
    colorUpdated(CALL_MODE_DIRECT_CHANGE);

    if (m == 0 && chimePresetId > 0) {
      startChime(h);
    }
  }

  void addToJsonInfo(JsonObject& root) override {
    JsonObject user = root["u"];
    if (user.isNull()) user = root.createNestedObject("u");

    JsonArray arr = user.createNestedArray(FPSTR(_name));

    if (!enabled) {
      arr.add("disabled");
      return;
    }

    // Warn if 2D enabled (common mistake)
    if (strip.isMatrix) {
      arr.add("Disable 2D config");
      return;
    }

    // Show current time
    updateLocalTime();
    char timeStr[16];
    sprintf(timeStr, "%02d:%02d", hour(localTime), minute(localTime));
    arr.add(timeStr);

    // Show status
    if (chimeActive) {
      arr.add("chiming");
    } else if (isNightTime(hour(localTime))) {
      arr.add("night");
    } else {
      arr.add("day");
    }
  }

  void addToConfig(JsonObject& root) override {
    JsonObject top = root.createNestedObject(FPSTR(_name));
    top[FPSTR(_enabled)]        = enabled;
    top["dayBright"]            = dayBrightness;
    top["nightBright"]          = nightBrightness;
    top["nightStart"]           = nightStartHour;
    top["nightEnd"]             = nightEndHour;
    top["backgroundPreset"]     = backgroundPresetId;
    top["foregroundPreset"]     = foregroundPresetId;
    top["chimePreset"]          = chimePresetId;
    top["chimeDuration"]        = chimeDuration / 1000;
    top["presetsCreated"]       = presetsCreated;
  }

  bool readFromConfig(JsonObject& root) override {
    JsonObject top = root[FPSTR(_name)];
    if (top.isNull()) return false;

    bool configComplete = true;

    configComplete &= getJsonValue(top[FPSTR(_enabled)], enabled, true);
    configComplete &= getJsonValue(top["dayBright"], dayBrightness, 180);
    configComplete &= getJsonValue(top["nightBright"], nightBrightness, 32);
    configComplete &= getJsonValue(top["nightStart"], nightStartHour, 22);
    configComplete &= getJsonValue(top["nightEnd"], nightEndHour, 7);
    configComplete &= getJsonValue(top["backgroundPreset"], backgroundPresetId, 1);
    configComplete &= getJsonValue(top["foregroundPreset"], foregroundPresetId, 2);
    configComplete &= getJsonValue(top["chimePreset"], chimePresetId, 3);
    configComplete &= getJsonValue(top["presetsCreated"], presetsCreated, false);

    uint32_t chimeSec = chimeDuration / 1000;
    configComplete &= getJsonValue(top["chimeDuration"], chimeSec, 60);
    chimeDuration = chimeSec * 1000;

    // Validate ranges
    dayBrightness = constrain(dayBrightness, 1, 255);
    nightBrightness = constrain(nightBrightness, 1, 255);
    nightStartHour = constrain(nightStartHour, 0, 23);
    nightEndHour = constrain(nightEndHour, 0, 23);
    backgroundPresetId = constrain(backgroundPresetId, 0, 250);
    foregroundPresetId = constrain(foregroundPresetId, 0, 250);
    chimePresetId = constrain(chimePresetId, 0, 250);
    chimeDuration = constrain(chimeDuration, 1000, 600000);

    return configComplete;
  }

  uint16_t getId() override {
    return USERMOD_ID_WORD_CLOCK_MATRIX;
  }
};

// PROGMEM strings
const char WordClockMatrix::_name[]    PROGMEM = "WordClockMatrix";
const char WordClockMatrix::_enabled[] PROGMEM = "enabled";
