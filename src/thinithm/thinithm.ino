#include "segaSlider.h"
#include "sliderdefs.h"
#include "airTower.h"
#include "pins.h"
#include <QuickMpr121.h>
#include <FastLED.h>
#include <Keyboard.h>


// disable input scanning and generate fake slider data instead
// (useful for testing protocol without full hardware)
#define FAKE_DATA false

#define FAKE_DATA_TYPE_CHASE 0
#define FAKE_DATA_TYPE_PULSE 1
#define FAKE_DATA_TYPE_TIMERS 2
#define FAKE_DATA_TYPE FAKE_DATA_TYPE_TIMERS

#define FAKE_DATA_DO_SCANNING true


#if FAKE_DATA && FAKE_DATA_TYPE == FAKE_DATA_TYPE_TIMERS
  #include "debugTimer.h"
  debugTimer loopTimer;
  debugTimer sendTimer;
#endif


// slider LED vars
#define NUM_SLIDER_LEDS 32
CRGB sliderLeds[NUM_SLIDER_LEDS];

#define RGB_BRIGHTNESS 127
// 450mA should be unnoticeable up to ~140 brightness, and even then going brighter won't be too bad
#define RGB_POWER_LIMIT 450

#define MODE_LED_RGB_INDEX NUM_SLIDER_LEDS-1


// status/error LED state
enum errorState : byte {
  ERRORSTATE_NONE = 0, // no error, used for resetting state
  ERRORSTATE_SERIAL_TIMEOUT = 1, // not receiving serial data (timeout)
  ERRORSTATE_PACKET_CHECKSUM = 2, // at least one invalid packet (due to checksum error) was received
  ERRORSTATE_PACKET_OK = 4, // at least one valid packet was received
  ERRORSTATE_PACKET_MAX_REACHED = 8, // number of packets exceeded MAX_PACKETS_PER_LOOP
  ERRORSTATE_SERIAL_SEND_FAILURE = 16, // sendPacket returned false
  ERRORSTATE_MPR_STOPPED = 32, // an mpr121 stopped running
};
errorState operator |(errorState a, errorState b)
{
    return static_cast<errorState>(static_cast<byte>(a) | static_cast<byte>(b));
}
errorState operator |=(errorState &a, errorState b)
{
    return a = (a | b);
}

errorState curError;


// status/error LED pins
#ifdef LED_BUILTIN_RX
  #define STATUS_LED_BASIC_1_PIN LED_BUILTIN_RX // pro micro has no LED_BUILTIN, so use the RX led
  #define STATUS_LED_BASIC_1_USES_RXTX 1
#else
  #define STATUS_LED_BASIC_1_PIN LED_BUILTIN
#endif

#ifdef LED_BUILTIN_TX
  #define STATUS_LED_BASIC_2_PIN LED_BUILTIN_TX // pro micro has no LED_BUILTIN, so use the TX led
  #define STATUS_LED_BASIC_2_USES_RXTX 1
#else
  #define STATUS_LED_BASIC_2_PIN LED_BUILTIN
#endif

#define STATUS_LED_BASIC_1_ERRORS (ERRORSTATE_SERIAL_TIMEOUT | ERRORSTATE_PACKET_CHECKSUM | /*ERRORSTATE_PACKET_MAX_REACHED |*/ ERRORSTATE_SERIAL_SEND_FAILURE | ERRORSTATE_MPR_STOPPED)
#define STATUS_LED_BASIC_2_ERRORS (ERRORSTATE_PACKET_OK)


// slider serial protocol implementation
segaSlider sliderProtocol = segaSlider(&Serial);


// slider board type for remapping
sliderBoardType curSliderMode = SLIDER_TYPE_DIVA;
const sliderDef* curSliderDef = allSliderDefs[curSliderMode];


// slider protocol packets for reuse
byte sliderBuf[32];
sliderPacket scanPacket = { SLIDER_SCAN_REPORT, sliderBuf, 32, true };

byte emptyBytes[0];
sliderPacket emptyPacket = { (sliderCommand)0, emptyBytes, 0, true }; // command will be replaced as necessary

boardInfo boardInfoData;
sliderPacket boardinfoPacket = { SLIDER_BOARDINFO, (byte*)&boardInfoData, sizeof(boardInfo), true };


// mpr121s
#define NUM_MPRS 1
mpr121 mprs[NUM_MPRS] = { mpr121(0x5a) }; //, mpr121(0x5b), mpr121(0x5c), mpr121(0x5d) };


// air tower
airTower airTower({ { {PIN_AIRLED_1, PIN_AIRLED_1B}, {PIN_AIRLED_2, PIN_AIRLED_2B}, {PIN_AIRLED_3, PIN_AIRLED_3B} }, {PIN_AIRSENSOR_1, PIN_AIRSENSOR_2, PIN_AIRSENSOR_3, PIN_AIRSENSOR_4, PIN_AIRSENSOR_5, PIN_AIRSENSOR_6} });


// loop timing/processing stuff

// sleep for x microseconds after running the main loop
#define LOOP_SLEEP_US 333

// update air readings every x loops
// (air is less sensitive to timing so can be updated less often if necessary)
#define AIR_UPDATE_DUTY 2

// time to wait after losing serial connection before disabling scan and LEDs
#define SERIAL_TIMEOUT_MS 10000

// maximum number of packets to process in one loop
#define MAX_PACKETS_PER_LOOP 1


void setup() {
  // set pin modes for stuff that's handled in the main sketch file
  pinMode(STATUS_LED_BASIC_1_PIN, OUTPUT);
  pinMode(STATUS_LED_BASIC_2_PIN, OUTPUT);
  pinMode(PIN_MODESEL, INPUT_PULLUP);
  pinMode(PIN_SLIDER_IRQ, INPUT);

  // turn on led during setup
  // not gonna bother shifting this to the status lights
  digitalWrite(LED_BUILTIN, LOW);


  #if !FAKE_DATA || FAKE_DATA_DO_SCANNING
    for (mpr121 &mpr : mprs) {
      mpr.begin();
      mpr.ESI = MPR_ESI_1; // get 4ms response time (4 samples * 1ms rate)
      mpr.autoConfigUSL = 256L * (3200 - 700) / 3200; // set autoconfig for 3.2V
    }
  #endif // !FAKE_DATA || FAKE_DATA_DO_SCANNING

  
  // 5V is LED voltage, not arduino voltage
  FastLED.setMaxPowerInVoltsAndMilliamps(5, RGB_POWER_LIMIT); 
  
  // SK6812 should be WS2812(B) compatible, but FastLED has it natively anyway
  FastLED.addLeds<SK6812, PIN_SLIDER_LEDIN, GRB>(sliderLeds, NUM_SLIDER_LEDS);


  Serial.setTimeout(0);
  Serial.begin(115200);
  while(!Serial) {} // wait for serial to be ready on USB boards
}


bool scanOn = false;

// enable or disable slider and air scanning
void setScanning(bool on_off) {
  if (on_off && !scanOn) {
    scanOn = true;
    #if !FAKE_DATA || FAKE_DATA_DO_SCANNING
      for (mpr121 &mpr : mprs) {
        mpr.start(12);
      }
      airTower.calibrate();
      Keyboard.begin();
    #endif // !FAKE_DATA || FAKE_DATA_DO_SCANNING
  }
  else if (!on_off && scanOn) {
    scanOn = false;
    #if !FAKE_DATA || FAKE_DATA_DO_SCANNING
      for (mpr121 &mpr : mprs) {
        mpr.stop();
      }
      Keyboard.releaseAll();
      Keyboard.end();
    #endif // !FAKE_DATA || FAKE_DATA_DO_SCANNING
  }
}

// perform a slider scan and send it to sliderProtocol
void doSliderScan() {
  // clear the output buffer
  memset(sliderBuf, 0, sizeof(sliderBuf));
  
  #if FAKE_DATA
    #if FAKE_DATA_TYPE == FAKE_DATA_TYPE_CHASE
      sliderBuf[(millis()/30) % 32] = 0xC0;
    #elif FAKE_DATA_TYPE == FAKE_DATA_TYPE_PULSE
      sliderBuf[0] = (millis() % 1000) < 150 ? 0xC0 : 0x00;
    #elif FAKE_DATA_TYPE == FAKE_DATA_TYPE_TIMERS
      sendTimer.log();
      
      sliderBuf[0] = loopTimer.getMinMicros() / 1000;
      sliderBuf[1] = loopTimer.getAverageMicros() / 1000;
      sliderBuf[2] = loopTimer.getMaxMicros() / 1000;
      
      sliderBuf[4] = sendTimer.getMinMicros() / 1000;
      sliderBuf[5] = sendTimer.getAverageMicros() / 1000;
      sliderBuf[6] = sendTimer.getMaxMicros() / 1000;

      static unsigned long lastResetMillis;
      if (millis() - lastResetMillis > 3000) {
        sendTimer.reset();
        loopTimer.reset();
        lastResetMillis = millis();
      }
    #endif
  #endif // FAKE_DATA
  
  #if !FAKE_DATA || FAKE_DATA_DO_SCANNING
    static const byte numInputTouches = 12 * NUM_MPRS;
    static bool allTouches[numInputTouches];
  
    // read all mpr touches into allTouches
    for (byte i = 0; i < NUM_MPRS; i++) {
      mpr121 &mpr = mprs[i];

      // error condition, should hopefully never be triggered
      if (!mpr.checkRunning()) {
        curError |= ERRORSTATE_MPR_STOPPED;
        setScanning(false);
        return;
      }
      
      #if MPR121_USE_BITFIELDS
        short touches = mpr.readTouchState();
        for (byte j = 0; j < 12; j++) {
          allTouches[12*i + j] = bitRead(touches, j);
        }
      #else // MPR121_USE_BITFIELDS
        bool* touches = mpr.readTouchState();
        memcpy(&allTouches[12*i], touches, sizeof(bool[12]));
      #endif // MPR121_USE_BITFIELDS
    }

    #if !FAKE_DATA
      // apply touch data to output buffer
      for (byte i = 0; i < curSliderDef->keyCount && i < sizeof(sliderBuf); i++) { // for all keys, with bounds limited
        for (byte j = 0; j < curSliderDef->inputsPerKey; j++) { // for all inputs that may contribute
          byte inputPos = curSliderDef->keyMap[i][j];
    
          if (inputPos < numInputTouches && allTouches[inputPos]) { // check the result to read is in-range
            sliderBuf[i] |=  0xC0; // note this uses bitwise or to stack nicely
          }
        }
      }
    #endif // !FAKE_DATA
  #endif // !FAKE_DATA || FAKE_DATA_DO_SCANNING
  
  if (!sliderProtocol.sendPacket(scanPacket))
    curError |= ERRORSTATE_SERIAL_SEND_FAILURE;
}

// perform an air sensor scan and send it to Keyboard
void doAirScan() {
  #if !FAKE_DATA || FAKE_DATA_DO_SCANNING
    static const char airKeys[6] = {'/', '.', '\'', ';', ']', '['};
    Keyboard.releaseAll();
  
    for (byte i = 0; i < 6; i++) {
      if (airTower.checkLevel(i)) {
        #if !FAKE_DATA
          Keyboard.press(airKeys[i]);
        #endif // !FAKE_DATA
      }
    }
  #endif // !FAKE_DATA || FAKE_DATA_DO_SCANNING
}


// timing vars used in main loop
int sleepTime = LOOP_SLEEP_US;
unsigned long lastSliderSendMillis;
unsigned long lastSerialRecvMillis = -SERIAL_TIMEOUT_MS;
unsigned long loopCount = 0;

bool ledUpdate = false;

void loop() {
  // clear errors (they'll be reset if necessary)
  curError = ERRORSTATE_NONE;

  //curSliderMode = (digitalRead(PIN_MODESEL) == LOW) ? SLIDER_TYPE_DIVA : SLIDER_TYPE_CHUNI;
  curSliderDef = allSliderDefs[curSliderMode];

  // check for new slider data
  byte pktCount = 0;
  while (pktCount < MAX_PACKETS_PER_LOOP) {
    sliderPacket pkt = sliderProtocol.getPacket();

    // if there was no data or the buffer was incomplete, `Command` will equal `(sliderCommand)0`
    // there's no point in looping here if there's no data or not enough data
    if (pkt.Command == (sliderCommand)0) {
      break;
    }

    // increment these when there's any packets, valid or not
    lastSerialRecvMillis = millis();
    pktCount++;

    // check if the packet checksum was valid, and skip handling the packet if it wasn't
    if (pkt.IsValid) {
      curError |= ERRORSTATE_PACKET_OK;
    }
    else {
      curError |= ERRORSTATE_PACKET_CHECKSUM;
      continue;
    }

    switch(pkt.Command) {
      case SLIDER_BOARDINFO:
        memcpy(boardInfoData.model, curSliderDef->model, sizeof(boardInfo::model));
        memcpy(boardInfoData.chipNumber, curSliderDef->chipNumber, sizeof(boardInfo::chipNumber));
        if (!sliderProtocol.sendPacket(boardinfoPacket))
          curError |= ERRORSTATE_SERIAL_SEND_FAILURE;
        break;

      case SLIDER_SCAN_REPORT:
        // there's no way this will give accurate results,
        // but at least it's implemented on a protocol level now
        if (!scanOn) {
          setScanning(true);
          delay(10);
          doSliderScan();
          setScanning(false);
        }
        break; // doSliderScan() sends the response
        
      case SLIDER_SCAN_ON:
        sliderProtocol.sendPacket(scanPacket);
        setScanning(true);
        break; // no response needed
        
      case SLIDER_SCAN_OFF:
        setScanning(false);
        emptyPacket.Command = SLIDER_SCAN_OFF;
        if (!sliderProtocol.sendPacket(emptyPacket))
          curError |= ERRORSTATE_SERIAL_SEND_FAILURE;
        break;
        
      case SLIDER_LED:
        if (pkt.DataLength > 0) {
          FastLED.setBrightness((pkt.Data[0] & 0x3f) * RGB_BRIGHTNESS / 0x3f); // this seems to max out at 0x3f (63), use that for division

          byte maxPacketLeds = (pkt.DataLength - 1) / 3; // subtract 1 because of brightness byte
          
          for (byte i = 0; i < curSliderDef->ledCount; i++) {
            byte outputLed = curSliderDef->ledMap[i];

            if (outputLed < NUM_SLIDER_LEDS) { // make sure there's no out of bounds writes
              if (i < maxPacketLeds) {
                if (outputLed % 2 == 0) {
                  sliderLeds[outputLed].b = pkt.Data[i*3 + 1]; // start with + 1 because of brightness byte
                  sliderLeds[outputLed].r = pkt.Data[i*3 + 2];
                  sliderLeds[outputLed].g = pkt.Data[i*3 + 3];
                }
                else {
                  // dim values for separators
                  sliderLeds[outputLed].b = pkt.Data[i*3 + 1] / 2; // start with + 1 because of brightness byte
                  sliderLeds[outputLed].r = pkt.Data[i*3 + 2] / 2;
                  sliderLeds[outputLed].g = pkt.Data[i*3 + 3] / 2;
                }
              }
              else {
                sliderLeds[outputLed] = CRGB::Black;
              }
            }
          }
          ledUpdate = true;
        }
        break; // no response needed
        
      default:
        emptyPacket.Command = pkt.Command; // just blindly acknowledge unknown commands
        if (!sliderProtocol.sendPacket(emptyPacket))
          curError |= ERRORSTATE_SERIAL_SEND_FAILURE;
        break;
    }
  }
  if (pktCount == MAX_PACKETS_PER_LOOP)
    curError |= ERRORSTATE_PACKET_MAX_REACHED;

  // disable scan and set error if serial is dead
  if ((millis() - lastSerialRecvMillis) > SERIAL_TIMEOUT_MS) {
    if (scanOn) {
      setScanning(false);
    }
    curError |= ERRORSTATE_SERIAL_TIMEOUT;

    
    // set mode colour for 5s (this should trigger at boot)
    // also use this to black out slider
    if ((millis() - lastSerialRecvMillis) < SERIAL_TIMEOUT_MS + 5000) {
      FastLED.setBrightness(RGB_BRIGHTNESS);

      for (byte i = 0; i < NUM_SLIDER_LEDS; i++) {
        sliderLeds[i] = CRGB::Black;
      }
      
      switch(curSliderMode) {
        case SLIDER_TYPE_DIVA:
          sliderLeds[MODE_LED_RGB_INDEX] = CRGB::Teal;
          break;
        default:
          sliderLeds[MODE_LED_RGB_INDEX] = CRGB::Olive;
          break;
      }

      ledUpdate = true;
    }
    else {
      if (sliderLeds[MODE_LED_RGB_INDEX] != (CRGB)CRGB::Black) {
        sliderLeds[MODE_LED_RGB_INDEX] = CRGB::Black;
        ledUpdate = true;
      }
    }
  }

  // now update LEDs once per loop
  // done before scanning to reduce delay from receiving to showing
  if (ledUpdate) {
    FastLED.show();
    ledUpdate = false;
  }

  // if slider scanning is on, update inputs (as appropriate)
  if (scanOn) {
    
    // if slider touch state has changed (interrupt was triggered), send data
    // also send as a keep alive if slider touch state hasn't changed recently
    if ( /*(digitalRead(PIN_SLIDER_IRQ) == LOW) ||*/ ((millis() - lastSliderSendMillis) > 10) ) {
      lastSliderSendMillis = millis();
      doSliderScan();
    }

    if (curSliderDef->hasAir) {
      // if air should be updated this loop, update it
      if (loopCount % AIR_UPDATE_DUTY == 0) {
        doAirScan();
      }
    }
  }


  // set status leds
  if (STATUS_LED_BASIC_1_ERRORS & curError) {
    #ifdef STATUS_LED_BASIC_1_USES_RXTX
      pinMode(STATUS_LED_BASIC_1_PIN, OUTPUT); // set as OUTPUT to re-enable normal operation
    #endif
    digitalWrite(STATUS_LED_BASIC_1_PIN, LOW);
  }
  else {
    #ifdef STATUS_LED_BASIC_1_USES_RXTX
      pinMode(STATUS_LED_BASIC_1_PIN, INPUT); // set as INPUT to disable normal operation
    #endif
    digitalWrite(STATUS_LED_BASIC_1_PIN, HIGH);
  }

  if (STATUS_LED_BASIC_2_ERRORS & curError) {
    #ifdef STATUS_LED_BASIC_2_USES_RXTX
      pinMode(STATUS_LED_BASIC_2_PIN, OUTPUT); // set as OUTPUT to re-enable normal operation
    #endif
    digitalWrite(STATUS_LED_BASIC_2_PIN, LOW);
  }
  else {
    #ifdef STATUS_LED_BASIC_2_USES_RXTX
      pinMode(STATUS_LED_BASIC_2_PIN, INPUT); // set as INPUT to disable normal operation
    #endif
    digitalWrite(STATUS_LED_BASIC_2_PIN, HIGH);
  }

  loopCount++;

  #if FAKE_DATA && FAKE_DATA_TYPE == FAKE_DATA_TYPE_TIMERS
    loopTimer.log();
  #endif

  delayMicroseconds(sleepTime);
}
