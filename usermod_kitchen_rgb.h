#pragma once

#include "wled.h"

static const char _data_FX_MODE_KITCHEN_RGB_FLOW[] PROGMEM =
  "Kitchen RGB Flow@Speed,Color Length;Brightness;;";

class KitchenRGBUsermod : public Usermod
{
private:

  bool enabled = true;
  uint8_t effectId = 255;

  // ==================================================
  // VITEZA
  //
  // 0   = foarte lent
  // 255 = foarte rapid
  // ==================================================

  static uint32_t speedToDuration(uint8_t value)
  {
    const uint32_t slowest = 30000UL; // 30 secunde
    const uint32_t fastest = 500UL;   // 0.5 secunde

    return
      slowest -
      (
        (uint32_t)value *
        (slowest - fastest) /
        255UL
      );
  }


  // ==================================================
  // RAINBOW WHEEL
  //
  // Generează tranziție lină:
  // roșu -> verde -> albastru -> roșu
  // ==================================================

  static uint32_t rainbowColor(uint8_t pos)
  {
    pos = 255 - pos;

    uint8_t r;
    uint8_t g;
    uint8_t b;

    if (pos < 85)
    {
      r = 255 - pos * 3;
      g = 0;
      b = pos * 3;
    }
    else if (pos < 170)
    {
      pos -= 85;

      r = 0;
      g = pos * 3;
      b = 255 - pos * 3;
    }
    else
    {
      pos -= 170;

      r = pos * 3;
      g = 255 - pos * 3;
      b = 0;
    }

    return RGBW32(
      r,
      g,
      b,
      0
    );
  }


  // ==================================================
  // KITCHEN RGB FLOW
  // ==================================================

  static uint16_t mode_kitchen_rgb_flow()
  {
    if (SEGLEN == 0)
      return FRAMETIME;


    // ==================================================
    // SPEED
    // ==================================================

    uint32_t duration =
      speedToDuration(
        SEGMENT.speed
      );


    // ==================================================
    // FAZA ANIMAȚIEI
    // ==================================================

    uint32_t now =
      millis();

    uint8_t phase =
      (uint8_t)(
        (
          (now % duration) *
          255UL
        ) /
        duration
      );


    // ==================================================
    // COLOR LENGTH
    //
    // 0:
    // gradient scurt, mai multe culori
    //
    // 255:
    // gradient lung, foarte fluid
    // ==================================================

    float colorLength =
      4.0f +
      (
        (float)SEGMENT.intensity /
        255.0f
      ) *
      60.0f;


    // ==================================================
    // DESENĂM BANDA
    // ==================================================

    for (uint16_t i = 0; i < SEGLEN; i++)
    {
      float hueOffsetFloat =
        (
          (float)i /
          colorLength
        ) *
        255.0f;

      uint8_t hueOffset =
        (uint8_t)hueOffsetFloat;

      uint8_t hue =
        phase +
        hueOffset;


      uint32_t color =
        rainbowColor(
          hue
        );


      SEGMENT.setPixelColor(
        i,
        color
      );
    }


    return FRAMETIME;
  }


public:

  // ==================================================
  // SETUP
  // ==================================================

  void setup() override
  {
    if (enabled)
    {
      effectId =
        strip.addEffect(
          188,
          &mode_kitchen_rgb_flow,
          _data_FX_MODE_KITCHEN_RGB_FLOW
        );
    }
  }


  // ==================================================
  // LOOP
  // ==================================================

  void loop() override
  {
  }


  // ==================================================
  // CONFIG
  // ==================================================

  void addToConfig(
    JsonObject& root
  ) override
  {
    JsonObject cfg =
      root.createNestedObject(
        "Kitchen RGB"
      );

    cfg["enabled"] =
      enabled;

    cfg["effect-id"] =
      effectId;
  }


  bool readFromConfig(
    JsonObject& root
  ) override
  {
    JsonObject cfg =
      root["Kitchen RGB"];

    if (cfg.isNull())
      return false;

    enabled =
      cfg["enabled"] | true;

    return true;
  }


  uint16_t getId() override
  {
    return USERMOD_ID_UNSPECIFIED;
  }
};
