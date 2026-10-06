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
  // EFECT RGB FLOW
  // ==================================================

  static uint16_t mode_kitchen_rgb_flow()
  {
    if (SEGLEN == 0)
      return FRAMETIME;


    // --------------------------------------------------
    // Viteza animației
    // --------------------------------------------------

    uint32_t duration =
      speedToDuration(
        SEGMENT.speed
      );


    // --------------------------------------------------
    // Faza curentă a culorilor
    // --------------------------------------------------

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


    // --------------------------------------------------
    // COLOR LENGTH
    //
    // intensity mic:
    // multe culori / gradient mai scurt
    //
    // intensity mare:
    // gradient lung / culori mai întinse
    // --------------------------------------------------

    float colorLength =
      4.0f +
      (
        (float)SEGMENT.intensity /
        255.0f
      ) *
      60.0f;


    // --------------------------------------------------
    // Desenăm banda
    // --------------------------------------------------

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


      // color_wheel() generează tranziție RGB lină
      uint32_t color =
        color_wheel(hue);


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
  //
  // Necesar pentru Usermod în WLED 0.15.3
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
