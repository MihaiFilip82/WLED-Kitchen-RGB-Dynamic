#pragma once

#include "wled.h"

static const char _data_FX_MODE_KITCHEN_RGB_WAVE[] PROGMEM =
  "Kitchen RGB Wave Flow@ON Time,Color Length,OFF Time,Flow Speed,Wave Fade,Rainbow,Use Color 2,Use Color 3;Color 1,Color 2,Color 3;;";

class KitchenRGBUsermod : public Usermod
{
private:

  // ==================================================
  // CONFIG GENERAL
  // ==================================================

  bool enabled = true;
  uint8_t effectId = 255;

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
  // SEGMENT AUTO FIX
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

    return
      x * x *
      (3.0f - 2.0f * x);
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
  //
  // 0   = foarte lent
  // 255 = foarte rapid
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
  //
  // Se adapteaza automat la lungimea segmentului.
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
  // WAVE FADE
  //
  // custom3 = 0...31
  //
  // 0  = aproximativ 1 pixel
  // 31 = aproximativ 15 pixeli
  // ==================================================

  static float getWaveFade(uint8_t value)
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
  // RAINBOW
  // ==================================================

  static uint32_t rainbowColor(uint8_t pos)
  {
    pos =
      255 - pos;

    uint8_t r;
    uint8_t g;
    uint8_t b;

    if (pos < 85)
    {
      r =
        255 - pos * 3;

      g =
        0;

      b =
        pos * 3;
    }
    else if (pos < 170)
    {
      pos -= 85;

      r =
        0;

      g =
        pos * 3;

      b =
        255 - pos * 3;
    }
    else
    {
      pos -= 170;

      r =
        pos * 3;

      g =
        255 - pos * 3;

      b =
        0;
    }

    return
      RGBW32(
        r,
        g,
        b,
        0
      );
  }


  // ==================================================
  // COLOR BLEND
  // ==================================================

  static uint32_t blendColors(
    uint32_t a,
    uint32_t b,
    float amount
  )
  {
    amount =
      clamp01(amount);

    uint8_t aw =
      (a >> 24) & 0xFF;

    uint8_t ar =
      (a >> 16) & 0xFF;

    uint8_t ag =
      (a >> 8) & 0xFF;

    uint8_t ab =
      a & 0xFF;


    uint8_t bw =
      (b >> 24) & 0xFF;

    uint8_t br =
      (b >> 16) & 0xFF;

    uint8_t bg =
      (b >> 8) & 0xFF;

    uint8_t bb =
      b & 0xFF;


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


    return
      RGBW32(
        r,
        g,
        blue,
        w
      );
  }


  // ==================================================
  // REDUCERE LUMINOZITATE PENTRU WAVE
  // ==================================================

  static uint32_t scaleColor(
    uint32_t color,
    float level
  )
  {
    level =
      clamp01(level);

    uint8_t w =
      (color >> 24) & 0xFF;

    uint8_t r =
      (color >> 16) & 0xFF;

    uint8_t g =
      (color >> 8) & 0xFF;

    uint8_t b =
      color & 0xFF;


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


    return
      RGBW32(
        r,
        g,
        b,
        w
      );
  }


  // ==================================================
  // CULOAREA SELECTATA DE UTILIZATOR
  //
  // Color 1 este mereu activ.
  //
  // check2 = Use Color 2
  // check3 = Use Color 3
  // ==================================================

  static uint32_t userFlowColor(
    float cyclePosition
  )
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


    // ------------------------------------------
    // O singura culoare
    // ------------------------------------------

    if (count == 1)
    {
      return
        colors[0];
    }


    // ------------------------------------------
    // Normalizam pozitia intre 0 si 1
    // ------------------------------------------

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


    // Curba mai fina intre culori
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
  // CULOARE FINALA FLOW
  // ==================================================

  static uint32_t flowColor(
    uint16_t pixel,
    uint16_t length,
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


    // ------------------------------------------
    // RAINBOW
    // ------------------------------------------

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


      return
        rainbowColor(hue);
    }


    // ------------------------------------------
    // CULORILE ALES DE UTILIZATOR
    // ------------------------------------------

    return
      userFlowColor(position);
  }


  // ==================================================
  // MASCA WAVE ON / OFF
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


    // ==================================================
    // ON
    // ==================================================

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
      {
        return 0.0f;
      }


      if (distance >= fadeWidth)
      {
        return 1.0f;
      }


      return
        smoothStep(
          distance /
          fadeWidth
        );
    }


    // ==================================================
    // OFF
    // ==================================================

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
    {
      return 1.0f;
    }


    if (distance >= fadeWidth)
    {
      return 0.0f;
    }


    return
      1.0f -
      smoothStep(
        distance /
        fadeWidth
      );
  }


  // ==================================================
  // EFFECT
  // ==================================================

  static uint16_t mode_kitchen_rgb_wave()
  {
    if (SEGLEN == 0)
    {
      return FRAMETIME;
    }


    KitchenRGBUsermod* mod =
      instance;


    if (mod == nullptr)
    {
      return FRAMETIME;
    }


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
      getWaveFade(
        SEGMENT.custom3
      );


    // ==================================================
    // FINAL OFF
    // ==================================================

    if (
      mod->finalizeOff ||
      mod->ignoreNextStateChange
    )
    {
      SEGMENT.fill(0);

      return FRAMETIME;
    }


    // ==================================================
    // OFF WAVE
    // ==================================================

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


      for (
        uint16_t i = 0;
        i < SEGLEN;
        i++
      )
      {
        uint32_t color =
          flowColor(
            i,
            SEGLEN,
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


    // ==================================================
    // INITIALIZARE EFECT
    // ==================================================

    if (SEGENV.call == 0)
    {
      mod->onAnimating =
        true;

      mod->onStart =
        millis();

      mod->flowEpoch =
        millis();
    }


    // ==================================================
    // ON WAVE
    // ==================================================

    float onLevelProgress =
      1.0f;


    if (mod->onAnimating)
    {
      onLevelProgress =
        (float)(
          millis() -
          mod->onStart
        ) /
        (float)onDuration;


      if (onLevelProgress >= 1.0f)
      {
        onLevelProgress =
          1.0f;

        mod->onAnimating =
          false;
      }
      else
      {
        onLevelProgress =
          clamp01(
            onLevelProgress
          );
      }
    }


    // ==================================================
    // DESENARE
    // ==================================================

    for (
      uint16_t i = 0;
      i < SEGLEN;
      i++
    )
    {
      uint32_t color =
        flowColor(
          i,
          SEGLEN,
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
            onLevelProgress,
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
  // BUTTON POWER TOGGLE
  // ==================================================

  void togglePowerFromButton()
  {
    // ------------------------------------------
    // Daca se stinge chiar acum,
    // o apasare scurta il readuce ON.
    // ------------------------------------------

    if (offAnimating)
    {
      startOnAnimation();

      return;
    }


    // ------------------------------------------
    // OFF
    // ------------------------------------------

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


    // ------------------------------------------
    // ON
    // ------------------------------------------

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
  // 5 SEC = RAINBOW ON/OFF
  // ==================================================

  void toggleRainbow()
  {
    Segment& seg =
      strip.getMainSegment();


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
  // 10 SEC = RESTART EFECT
  //
  // Nu schimba:
  // - culorile
  // - viteza
  // - Rainbow
  // - celelalte setari
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
  // BUTTON RELEASE ACTION
  // ==================================================

  void handleButtonRelease(
    uint32_t duration
  )
  {
    // < 1 sec
    if (duration < 1000UL)
    {
      togglePowerFromButton();

      return;
    }


    // 1 sec - 5 sec
    // NIMIC
    if (duration < 5000UL)
    {
      return;
    }


    // 5 sec - 10 sec
    if (duration < 10000UL)
    {
      toggleRainbow();

      return;
    }


    // 10+ sec
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


    // ------------------------------------------
    // APASAT
    // ------------------------------------------

    if (stableButton == LOW)
    {
      buttonPressed =
        true;

      buttonPressStart =
        millis();

      return;
    }


    // ------------------------------------------
    // ELIBERAT
    // ------------------------------------------

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
  // AUTO FIX SEGMENT
  //
  // Daca avem un singur segment,
  // il extinde automat pe toata lungimea
  // configurata in LED Preferences.
  //
  // Exemplu:
  // Length = 100
  // segment blocat la 60
  // -> devine automat 0...100
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
      effectId =
        strip.addEffect(
          255,
          &mode_kitchen_rgb_wave,
          _data_FX_MODE_KITCHEN_RGB_WAVE
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


    // ------------------------------------------
    // FINAL OFF
    // ------------------------------------------

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


    if (
      seg.mode != effectId
    )
    {
      return;
    }


    // ==================================================
    // OFF REQUEST
    // ==================================================

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


    // ==================================================
    // ON REQUEST
    //
    // Repornim intotdeauna efectul de la inceput.
    // ==================================================

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


    cfg["effect-id"] =
      effectId;


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
    return
      USERMOD_ID_UNSPECIFIED;
  }
};


KitchenRGBUsermod*
KitchenRGBUsermod::instance =
  nullptr;
