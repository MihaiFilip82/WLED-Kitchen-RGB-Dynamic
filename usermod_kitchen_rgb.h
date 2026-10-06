#pragma once

#include "wled.h"

// ==================================================
// NUMELE EFECTELOR
// ==================================================

static const char _data_FX_MODE_KITCHEN_RGB_WAVE[] PROGMEM =
  "Kitchen RGB Wave Flow@ON Time,Color Length,OFF Time,Flow Speed,Wave Fade,Rainbow,Use Color 2,Use Color 3;Color 1,Color 2,Color 3;;";

static const char _data_FX_MODE_KITCHEN_WARM_WHITE[] PROGMEM =
  "Kitchen Warm White Wave@ON Time,Wave Fade,OFF Time;Brightness;;";

static const char _data_FX_MODE_KITCHEN_NEUTRAL_WHITE[] PROGMEM =
  "Kitchen Neutral White Wave@ON Time,Wave Fade,OFF Time;Brightness;;";

static const char _data_FX_MODE_KITCHEN_COOL_WHITE[] PROGMEM =
  "Kitchen Cool White Wave@ON Time,Wave Fade,OFF Time;Brightness;;";


class KitchenRGBUsermod : public Usermod
{
private:

  bool enabled = true;

  uint8_t effectIdRGB = 255;
  uint8_t effectIdWarm = 255;
  uint8_t effectIdNeutral = 255;
  uint8_t effectIdCool = 255;

  static KitchenRGBUsermod* instance;


  // ==================================================
  // BUTON IO13
  // ==================================================

  static const uint8_t BUTTON_PIN = 13;

  bool lastRawButton = HIGH;
  bool stableButton = HIGH;
  bool buttonPressed = false;

  uint32_t lastButtonChange = 0;
  uint32_t buttonPressStart = 0;

  static const uint32_t DEBOUNCE_MS = 35;


  // ==================================================
  // ANIMATIE ON / OFF
  // ==================================================

  bool onAnimating = false;
  bool offAnimating = false;

  bool finalizeOff = false;
  bool ignoreNextStateChange = false;

  uint32_t onStart = 0;
  uint32_t offStart = 0;
  uint32_t flowEpoch = 0;

  uint8_t savedBrightness = 128;


  // ==================================================
  // SEGMENT AUTO-FIX
  // ==================================================

  uint32_t lastSegmentCheck = 0;


  // ==================================================
  // HELPERS
  // ==================================================

  static float clamp01(float x)
  {
    if (x <= 0.0f) return 0.0f;
    if (x >= 1.0f) return 1.0f;

    return x;
  }


  static float smoothStep(float x)
  {
    x = clamp01(x);

    return x * x * (3.0f - 2.0f * x);
  }


  // ==================================================
  // ESTE EFECTUL NOSTRU?
  // ==================================================

  bool isOurEffect(uint8_t mode)
  {
    return
      mode == effectIdRGB ||
      mode == effectIdWarm ||
      mode == effectIdNeutral ||
      mode == effectIdCool;
  }


  // ==================================================
  // ON / OFF TIME
  //
  // 0   = 0.25 sec
  // 255 = 30 sec
  // ==================================================

  static uint32_t timeToDuration(uint8_t value)
  {
    const uint32_t minimum = 250UL;
    const uint32_t maximum = 30000UL;

    return
      minimum +
      (
        (uint32_t)value *
        (maximum - minimum) /
        255UL
      );
  }


  // ==================================================
  // FLOW SPEED
  // ==================================================

  static uint32_t flowToDuration(uint8_t value)
  {
    const uint32_t slowest = 30000UL;
    const uint32_t fastest = 500UL;

    return
      slowest -
      (
        (uint32_t)value *
        (slowest - fastest) /
        255UL
      );
  }


  // ==================================================
  // COLOR LENGTH
  // ==================================================

  static float getColorLength(
    uint8_t value,
    uint16_t length
  )
  {
    float minimum = 3.0f;

    float maximum =
      (length > 4)
        ? (float)length
        : 4.0f;

    return
      minimum +
      (
        (float)value /
        255.0f
      ) *
      (maximum - minimum);
  }


  // ==================================================
  // WAVE FADE RGB
  //
  // custom3 = 0...31
  // ==================================================

  static float getRGBWaveFade(uint8_t value)
  {
    return
      1.0f +
      (
        (float)value /
        31.0f
      ) *
      14.0f;
  }


  // ==================================================
  // WAVE FADE WHITE
  //
  // intensity = 0...255
  // ==================================================

  static float getWhiteWaveFade(uint8_t value)
  {
    return
      1.0f +
      (
        (float)value /
        255.0f
      ) *
      14.0f;
  }


  // ==================================================
  // RAINBOW
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

    return RGBW32(r, g, b, 0);
  }


  // ==================================================
  // BLEND CULORI
  // ==================================================

  static uint32_t blendColors(
    uint32_t a,
    uint32_t b,
    float amount
  )
  {
    amount = clamp01(amount);

    uint8_t aw = (a >> 24) & 0xFF;
    uint8_t ar = (a >> 16) & 0xFF;
    uint8_t ag = (a >> 8) & 0xFF;
    uint8_t ab = a & 0xFF;

    uint8_t bw = (b >> 24) & 0xFF;
    uint8_t br = (b >> 16) & 0xFF;
    uint8_t bg = (b >> 8) & 0xFF;
    uint8_t bb = b & 0xFF;

    uint8_t w =
      aw +
      (int16_t)(bw - aw) *
      amount;

    uint8_t r =
      ar +
      (int16_t)(br - ar) *
      amount;

    uint8_t g =
      ag +
      (int16_t)(bg - ag) *
      amount;

    uint8_t blue =
      ab +
      (int16_t)(bb - ab) *
      amount;

    return RGBW32(r, g, blue, w);
  }


  // ==================================================
  // SCALE COLOR
  // ==================================================

  static uint32_t scaleColor(
    uint32_t color,
    float level
  )
  {
    level = clamp01(level);

    uint8_t w = (color >> 24) & 0xFF;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    w =
      (uint8_t)(
        (float)w *
        level
      );

    r =
      (uint8_t)(
        (float)r *
        level
      );

    g =
      (uint8_t)(
        (float)g *
        level
      );

    b =
      (uint8_t)(
        (float)b *
        level
      );

    return RGBW32(r, g, b, w);
  }


  // ==================================================
  // CULORILE USERULUI
  // ==================================================

  static uint32_t userFlowColor(float cyclePosition)
  {
    uint32_t colors[3];

    uint8_t count = 0;

    colors[count++] =
      SEGCOLOR(0);

    if (SEGMENT.check2)
    {
      colors[count++] =
        SEGCOLOR(1);
    }

    if (SEGMENT.check3)
    {
      colors[count++] =
        SEGCOLOR(2);
    }

    if (count == 1)
    {
      return colors[0];
    }

    cyclePosition =
      cyclePosition -
      floorf(cyclePosition);

    float scaled =
      cyclePosition *
      (float)count;

    uint8_t index =
      (uint8_t)floorf(scaled);

    if (index >= count)
    {
      index =
        count - 1;
    }

    uint8_t next =
      (index + 1) %
      count;

    float local =
      scaled -
      floorf(scaled);

    local =
      smoothStep(local);

    return
      blendColors(
        colors[index],
        colors[next],
        local
      );
  }


  // ==================================================
  // RGB FLOW COLOR
  // ==================================================

  static uint32_t flowColor(
    uint16_t pixel,
    float colorLength,
    uint32_t flowDuration,
    uint32_t flowEpoch
  )
  {
    float elapsed =
      (float)(
        millis() -
        flowEpoch
      );

    float phase =
      elapsed /
      (float)flowDuration;

    float spatial =
      (float)pixel /
      colorLength;

    float position =
      spatial +
      phase;

    if (SEGMENT.check1)
    {
      float wrapped =
        position -
        floorf(position);

      uint8_t hue =
        (uint8_t)(
          wrapped *
          255.0f
        );

      return rainbowColor(hue);
    }

    return userFlowColor(position);
  }


  // ==================================================
  // ALBURI FIXE
  //
  // Banda este RGB, fara canal W.
  // Albul este simulat din RGB.
  // ==================================================

  static uint32_t warmWhiteColor()
  {
    // aprox. 2700-3000K
    return RGBW32(
      255,
      190,
      110,
      0
    );
  }


  static uint32_t neutralWhiteColor()
  {
    // aprox. 4000K
    return RGBW32(
      255,
      245,
      235,
      0
    );
  }


  static uint32_t coolWhiteColor()
  {
    // aprox. 6000-6500K
    return RGBW32(
      210,
      230,
      255,
      0
    );
  }


  // ==================================================
  // WAVE LEVEL
  // ==================================================

  static float waveLevel(
    uint16_t pos,
    uint16_t length,
    float progress,
    float fadeWidth,
    bool turningOff
  )
  {
    if (length == 0)
      return 0.0f;

    float lastPixel =
      (float)(
        length - 1
      );

    // ON
    if (!turningOff)
    {
      float head =
        progress *
        (
          lastPixel +
          fadeWidth
        );

      float distance =
        head -
        (float)pos;

      if (distance <= 0.0f)
        return 0.0f;

      if (distance >= fadeWidth)
        return 1.0f;

      return
        smoothStep(
          distance /
          fadeWidth
        );
    }

    // OFF
    float head =
      lastPixel -
      progress *
      (
        lastPixel +
        fadeWidth
      );

    float distance =
      (float)pos -
      head;

    if (distance <= 0.0f)
      return 1.0f;

    if (distance >= fadeWidth)
      return 0.0f;

    return
      1.0f -
      smoothStep(
        distance /
        fadeWidth
      );
  }


  // ==================================================
  // RGB WAVE FLOW
  // ==================================================

  static uint16_t mode_kitchen_rgb_wave()
  {
    if (SEGLEN == 0)
      return FRAMETIME;

    KitchenRGBUsermod* mod =
      instance;

    if (mod == nullptr)
      return FRAMETIME;

    uint32_t onDuration =
      timeToDuration(
        SEGMENT.speed
      );

    uint32_t offDuration =
      timeToDuration(
        SEGMENT.custom1
      );

    uint32_t flowDuration =
      flowToDuration(
        SEGMENT.custom2
      );

    float colorLength =
      getColorLength(
        SEGMENT.intensity,
        SEGLEN
      );

    float fadeWidth =
      getRGBWaveFade(
        SEGMENT.custom3
      );

    if (
      mod->finalizeOff ||
      mod->ignoreNextStateChange
    )
    {
      SEGMENT.fill(0);
      return FRAMETIME;
    }


    // OFF WAVE
    if (mod->offAnimating)
    {
      float progress =
        (float)(
          millis() -
          mod->offStart
        ) /
        (float)offDuration;

      if (progress >= 1.0f)
      {
        SEGMENT.fill(0);

        mod->offAnimating =
          false;

        mod->finalizeOff =
          true;

        return FRAMETIME;
      }

      progress =
        clamp01(progress);

      for (uint16_t i = 0; i < SEGLEN; i++)
      {
        uint32_t color =
          flowColor(
            i,
            colorLength,
            flowDuration,
            mod->flowEpoch
          );

        float level =
          waveLevel(
            i,
            SEGLEN,
            progress,
            fadeWidth,
            true
          );

        SEGMENT.setPixelColor(
          i,
          scaleColor(
            color,
            level
          )
        );
      }

      return FRAMETIME;
    }


    // INITIALIZARE
    if (SEGENV.call == 0)
    {
      mod->onAnimating =
        true;

      mod->onStart =
        millis();

      mod->flowEpoch =
        millis();
    }


    float onProgress =
      1.0f;

    if (mod->onAnimating)
    {
      onProgress =
        (float)(
          millis() -
          mod->onStart
        ) /
        (float)onDuration;

      if (onProgress >= 1.0f)
      {
        onProgress =
          1.0f;

        mod->onAnimating =
          false;
      }
      else
      {
        onProgress =
          clamp01(onProgress);
      }
    }


    for (uint16_t i = 0; i < SEGLEN; i++)
    {
      uint32_t color =
        flowColor(
          i,
          colorLength,
          flowDuration,
          mod->flowEpoch
        );

      float level =
        1.0f;

      if (mod->onAnimating)
      {
        level =
          waveLevel(
            i,
            SEGLEN,
            onProgress,
            fadeWidth,
            false
          );
      }

      SEGMENT.setPixelColor(
        i,
        scaleColor(
          color,
          level
        )
      );
    }

    return FRAMETIME;
  }


  // ==================================================
  // EFECT ALB STATIC CU WAVE
  // ==================================================

  static uint16_t mode_kitchen_white_wave()
  {
    if (SEGLEN == 0)
      return FRAMETIME;

    KitchenRGBUsermod* mod =
      instance;

    if (mod == nullptr)
      return FRAMETIME;


    uint32_t onDuration =
      timeToDuration(
        SEGMENT.speed
      );

    uint32_t offDuration =
      timeToDuration(
        SEGMENT.custom1
      );

    float fadeWidth =
      getWhiteWaveFade(
        SEGMENT.intensity
      );


    uint32_t color =
      warmWhiteColor();


    if (SEGMENT.mode == mod->effectIdNeutral)
    {
      color =
        neutralWhiteColor();
    }
    else if (SEGMENT.mode == mod->effectIdCool)
    {
      color =
        coolWhiteColor();
    }


    if (
      mod->finalizeOff ||
      mod->ignoreNextStateChange
    )
    {
      SEGMENT.fill(0);
      return FRAMETIME;
    }


    // OFF
    if (mod->offAnimating)
    {
      float progress =
        (float)(
          millis() -
          mod->offStart
        ) /
        (float)offDuration;


      if (progress >= 1.0f)
      {
        SEGMENT.fill(0);

        mod->offAnimating =
          false;

        mod->finalizeOff =
          true;

        return FRAMETIME;
      }


      progress =
        clamp01(progress);


      for (uint16_t i = 0; i < SEGLEN; i++)
      {
        float level =
          waveLevel(
            i,
            SEGLEN,
            progress,
            fadeWidth,
            true
          );

        SEGMENT.setPixelColor(
          i,
          scaleColor(
            color,
            level
          )
        );
      }


      return FRAMETIME;
    }


    // INITIALIZARE
    if (SEGENV.call == 0)
    {
      mod->onAnimating =
        true;

      mod->onStart =
        millis();

      mod->flowEpoch =
        millis();
    }


    float onProgress =
      1.0f;


    if (mod->onAnimating)
    {
      onProgress =
        (float)(
          millis() -
          mod->onStart
        ) /
        (float)onDuration;


      if (onProgress >= 1.0f)
      {
        onProgress =
          1.0f;

        mod->onAnimating =
          false;
      }
      else
      {
        onProgress =
          clamp01(onProgress);
      }
    }


    for (uint16_t i = 0; i < SEGLEN; i++)
    {
      float level =
        1.0f;

      if (mod->onAnimating)
      {
        level =
          waveLevel(
            i,
            SEGLEN,
            onProgress,
            fadeWidth,
            false
          );
      }

      SEGMENT.setPixelColor(
        i,
        scaleColor(
          color,
          level
        )
      );
    }


    return FRAMETIME;
  }


  // ==================================================
  // START ON
  // ==================================================

  void startOnAnimation()
  {
    finalizeOff =
      false;

    offAnimating =
      false;

    onAnimating =
      true;

    onStart =
      millis();

    flowEpoch =
      millis();

    strip.trigger();
  }


  // ==================================================
  // BUTTON ON/OFF
  // ==================================================

  void togglePowerFromButton()
  {
    if (offAnimating)
    {
      startOnAnimation();
      return;
    }


    if (bri > 0)
    {
      bri =
        0;

      briT =
        0;

      stateChanged =
        true;

      stateUpdated(
        CALL_MODE_BUTTON
      );

      return;
    }


    uint8_t target =
      briLast;


    if (target == 0)
    {
      target =
        128;
    }


    startOnAnimation();


    bri =
      target;

    briT =
      target;

    stateChanged =
      true;

    stateUpdated(
      CALL_MODE_BUTTON
    );
  }


  // ==================================================
  // 5-10 SEC = RAINBOW
  //
  // Functioneaza numai pe RGB Wave Flow.
  // ==================================================

  void toggleRainbow()
  {
    Segment& seg =
      strip.getMainSegment();


    if (seg.mode != effectIdRGB)
    {
      return;
    }


    seg.check1 =
      !seg.check1;


    seg.markForReset();

    strip.trigger();


    stateChanged =
      true;

    stateUpdated(
      CALL_MODE_BUTTON
    );
  }


  // ==================================================
  // 10+ SEC = RESTART EFECT
  // ==================================================

  void restartEffect()
  {
    if (bri > 0)
    {
      startOnAnimation();
    }
    else
    {
      flowEpoch =
        millis();
    }
  }


  // ==================================================
  // BUTTON RELEASE
  // ==================================================

  void handleButtonRelease(
    uint32_t duration
  )
  {
    // sub 1 sec = ON/OFF
    if (duration < 1000UL)
    {
      togglePowerFromButton();
      return;
    }

    // 1-5 sec = nimic
    if (duration < 5000UL)
    {
      return;
    }

    // 5-10 sec = Rainbow
    if (duration < 10000UL)
    {
      toggleRainbow();
      return;
    }

    // 10+ sec = restart efect
    restartEffect();
  }


  // ==================================================
  // BUTTON LOOP
  // ==================================================

  void handleButton()
  {
    bool raw =
      digitalRead(
        BUTTON_PIN
      );


    if (raw != lastRawButton)
    {
      lastRawButton =
        raw;

      lastButtonChange =
        millis();
    }


    if (
      millis() -
      lastButtonChange <
      DEBOUNCE_MS
    )
    {
      return;
    }


    if (raw == stableButton)
    {
      return;
    }


    stableButton =
      raw;


    if (stableButton == LOW)
    {
      buttonPressed =
        true;

      buttonPressStart =
        millis();

      return;
    }


    if (buttonPressed)
    {
      uint32_t duration =
        millis() -
        buttonPressStart;


      buttonPressed =
        false;


      handleButtonRelease(
        duration
      );
    }
  }


  // ==================================================
  // AUTO-FIX SEGMENT
  // ==================================================

  void ensureFullSegment()
  {
    if (
      millis() -
      lastSegmentCheck <
      2000UL
    )
    {
      return;
    }


    lastSegmentCheck =
      millis();


    if (
      strip.getSegmentsNum() != 1
    )
    {
      return;
    }


    uint16_t total =
      strip.getLengthTotal();


    if (total == 0)
    {
      return;
    }


    Segment& seg =
      strip.getSegment(0);


    if (
      seg.start == 0 &&
      seg.stop == total
    )
    {
      return;
    }


    uint8_t grp =
      seg.grouping;

    uint8_t spc =
      seg.spacing;

    uint16_t ofs =
      seg.offset;


    strip.suspend();


    seg.setGeometry(
      0,
      total,
      grp,
      spc,
      ofs
    );


    strip.resume();


    strip.trigger();
  }


public:

  // ==================================================
  // SETUP
  // ==================================================

  void setup() override
  {
    instance =
      this;


    pinMode(
      BUTTON_PIN,
      INPUT_PULLUP
    );


    lastRawButton =
      digitalRead(
        BUTTON_PIN
      );


    stableButton =
      lastRawButton;


    if (enabled)
    {
      effectIdRGB =
        strip.addEffect(
          188,
          &mode_kitchen_rgb_wave,
          _data_FX_MODE_KITCHEN_RGB_WAVE
        );


      effectIdWarm =
        strip.addEffect(
          189,
          &mode_kitchen_white_wave,
          _data_FX_MODE_KITCHEN_WARM_WHITE
        );


      effectIdNeutral =
        strip.addEffect(
          190,
          &mode_kitchen_white_wave,
          _data_FX_MODE_KITCHEN_NEUTRAL_WHITE
        );


      effectIdCool =
        strip.addEffect(
          191,
          &mode_kitchen_white_wave,
          _data_FX_MODE_KITCHEN_COOL_WHITE
        );
    }
  }


  // ==================================================
  // LOOP
  // ==================================================

  void loop() override
  {
    handleButton();

    ensureFullSegment();


    if (finalizeOff)
    {
      ignoreNextStateChange =
        true;


      bool oldFadeTransition =
        fadeTransition;


      fadeTransition =
        false;


      bri =
        0;

      briT =
        0;


      stateChanged =
        true;


      stateUpdated(
        CALL_MODE_NO_NOTIFY
      );


      fadeTransition =
        oldFadeTransition;


      finalizeOff =
        false;
    }
  }


  // ==================================================
  // STATE CHANGE
  // ==================================================

  void onStateChange(
    uint8_t callMode
  ) override
  {
    if (ignoreNextStateChange)
    {
      ignoreNextStateChange =
        false;

      return;
    }


    Segment& seg =
      strip.getMainSegment();


    if (!isOurEffect(seg.mode))
    {
      return;
    }


    // OFF
    if (
      bri == 0 &&
      briOld > 0 &&
      !offAnimating &&
      !finalizeOff
    )
    {
      savedBrightness =
        briOld;


      if (savedBrightness == 0)
      {
        savedBrightness =
          briLast;
      }


      bri =
        savedBrightness;

      briT =
        savedBrightness;


      onAnimating =
        false;


      offStart =
        millis();


      offAnimating =
        true;


      strip.trigger();

      return;
    }


    // ON
    if (
      bri > 0 &&
      briOld == 0
    )
    {
      startOnAnimation();
    }
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


    cfg["rgb-effect-id"] =
      effectIdRGB;


    cfg["warm-white-id"] =
      effectIdWarm;


    cfg["neutral-white-id"] =
      effectIdNeutral;


    cfg["cool-white-id"] =
      effectIdCool;


    cfg["button-gpio"] =
      BUTTON_PIN;
  }


  bool readFromConfig(
    JsonObject& root
  ) override
  {
    JsonObject cfg =
      root["Kitchen RGB"];


    if (cfg.isNull())
    {
      return false;
    }


    enabled =
      cfg["enabled"] |
      true;


    return true;
  }


  uint16_t getId() override
  {
    return USERMOD_ID_UNSPECIFIED;
  }
};


KitchenRGBUsermod*
KitchenRGBUsermod::instance =
  nullptr;
