#define NUM_MPRS 3 // must be using sequential addresses
#define ANALOG_OUTPUT false

#include "mpr121.h"

mpr121 mprs[NUM_MPRS] = NULL;

void setup() {
  Wire.begin();
  Wire.setClock(400000); // mpr121 can run in fast mode. if you have issues, try removing this line
  for (int i = 0; i < NUM_MPRS; i++) {
    mpr121 &mpr = mprs[i];
    mpr = mpr121(0x5a + i);
    mpr.ESI = MPR_ESI_1; // get 4ms response time (4 samples * 1ms rate)
    mpr.autoConfigUSL = 256 * (3200 - 700) / 3200; // set autoconfig for 3.2V
    mpr.startMPR(12);
  }

  Serial.begin(115200);
}

void loop() {
  for (int i = 0; i < NUM_MPRS; i++) {
    mpr121 &mpr = mprs[i];
    if (ANALOG_OUTPUT) {
      short* values = mpr.readElectrodeData(0, 12); // read 12 electrodes, starting from electrode 0
      for (int j = 0; j < 12; j++)
        Serial.print(values[j]);
        Serial.print(" ");
    }
    else {
      bool* touches = mpr.readTouchState();
      for (int j = 0; j < 12; j++)
        Serial.print(touches[j]);
    }
    Serial.print(" ");
  }
  Serial.println();
  delay(10);
}