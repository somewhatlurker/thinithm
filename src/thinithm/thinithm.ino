#include "segaSlider.h"
#include "sliderdefs.h"
#include "mpr121.h"
#include "pins.h"
#include <FastLED.h>

#define NUM_SLIDER_LEDS 32
CRGB sliderLeds[NUM_SLIDER_LEDS];

#define RGB_BRIGHTNESS 127

enum errorState : byte {
  ERRORSTATE_NONE = 0,
  ERRORSTATE_SERIAL_RX = 1,
  ERRORSTATE_PACKET_CHECKSUM = 2,
  ERRORSTATE_PACKET_OK = 4,
};
errorState operator |(errorState a, errorState b)
{
    return static_cast<errorState>(static_cast<int>(a) | static_cast<int>(b));
}
errorState operator |=(errorState &a, errorState b)
{
    return a = (a | b);
}

errorState curError;

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

#define STATUS_LED_BASIC_1_ERRORS (ERRORSTATE_SERIAL_RX | ERRORSTATE_PACKET_CHECKSUM)
#define STATUS_LED_BASIC_2_ERRORS (ERRORSTATE_PACKET_OK)


segaSlider sliderProtocol = segaSlider(&Serial);

sliderBoardType curSliderMode;

byte sliderBuf[32];
sliderPacket scanPacket;

byte emptyBytes[0];
sliderPacket emptyPacket;

// 8 byte model number (string with spaces for padding), 1 byte device class, 5 byte chip part number (string), 1 byte unknown, 1 byte fw version, 2 bytes unknown
// board "15275   ", class 0x0a, chip "06687", 0xff, ver 144, 0x00, 0x64
byte boardInfoDataDiva[18] = {0x31, 0x35, 0x32, 0x37, 0x35, 0x20, 0x20, 0x20, 0xa0, 0x30, 0x36, 0x36, 0x38, 0x37, 0xFF, 0x90, 0x00, 0x64};
// board "15330   ", class 0x0a, chip "06712", 0xff, ver 144, 0x00, 0x64
byte boardInfoDataChuni[18] = {0x31, 0x35, 0x33, 0x33, 0x30, 0x20, 0x20, 0x20, 0xa0, 0x30, 0x36, 0x37, 0x31, 0x32, 0xFF, 0x90, 0x00, 0x64};
sliderPacket boardinfoPacket;

//#define NUM_MPRS 4
//mpr121 mprs[NUM_MPRS] = { mpr121(0x5a), mpr121(0x5b), mpr121(0x5c), mpr121(0x5d) };

void setup() {
  pinMode(STATUS_LED_BASIC_1_PIN, OUTPUT);
  pinMode(STATUS_LED_BASIC_2_PIN, OUTPUT);
  pinMode(PIN_MODESEL, INPUT_PULLUP);
  pinMode(PIN_SLIDER_IRQ, INPUT);
  
  digitalWrite(LED_BUILTIN, LOW);
  
  scanPacket.Command = SLIDER_SCAN_REPORT;
  scanPacket.Data = sliderBuf;
  scanPacket.DataLength = 32;
  scanPacket.IsValid = true;

  emptyPacket.Command = (sliderCommand)0; // this will be replaced as necessary
  emptyPacket.Data = emptyBytes;
  emptyPacket.DataLength = 0;
  emptyPacket.IsValid = true;

  boardinfoPacket.Command = SLIDER_BOARDINFO;
  //boardinfoPacket.Data = boardInfoData;
  boardinfoPacket.DataLength = 18;
  boardinfoPacket.IsValid = true;

  //Wire.begin();
  //Wire.setClock(400000); // mpr121 can run in fast mode. if you have issues, try removing this line
  //for (mpr121 &mpr : mprs) {
  //  mpr.ESI = MPR_ESI_1; // get 4ms response time (4 samples * 1ms rate)
  //  mpr.autoConfigUSL = 256 * (3200 - 700) / 3200; // set autoconfig for 3.2V
  //}

  // SK6812 should be WS2812(B) compatible, but FastLED has it natively anyway
  FastLED.addLeds<SK6812, PIN_SLIDER_LEDIN, GRB>(sliderLeds, NUM_SLIDER_LEDS);

  Serial.setTimeout(0.01);
  Serial.begin(115200);
}

int curSliderPatternByte;
bool scanOn = false;
int sleepTime = 1;
unsigned long lastSliderSendMillis;
unsigned long lastSerialRecvMillis;

void loop() {
  curError = ERRORSTATE_NONE;
  curSliderMode = (digitalRead(PIN_MODESEL) == LOW) ? SLIDER_TYPE_DIVA : SLIDER_TYPE_CHUNI;
  
  if (sliderProtocol.readSerial()) {
    lastSerialRecvMillis = millis();
    
    // while packets can be read, process them
    sliderPacket pkt;
    while (true) {
      pkt = sliderProtocol.readNextPacket();
      if (!pkt.IsValid) {
        if (pkt.Command != (sliderCommand)0) // if `Command == (sliderCommand)0` it was probably caused by end of buffer and not corruption
          curError |= ERRORSTATE_PACKET_CHECKSUM;
        break;
      }

      curError |= ERRORSTATE_PACKET_OK;
      
      switch(pkt.Command) {
        case SLIDER_BOARDINFO:
          switch(curSliderMode) {
            case SLIDER_TYPE_DIVA:
              boardinfoPacket.Data = boardInfoDataDiva;
              break;
            default:
              boardinfoPacket.Data = boardInfoDataChuni;
              break;
          }
          sliderProtocol.sendSliderPacket(boardinfoPacket);
          break;
          
        case SLIDER_SCAN_ON:
          scanOn = true;
          //for (mpr121 &mpr : mprs) {
          //  mpr.startMPR(12);
          //}
          break; // no response needed
          
        case SLIDER_SCAN_OFF:
          scanOn = false;
          //for (mpr121 &mpr : mprs) {
          //  mpr.stopMPR();
          //}
          emptyPacket.Command = SLIDER_SCAN_OFF;
          sliderProtocol.sendSliderPacket(emptyPacket);
          break;
          
        case SLIDER_LED:
          if (pkt.DataLength > 0) {
            FastLED.setBrightness(pkt.Data[0] * RGB_BRIGHTNESS / 63); // this seems to max out at 0x3f (63), use that for division

            int maxPacketLeds = (pkt.DataLength - 1) / 3; // subtract 1 because of brightness byte
            
            for (int i = 0; i < allSliderDefs[curSliderMode]->ledCount; i++) {
              int outputLed = allSliderDefs[curSliderMode]->ledMap[i];

              if (outputLed < NUM_SLIDER_LEDS) { // make sure there's no out of bounds writes
                if (i < maxPacketLeds) {
                  sliderLeds[outputLed].b = pkt.Data[i*3 + 1]; // start with + 1 because of brightness byte
                  sliderLeds[outputLed].r = pkt.Data[i*3 + 2];
                  sliderLeds[outputLed].g = pkt.Data[i*3 + 3];
                }
                else {
                  sliderLeds[outputLed] = CRGB::Black;
                }
              }
            }
            FastLED.show();
          }
          break; // no response needed
          
        default:
          emptyPacket.Command = pkt.Command; // just blindly acknowledge unknown commands
          sliderProtocol.sendSliderPacket(emptyPacket);
      }
    }
  }

  if ((millis() - lastSerialRecvMillis) > 10000) { // disable scan and set error if serial is dead
    scanOn = false;
    //for (mpr121 &mpr : mprs) {
    //  mpr.stopMPR();
    //}
    curError |= ERRORSTATE_SERIAL_RX;
  }

  if ( scanOn &&
       ( (digitalRead(PIN_SLIDER_IRQ) == LOW) ||
         ((millis() - lastSliderSendMillis) > 250) ) // if no interrupt recently, send a keep alive
     )
  {
    //sliderBuf[0] = (millis() % 1000) < 150 ? 0xC0 : 0x00;
    curSliderPatternByte = (millis()/30) % 32;
    sliderBuf[curSliderPatternByte] = 0xC0;

    /*
    // clear the output buffer
    memset(sliderBuf, 0, sizeof(sliderBuf));

    static const int numInputTouches = 12 * NUM_MPRS;
    static bool allTouches[numInputTouches];

    // read all mpr touches into allTouches
    for (int i = 0; i < NUM_MPRS; i++) {
      mpr121 &mpr = mprs[i];
      bool* touches = mpr.readTouchState();
      memcpy(&allTouches[12*i], touches, sizeof(bool[12]));
    }

    // apply touch data to output buffer
    for (int i = 0; i < allSliderDefs[curSliderMode]->keyCount && i < sizeof(sliderBuf); i++) { // for all keys, with bounds limited
      for (int j = 0; j < SLIDER_BOARDS_INPUT_KEYS_PER_OUTPUT; j++) { // for all inputs that may contribute
        int inputPos = allSliderDefs[curSliderMode]->keyMap[i][j];
  
        if (inputPos < numInputTouches && allTouches[inputPos]) { // check the result to read is in-range
          sliderBuf[i] |=  0xC0; // note this uses bitwise or to stack nicely
        }
      }
    }
    */
    
    sliderProtocol.sendSliderPacket(scanPacket);
    
    sliderBuf[curSliderPatternByte] = 0x00;
    
    lastSliderSendMillis = millis();
  }

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
  
  delay(sleepTime);
}
