#include "wled.h"

class WordClockMatrix : public Usermod {
private:
  int  lastMinute     = -1;
  bool chimeTriggered = false;

  bool    usermodActive   = true;
  bool    displayItIs     = true;
  uint8_t dayBrightness   = 128;
  uint8_t nightBrightness = 16;
  uint16_t ledOffset      = 0;

  static const uint8_t LED_COUNT = 65;
  uint8_t maskLeds[LED_COUNT];

  // Word index arrays
  static constexpr uint8_t WORD_IT_IS[3]      = {  0,  1,  2 };
  static constexpr uint8_t WORD_HALF[4]       = {  3,  4,  5,  6 };
  static constexpr uint8_t WORD_PAST[4]       = { 24, 25, 26, 27 };
  static constexpr uint8_t WORD_TO[2]         = { 22, 23 };
  static constexpr uint8_t WORD_OCLOCK[5]     = { 60, 61, 62, 63, 64 };
  static constexpr uint8_t WORD_FIVE[3]       = { 16, 17, 18 };
  static constexpr uint8_t WORD_TEN[3]        = {  6,  7,  8 };
  static constexpr uint8_t WORD_QUARTER[5]    = {  8,  9, 10, 11, 12 };
  static constexpr uint8_t WORD_TWENTY[5]     = { 12, 13, 14, 15, 16 };
  static constexpr uint8_t WORD_TWENTYFIVE[7] = { 12, 13, 14, 15, 16, 17, 18 };

  // Hour index arrays
  static constexpr uint8_t HOUR_1[3]  = { 27, 28, 29 };
  static constexpr uint8_t HOUR_2[3]  = { 35, 36, 37 };
  static constexpr uint8_t HOUR_3[4]  = { 29, 30, 31, 32 };
  static constexpr uint8_t HOUR_4[4]  = { 32, 33, 34, 35 };
  static constexpr uint8_t HOUR_5[4]  = { 37, 38, 39, 40 };
  static constexpr uint8_t HOUR_6[3]  = { 43, 44, 45 };
  static constexpr uint8_t HOUR_7[4]  = { 40, 41, 42, 43 };
  static constexpr uint8_t HOUR_8[4]  = { 45, 46, 47, 48 };
  static constexpr uint8_t HOUR_9[3]  = { 48, 49, 50 };
  static constexpr uint8_t HOUR_10[3] = { 54, 55, 56 };
  static constexpr uint8_t HOUR_11[5] = { 50, 51, 52, 53, 54 };
  static constexpr uint8_t HOUR_12[5] = { 56, 57, 58, 59, 60 };

  static constexpr uint8_t HOUR_SIZES[13] = {
    0, 3, 3, 4, 4, 4, 3, 4, 4, 3, 3, 5, 5
  };

  void clearMask() {
    for (uint8_t i = 0; i < LED_COUNT; i++) maskLeds[i] = 0;
  }

  void applyWord(const uint8_t* word, uint8_t length) {
    for (uint8_t i = 0; i < length; i++) {
      uint8_t idx = word[i];
      if (idx < LED_COUNT) maskLeds[idx] = 1;
    }
  }

  void applyHour(uint8_t hour) {
    const uint8_t* w = nullptr;
    uint8_t len = 0;
    switch (hour) {
      case 1:  w = HOUR_1;  len = HOUR_SIZES[1];  break;
      case 2:  w = HOUR_2;  len = HOUR_SIZES[2];  break;
      case 3:  w = HOUR_3;  len = HOUR_SIZES[3];  break;
      case 4:  w = HOUR_4;  len = HOUR_SIZES[4];  break;
      case 5:  w = HOUR_5;  len = HOUR_SIZES[5];  break;
      case 6:  w = HOUR_6;  len = HOUR_SIZES[6];  break;
      case 7:  w = HOUR_7;  len = HOUR_SIZES[7];  break;
      case 8:  w = HOUR_8;  len = HOUR_SIZES[8];  break;
      case 9:  w = HOUR_9;  len = HOUR_SIZES[9];  break;
      case 10: w = HOUR_10; len = HOUR_SIZES[10]; break;
      case 11: w = HOUR_11; len = HOUR_SIZES[11]; break;
      case 12: w = HOUR_12; len = HOUR_SIZES[12]; break;
      default: break;
    }
    if (w && len) applyWord(w, len);
  }

  bool isNight() {
    uint8_t hr = hour(localTime);
    return (hr < 8 || hr >= 22);
  }

  void prepareMask(uint8_t h, uint8_t m) {
    clearMask();
    if (displayItIs) applyWord(WORD_IT_IS, sizeof(WORD_IT_IS));

    bool toNext = false;

    if (m == 0) {
      applyWord(WORD_OCLOCK, sizeof(WORD_OCLOCK));
    } else if (m <= 4) {
      // only hour
    } else if (m <= 9) {
      applyWord(WORD_FIVE, sizeof(WORD_FIVE));
      applyWord(WORD_PAST, sizeof(WORD_PAST));
    } else if (m <= 14) {
      applyWord(WORD_TEN, sizeof(WORD_TEN));
      applyWord(WORD_PAST, sizeof(WORD_PAST));
    } else if (m <= 19) {
      applyWord(WORD_QUARTER, sizeof(WORD_QUARTER));
      applyWord(WORD_PAST, sizeof(WORD_PAST));
    } else if (m <= 24) {
      applyWord(WORD_TWENTY, sizeof(WORD_TWENTY));
      applyWord(WORD_PAST, sizeof(WORD_PAST));
    } else if (m <= 29) {
      applyWord(WORD_TWENTYFIVE, sizeof(WORD_TWENTYFIVE));
      applyWord(WORD_PAST, sizeof(WORD_PAST));
    } else if (m <= 34) {
      applyWord(WORD_HALF, sizeof(WORD_HALF));
      applyWord(WORD_PAST, sizeof(WORD_PAST));
    } else if (m <= 39) {
      applyWord(WORD_TWENTYFIVE, sizeof(WORD_TWENTYFIVE));
      applyWord(WORD_TO, sizeof(WORD_TO));
      toNext = true;
    } else if (m <= 44) {
      applyWord(WORD_TWENTY, sizeof(WORD_TWENTY));
      applyWord(WORD_TO, sizeof(WORD_TO));
      toNext = true;
    } else if (m <= 49) {
      applyWord(WORD_QUARTER, sizeof(WORD_QUARTER));
      applyWord(WORD_TO, sizeof(WORD_TO));
      toNext = true;
    } else if (m <= 54) {
      applyWord(WORD_TEN, sizeof(WORD_TEN));
      applyWord(WORD_TO, sizeof(WORD_TO));
      toNext = true;
    } else {
      applyWord(WORD_FIVE, sizeof(WORD_FIVE));
      applyWord(WORD_TO, sizeof(WORD_TO));
      toNext = true;
    }

    uint8_t dispH = h;
    if (toNext) dispH = (h % 12) + 1;
    if (dispH == 0) dispH = 12;
    applyHour(dispH);
  }

public:
  void setup() override {
    clearMask();
    updateLocalTime();
    prepareMask(hour(localTime), minute(localTime));
    strip.setBrightness(isNight() ? nightBrightness : dayBrightness);
  }

  void connected() override {
    // not used
  }

  void loop() override {
    if (!usermodActive) return;
    updateLocalTime();
    int m = minute(localTime), h = hour(localTime);

    if (m != lastMinute) {
      strip.setBrightness(isNight() ? nightBrightness : dayBrightness);
      prepareMask(h, m);
      lastMinute = m;

      if (m == 0 && !chimeTriggered) {
        applyPreset(13, CALL_MODE_FX_CHANGED);
        chimeTriggered = true;
      } else if (m != 0) {
        chimeTriggered = false;
      }
    }
  }

  void handleOverlayDraw() override {
    if (!usermodActive) return;

    // Always use segment 0's primary color
    uint32_t col = 0xFFFFFF;
    if (strip.getSegmentsNum() > 0) {
      auto &seg0 = strip.getSegment(0);
      col = seg0.colors[0];
    }

    for (uint8_t i = 0; i < LED_COUNT; i++) {
      strip.setPixelColor(ledOffset + i, maskLeds[i] ? col : 0);
    }
  }

  void addToConfig(JsonObject &root) override {
    JsonObject obj = root[F("wordclock")];
    if (!obj) obj = root.createNestedObject(F("wordclock"));
    obj[F("active")]        = usermodActive;
    obj[F("displayItIs")]   = displayItIs;
    obj[F("dayBrightness")] = dayBrightness;
    obj[F("nightBrightness")] = nightBrightness;
    obj[F("ledOffset")]     = ledOffset;
  }

  bool readFromConfig(JsonObject &root) override {
    JsonObject obj = root[F("wordclock")];
    if (!obj) return false;
    if (obj.containsKey(F("active")))        usermodActive   = obj[F("active")].as<bool>();
    if (obj.containsKey(F("displayItIs")))    displayItIs     = obj[F("displayItIs")].as<bool>();
    if (obj.containsKey(F("dayBrightness"))) {
      uint16_t v = obj[F("dayBrightness")].as<uint16_t>();
      if (v <= 255) dayBrightness = (uint8_t)v;
    }
    if (obj.containsKey(F("nightBrightness"))) {
      uint16_t v = obj[F("nightBrightness")].as<uint16_t>();
      if (v <= 255) nightBrightness = (uint8_t)v;
    }
    if (obj.containsKey(F("ledOffset"))) {
      uint32_t v = obj[F("ledOffset")].as<uint32_t>();
      ledOffset = (uint16_t)v;
    }
    return true;
  }

  uint16_t getId() override {
    return 0x0150;
  }
};