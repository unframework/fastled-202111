#include "FastLED.h"

// using code from TwinkleFox by Mark Kriegsman and inoise8_pal_demo.ino by Andrew Tuline

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define NUM_LEDS       12
#define LED_TYPE   TM1804
#define DATA_PIN        5
#define W_PIN           6
// #define VOLTS          12
// #define MAX_MA       4000

CRGBArray<NUM_LEDS> leds;

// palette state and target
CRGBPalette16 gCurrentPalette(CRGB::Black);
CRGBPalette16 gTargetPalette;

// @todo fluorescent bulb startup animation
void setup() {
  // W pin init
  pinMode(W_PIN, OUTPUT);
  digitalWrite(W_PIN, LOW);

  Serial.begin(9600);

  // safety startup delay
  delay(3000);

  // FastLED.setMaxPowerInVoltsAndMilliamps( VOLTS, MAX_MA);
  FastLED.addLeds<LED_TYPE,DATA_PIN>(leds, NUM_LEDS)
    .setCorrection(TypicalLEDStrip);

  // randomize math
  random16_add_entropy(analogRead(A0));

  // initial palette setup
  fillRandomPalette(gTargetPalette);
}

#define MAX_COMMAND_LEN 255
char commandBuffer[MAX_COMMAND_LEN + 1];
int commandLength = 0;

#define COMMAND_COUNT 2
char * commandStrings[] = {
  "off",
  "rgb"
};
typedef void (*commandPointer)();
commandPointer commands[] = {
  command_off,
  command_rgb
};

void runCommand() {
  for(int i = 0; i < COMMAND_COUNT; i++) {
    if (strncmp(commandBuffer, commandStrings[i], MAX_COMMAND_LEN) == 0) {
      commands[i]();
      break;
    }
  }
}

void loop() {
  if (Serial.available() > 0) {
    char incomingByte = Serial.read();
    if (incomingByte == '\r') {
      // cap the string
      commandBuffer[commandLength] = 0;
      runCommand();

      // reset command
      commandLength = 0;

      // echo new line and prompt
      Serial.write("\r\n> ");
    } else if (incomingByte == '\n') {
      // ignore completely @todo make either CR or LF work?
    } else if (commandLength < MAX_COMMAND_LEN) {
      commandBuffer[commandLength] = incomingByte;
      commandLength += 1;

      // echo back as well
      Serial.write(incomingByte);
    }
  }

  EVERY_N_MILLISECONDS(10) {
    nblendPaletteTowardPalette(gCurrentPalette, gTargetPalette, 12);
  }

  EVERY_N_SECONDS(5) {
    random16_add_entropy(analogRead(A0));

    // @todo cycle this occasionally, still
    // fillRandomPalette(gTargetPalette);
  }

  drawLEDs();

  FastLED.show();
}

#define NOISE_SCALE 80

void drawLEDs() {
  uint32_t clock32 = millis();

  for(int i = 0; i < NUM_LEDS; i++) {
    // generate simplex noise using both X and Y
    // @todo consider circle looping
    uint8_t value = inoise8(i * NOISE_SCALE, clock32 / 10 + i * NOISE_SCALE);

    // index into the palette
    leds[i] = ColorFromPalette(gCurrentPalette, value, 255, LINEARBLEND);
  }
}

// fill palette with random colours
void fillRandomPalette(CRGBPalette16& pal) {
  uint8_t baseC = random8();

  gTargetPalette = CRGBPalette16(
    CHSV(baseC + 16 + random8(16), 255, random8(128, 255)),
    CHSV(baseC + 32 + random8(32), 192, random8(128, 255)),
    CHSV(baseC + 48 + random8(48), 160, random8(128, 255)),
    CHSV(baseC + 8 + random8(8), 255, random8(128, 255))
  );
}

void command_off() {
  // @todo W-pin
  gTargetPalette = CRGBPalette16(CRGB::Black);
}

void command_rgb() {
  fillRandomPalette(gTargetPalette);
}
