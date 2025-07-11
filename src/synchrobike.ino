#include <I2S.h>
#include "painlessMesh.h"

// Mesh network configuration
#define   MESH_PREFIX     "synchrobike-fw"  // Mesh network name
#define   MESH_PASSWORD   "synchrobike-fw"  // Mesh network password
#define   MESH_PORT       5555              // Mesh network port

painlessMesh  mesh;  // Mesh network object

#include <string>
#include <sstream>
#include <iomanip>

char uniqueHostname[18]; // Fixed-size character array for "XX:XX:XX"

#include <ESP8266TrueRandom.h>  // Library for generating random numbers on ESP8266

// FastLED configuration
#define FASTLED_ESP8266_DMA // Use ESP8266's DMA for WS281x LEDs
#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_INTERRUPT_RETRY_COUNT 1
#include<FastLED.h>

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define LED_PIN     3 // GPIO pin for LED data (ignored with DMA)
#define NUM_LEDS    50 // Number of LEDs in the strip
#define BRIGHTNESS  255 // LED brightness (0-255)
#define LED_TYPE    WS2811 // LED type
#define COLOR_ORDER GRB // Color order for the LEDs
CRGB leds[NUM_LEDS]; // Array to hold LED colors

// Reverse LED strip animations
#define REVERSE_ANIMATIONS 0 // Set to 1 to reverse animations

// Function to get the LED index, considering reverse animations
inline int ledIndex(int i) {
#if REVERSE_ANIMATIONS
    return NUM_LEDS - 1 - i;
#else
    return i;
#endif
}

// Animation and palette timing configuration
#define HOLD_PALETTES_X_TIMES_AS_LONG 10 // Seconds to hold each color palette
#define HOLD_ANIMATION_X_TIMES_AS_LONG 20 // Seconds to hold each animation

// Palette and blending configuration
CRGBPalette16 currentPalette(CRGB::Black); // Current color palette
CRGBPalette16 targetPalette(OceanColors_p); // Target color palette
TBlendType    currentBlending;  // Blending type (NOBLEND or LINEARBLEND)

// Noise animation variables
static int16_t dist;  // Random number for noise generator
uint16_t xscale = 30; // Noise scale for x-axis
uint16_t yscale = 30; // Noise scale for y-axis
uint8_t maxChanges = 24; // Blending speed between palettes

// Flags for forcing changes
boolean force_direction_change = false;
boolean force_pallet_change = false;

// Variable to track the previous second for palette changes
uint32_t prev_second = 0;

// Variables for animation direction changes
uint8_t direction_change; // Time to wait before changing animation direction
uint32_t prev_direction_time = 0; // Variable to track the previous direction change time
bool direction = true; // Direction flag for animations

// Variables for sequences
uint8_t  thisfade = 8; // Fade speed
int       thishue = 192; // Starting hue
uint8_t   thisinc = 2; // Hue increment
uint8_t   thissat = 255; // Saturation
uint8_t   thisbri = 255; // Brightness
int       huediff = 256; // Hue range

// Firework animation variables
uint8_t firework_eased   = 0;
uint8_t firework_count   = 0;
uint8_t firework_lerpVal = 0;

uint32_t localNodeTime = 0; // Local time in microseconds
uint32_t lastMillis = 0;    // Last recorded millis() value

// Callback for receiving messages
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("[%s] SYSTEM: Received from %u msg=%s\n", uniqueHostname, from, msg.c_str());
}

// Callback for new connections
void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("[%s] SYSTEM: New Connection, nodeId = %u\n", uniqueHostname, nodeId);
}

// Callback for changed connections
void changedConnectionCallback() {
    Serial.printf("[%s] SYSTEM: Changed connections %s\n", uniqueHostname, mesh.subConnectionJson().c_str());
}

// Callback for node time adjustments
void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("[%s] SYSTEM: Adjusted time %u. Offset = %d\n", uniqueHostname, mesh.getNodeTime(), offset);
    localNodeTime += offset; // Adjust local time
    lastMillis = millis();   // Record the current millis()
    force_direction_change = true;
    force_pallet_change = true;
}

// Setup function
void setup() {
  Serial.begin(74880);

  // Format the unique MAC address and store it in uniqueHostname
  uint32_t chipId = ESP.getChipId();
  snprintf(uniqueHostname, sizeof(uniqueHostname), "%02X:%02X:%02X",
           (chipId >> 16) & 0xFF, (chipId >> 8) & 0xFF, chipId & 0xFF);

  Serial.printf("Initialized uniqueHostname: %s\n", uniqueHostname);

  // Initialize mesh network
  // mesh.setDebugMsgTypes( ERROR | STARTUP );  // Debug message types
  mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); 
  mesh.init( MESH_PREFIX, MESH_PASSWORD, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  
  // Initialize FastLED
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( BRIGHTNESS );
  set_max_power_in_volts_and_milliamps(5, 500); // Power management: 5V, 500mA
  
  // Seed random number generator
  randomSeed(ESP8266TrueRandom.random());
  direction_change = random(5,10); // Random time for direction change
  
  // Initialize target palette with random colors
  targetPalette = CRGBPalette16(CHSV(random(0,255), 255, random(128,255)),
                                CHSV(random(0,255), 255, random(128,255)),
                                CHSV(random(0,255), 192, random(128,255)),
                                CHSV(random(0,255), 255, random(128,255)));
  
  dist = random16(chipId); // Random number for noise generator
}

// Main loop
void loop() {
  mesh.update(); // Update mesh network

  // // Print free heap memory every 5 seconds
  EVERY_N_MILLISECONDS(5000) {
    // Serial.printf("Chip ID: %08X\n", ESP.getChipId());
    // Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
  }
  
  // // Update LEDs every 15 milliseconds
  EVERY_N_MILLISECONDS(15) {
    showLEDs();
  }

  // // Print current second every second
  EVERY_N_MILLISECONDS(1000) {
    Serial.printf("[%s] second: %d\n", uniqueHostname, getSecond());
  }
}

// Helper functions for time calculations
uint32_t getMillis() {
    return (mesh.getNodeTime() / 1000);
}

uint32_t getSecond() {
    return (mesh.getNodeTime() / 1000000);
}

uint32_t getMinute() {
    return ((mesh.getNodeTime() / 1000000) / 60);
}

// Array of animation functions to play through.
typedef void (*SimplePatternList[])();
SimplePatternList _animations = {
  fillNoise,
  confettiNoise,
  sparkles,
  firework,
  fireworks,
  fireworkWithBang,
};

// Index of the current animation.
int _currentAnimation = 0;

// Function to display LED animations
void showLEDs() {
  int numberOfAnimations = sizeof(_animations) / sizeof(_animations[0]);
  int animation = (getSecond() / HOLD_ANIMATION_X_TIMES_AS_LONG) % numberOfAnimations;
    
  if (animation != _currentAnimation || force_pallet_change) {
    _currentAnimation = animation;
    Serial.printf("[%s] animation: %u\n", uniqueHostname, _currentAnimation);
    
    // Reset animation-specific variables
    firework_eased = 0;
    firework_count = 0;
    firework_lerpVal = 0;
  }

  _animations[_currentAnimation](); // Call the current animation function
  FastLED.show(); // Update LEDs
}

void fillNoise() {
  nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);
  changePalette();
  fillnoise8();
}

void confettiNoise() {
  changePalette();
  nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);
  confettiNoise8();
}

void sparkles() {
  // Sparkles, single color 
  changePalette();
  nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);
  thisinc=1;
  thishue=0;
  thissat=255;
  thisfade=2;
  huediff=128;
  confetti();
}

void showPallet() {
  changePalette();
  for (int i = 0; i < NUM_LEDS; i++) {
    int index = 255 / NUM_LEDS * i;
    leds[ledIndex(i)] = ColorFromPalette(targetPalette, index, 255, LINEARBLEND);
  }
}

void changePalette(){
  // change color pallet every random (s)
  uint32_t second = getSecond() / HOLD_PALETTES_X_TIMES_AS_LONG;
  if ((prev_second != second) || force_pallet_change) {
      Serial.printf("[%s] Change color pallet %u\n", uniqueHostname, second);
      randomSeed(second);
      targetPalette = CRGBPalette16(
        CHSV(random(0,255), 255, random(128,255)),
        CHSV(random(0,255), 255, random(128,255)),
        CHSV(random(0,255), 192, random(128,255)),
        CHSV(random(0,255), 255, random(128,255)));
      prev_second = second;
      force_pallet_change = false;
  }
}

// Forces complementary colors, which is always pleasant but doesn't give great variety over time.
void changePaletteComplementary(){
  // change color pallet every random (s)
  uint32_t second = getSecond() / HOLD_PALETTES_X_TIMES_AS_LONG;
  if ((prev_second != second) || force_pallet_change) {
    Serial.printf("[%s] Change color pallet %u\n", uniqueHostname, second);
    randomSeed(second);

    // Color picker: https://alloyui.com/examples/color-picker/hsv

    uint8_t seedColor = random(0,255);
    targetPalette = CRGBPalette16(
      // First pick a random color.
      CHSV(seedColor, 255, random(200,255)),
      // A brighter white version of that color.
      CHSV(seedColor, 196, random(128,255)),
      // First split complementary color with random vibrance (darker).
      CHSV(hueMatch(seedColor, 6), 255, random(64,255)),
      // Second split complementary color with random vibrance (darker).
      CHSV(hueMatch(seedColor, 7), 255, random(64,255)));

    prev_second = second;
    force_pallet_change = false;
  }
}

uint8_t hueMatch(uint8_t hue, uint8_t SchemeNumber) {
  //1 : Analog -
  //2 : Analog +  
  //3 : Triad Color -
  //4 : Triad Color +
  //5 : Complementary
  //6 : Complementary -
  //7 : Complementary +
  
  switch (SchemeNumber) {
    case 1: return hue-21;
        break; 
    case 2: return hue+21;
        break; 
    case 3: return hue-85;
        break; 
    case 4: return hue+85;
        break; 
    case 5: return hue+127;
        break; 
    case 6: return hue+106;
        break; 
    case 7: return hue-106;
        break; 
    }
}

/* =============== NOISE ANIMATION =============== */

void fillnoise8() {
  // Just ONE loop to fill up the LED array as all of the pixels change.
  for(int i = 0; i < NUM_LEDS; i++) {
    // Get a value from the noise function. I'm using both x and y axis.
    uint8_t index = inoise8(0, dist+i* yscale) % 255;
    // With that value, look up the 8 bit colour palette value and assign it to the current LED.
    leds[ledIndex(i)] = ColorFromPalette(currentPalette, index, 255, LINEARBLEND);
  }
  changeDirection();
  if (direction){ // Moving along the distance (that random number we started out with). Vary it a bit with a sine wave.
    dist += beatsin8(10,1,4, mesh.getNodeTime());                        
  } else {
    dist -= beatsin8(10,1,4, mesh.getNodeTime()); 
  }
}

void changeDirection(){
    // change direction every random (s);
    uint32_t direction_time = getSecond() / direction_change;
    if((prev_direction_time != direction_time) || force_direction_change) {
        randomSeed(direction_time);
        direction = random(0,255) % 2;
        direction_change = random(5,6);
        prev_direction_time = direction_time;
        force_direction_change = false;
    }
}

/* =============== CONFETTI ANIMATION =============== */


void confetti() {                                             // random colored speckles that blink in and fade smoothly
    fadeToBlackBy(leds, NUM_LEDS, thisfade);                    // Low values = slower fade.
    int pos = random16(NUM_LEDS); 
    leds[ledIndex(pos)] = ColorFromPalette(currentPalette,  thishue + random16(huediff)/4, thisbri, currentBlending);
    thishue = thishue + thisinc;                                   // It increments here.
}

void confettiNoise8(){
    fadeToBlackBy(leds, NUM_LEDS, thisfade);   
    int pos = random16(NUM_LEDS);          
    uint8_t index = inoise8(0, dist+pos* yscale) % 255;
    leds[ledIndex(pos)] = ColorFromPalette(currentPalette, index, thisbri, currentBlending);
    dist += beatsin8(10,1,4, millis());       
}

/* =============== FIREWORK ANIMATION =============== */
/* = Single firework sent upward                    = */
/* ================================================== */
void firework() {
  changePalette();

  // Start with easeInVal at 0 and then go to 255 for the full easing.
  firework_eased = easeOutQuart(firework_count / 255.0) * 255; //ease8InOutCubic(count);
  firework_count++;

  // Map it to the number of LED's you have.
  firework_lerpVal = lerp8by8(0, NUM_LEDS, firework_eased);

  uint8_t index = inoise8(0, dist + firework_lerpVal * yscale) % 255;
  // With that value, look up the 8 bit colour palette value and assign it to the current LED.
  // Serial.printf("lerpVal %u\n", firework_lerpVal);

  if (firework_lerpVal >= NUM_LEDS) {
    // Serial.printf("Clamping on %u\n", firework_lerpVal);
    firework_lerpVal = NUM_LEDS - 1; // Clamp to the maximum index
  }
  leds[ledIndex(firework_lerpVal)] = ColorFromPalette(targetPalette, index, 255, LINEARBLEND);
  leds[ledIndex(firework_lerpVal)].maximizeBrightness();

  fadeToBlackBy(leds, NUM_LEDS, 16);  // 8 bit, 1 = slow fade, 255 = fast fade
}

float easeOutQuart(float t) {
  return 1-(--t)*t*t*t;
} 

float easeOutQuint(float t) {
  return 1+(--t)*t*t*t*t;
}

/* =============== FIREWORK w/ EXPLODE ANIMATION =============== */
/* = Single firework sent upward with explode ending           = */
/* ============================================================= */
float flarePos;
#define NUM_SPARKS 25 // max number (could be NUM_LEDS / 2);
float sparkPos[NUM_SPARKS];
float sparkVel[NUM_SPARKS];
float sparkCol[NUM_SPARKS];
float gravity; // m/s/s
float dying_gravity;
float c1;
float c2;
int nSparks;
void fireworkWithBang() {
  changePalette();
  if (firework_lerpVal != NUM_LEDS) { // Firework going up animation
    firework();
    if (firework_lerpVal == NUM_LEDS) {
        randomSeed(getSecond());
// Init Explosion Vars
        Serial.println("Init explosions");
        flarePos = NUM_LEDS - 10;
        sparkCol[0] = 255; // Initialize the first spark
        nSparks = NUM_SPARKS; // Set the number of sparks
        c1 = 120;
        c2 = 50;
        gravity = -0.004; // Gravity for sparks
        dying_gravity = gravity;

        // Initialize sparks
        for (int i = 0; i < nSparks && i < NUM_SPARKS; i++) {
            sparkPos[i] = flarePos;
            sparkVel[i] = (float(random16(0, 20000)) / 10000.0) - 1.0; // from -1 to 1
            sparkCol[i] = abs(sparkVel[i]) * 500; // set colors before scaling velocity to keep them bright
            sparkCol[i] = constrain(sparkCol[i], 0, 255);
            sparkVel[i] *= flarePos / NUM_LEDS; // proportional to height
        }
    }
  } else { // Firework exploding animation
    explode();
  }
}

void explode() {
  if(sparkCol[0] > c2/128) { // as long as our known spark is lit, work with all the sparks
   FastLED.clear();
    for (int i = 0; i < nSparks && i < NUM_SPARKS; i++) {
        sparkPos[i] += sparkVel[i];
        sparkPos[i] = constrain(sparkPos[i], NUM_LEDS - 25, NUM_LEDS);
        sparkVel[i] += dying_gravity;
        sparkCol[i] *= 0.99;
        leds[ledIndex(int(sparkPos[i]))] = ColorFromPalette(targetPalette, 255 - i, 255, LINEARBLEND);
        leds[ledIndex(int(sparkPos[i]))].maximizeBrightness();
        fadeToBlackBy(leds, NUM_LEDS, 32);
        if (int(sparkPos[i]) == (NUM_LEDS - 25)) {
            leds[ledIndex(int(sparkPos[i]))] = CRGB::Black;
        }
    }
    dying_gravity *= .995; // as sparks burn out they fall slower
  } else {
    FastLED.clear();
    force_pallet_change = true;
    firework_lerpVal = 0;
    firework_eased   = 0;
    firework_count   = 0;
  }
}


/* =============== FIREWORKS ANIMATION =============== */
/* = Shoot off multiple fireworks at once            = */
/* =============== FIREWORKS ANIMATION =============== */
#define  NUM_FIREWORKS 5 // Max amount fireworks can be displayed at once
uint8_t fireworks_eased[NUM_FIREWORKS];
uint8_t fireworks_count[NUM_FIREWORKS];
uint8_t fireworks_lerpVal[NUM_FIREWORKS];
CRGBPalette16 fireworks_pallete[NUM_FIREWORKS];
int chance = 10; // chance firework will launch;

void fireworks() {
  changePalette();

  // randomSeed(getSecond());
  for(int i = 0; i < NUM_FIREWORKS; i++) {
    // Serial.println(i);
    //Serial.println(fireworks_lerpVal[i]);
    // Serial.println(random(0,chance));
    if (fireworks_count[i] == 0 && ESP8266TrueRandom.random(0,chance) != chance - 1) {
      continue;
    } else {
      // Serial.println(fireworks_lerpVal[i]);
      // Start with easeInVal at 0 and then go to 255 for the full easing.
      fireworks_eased[i] = easeOutQuart(fireworks_count[i] / 255.0) * 255; //ease8InOutCubic(count);

      // Map it to the number of LED's you have.
      fireworks_lerpVal[i] = lerp8by8(0, NUM_LEDS, fireworks_eased[i]);

       uint8_t index = inoise8(0, fireworks_lerpVal[i]) % 255;
      // With that value, look up the 8 bit colour palette value and assign it to the current LED.
      if (fireworks_count[i] == 0) { // On first launch create color pallete
        randomSeed(getSecond());
        fireworks_pallete[i] = ColorFromPalette(
          CRGBPalette16(
            CHSV(random(0,255), 255, random(128,255)),
            CHSV(random(0,255), 255, random(128,255)),
            CHSV(random(0,255), 192, random(128,255)),
            CHSV(random(0,255), 255, random(128,255))
            ),
          index,
          255,
          LINEARBLEND
        );
      }
      fireworks_count[i] += 1;
      if (fireworks_lerpVal[i] >= NUM_LEDS) {
        fireworks_lerpVal[i] = NUM_LEDS - 1; // Clamp to the maximum index
      }
      leds[ledIndex(fireworks_lerpVal[i])] = ColorFromPalette(fireworks_pallete[i], index, 255, LINEARBLEND);
      leds[ledIndex(fireworks_lerpVal[i])].maximizeBrightness();
      // Serial.println("LETS GO " + fireworks_lerpVal[i]);
      if (fireworks_lerpVal[i] == 255) {
        // Serial.println(fireworks_lerpVal[i]);
        fireworks_lerpVal[i] = 0;
        fireworks_count[i] = 0;
        fireworks_eased[i] = 0;
      }


    }
    }
  fadeToBlackBy(leds, NUM_LEDS, 16);  // 8 bit, 1 = slow fade, 255 = fast fade
}
