/*
  ==TFT Software SPI connection ==
    OLED   =>    Arduino
   1. GND    ->    GND
   2. VCC    ->    3.3
   3. SCL    ->    13
   4. SDA    ->    11
   5. RES    ->    8
   6. DC     ->    9
   7. CS     ->    10
   8. BLK     ->    NC
*/


/*
  ==TFT Software SPI connection ==
    OLED   =>    Arduino
   1. GND    ->    GND
   2. VCC    ->    3.3
   3. SCL    ->    13
   4. SDA    ->    11
   5. RES    ->    8
   6. DC     ->    9
   7. CS     ->    10
   8. BLK     ->    NC
*/
#include "SPI.h"
#include "ERGFX.h"
#include "TFTM1.44-2.h"
#include <Wire.h>
#include "MAX30100.h"                                                     // Libraries required for the
#include "MAX30100_PulseOximeter.h"
#include <Adafruit_Fingerprint.h>
#include "bitmaps.h"
#include <ArduinoJson.h>

#define TFT_RST 9
#define TFT_DC 8
#define TFT_CS 10
// Recommended settings for the MAX30100, DO NOT CHANGE!!!!,  refer to the datasheet for further info
#define SAMPLING_RATE                       MAX30100_SAMPRATE_100HZ       // Max sample rate
#define IR_LED_CURRENT                      MAX30100_LED_CURR_50MA        // The LEDs currents must be set to a level that 
#define RED_LED_CURRENT                     MAX30100_LED_CURR_27_1MA      // avoids clipping and maximises the dynamic range
#define PULSE_WIDTH                         MAX30100_SPC_PW_1600US_16BITS // The pulse width of the LEDs driving determines
#define HIGHRES_MODE                        true                          // the resolution of the ADC


// Create objects for the raw data from the sensor (used to make the trace) and the pulse and oxygen levels
MAX30100 sensor;            // Raw Data
PulseOximeter pox;          // Pulse and Oxygen

// The following settings adjust various factors of the display
#define SCALING 12                                                  // Scale height of trace, reduce value to make trace height
// bigger, increase to make smaller
#define TRACE_SPEED 0.5                                             // Speed of trace across screen, higher=faster   
#define TRACE_MIDDLE_Y_POSITION 41                                  // y pos on screen of approx middle of trace
#define TRACE_HEIGHT 64                                             // Max height of trace in pixels    
#define HALF_TRACE_HEIGHT TRACE_HEIGHT/2                            // half Max height of trace in pixels (the trace amplitude)    
#define TRACE_MIN_Y TRACE_MIDDLE_Y_POSITION-HALF_TRACE_HEIGHT+1     // Min Y pos of trace, calculated from above values
#define TRACE_MAX_Y TRACE_MIDDLE_Y_POSITION+HALF_TRACE_HEIGHT-1     // Max Y pos of trace, calculated from above values
// If using the Software SPI
#define TFT_CLK  13
#define TFT_MOSI 11
ST7735 tft = ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST);


int PersonID;
      uint8_t BPM, O2;                            // BPM and O2 values
// Note that as sensors may be slightly different the code adjusts this
// on the fly if the trace is off screen. The default was determined
// By analysis of the raw data returned
int i = 0;
// Color definitions
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

int ThermistorPin = 0;
int Vo;
float R1 = 10000;
float logR2, R2, T, Tc, Tf;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;



Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial1);
int fingerprintID = 0;

void onBeatDetected()
{
  digitalWrite(3, HIGH); // beep the beeper
  digitalWrite(3, LOW);
}

void setup()
{
  Serial.begin(115200);
  Serial.print("Initializing pulse oximeter..");
  finger.begin(57600);
  // Initialize the sensor. Failures are generally due to an improper I2C wiring, missing power supply
  // or wrong target chip. Occasionally fails on startup (very rare), just press reset on Arduino
  if (!pox.begin()) {
    Serial.println("FAILED");
    for (;;);
  } else {
    Serial.println("SUCCESS");
  }
  pox.setOnBeatDetectedCallback(onBeatDetected);
  tft.begin();
  //  tft.initR(INITR_144GREENTAB);   // initialize a ST7735S chip, for 128x128 display
  tft.fillScreen(ST7735_WHITE);
  drawlogo();
  tft.fillScreen(ST7735_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ST7735_WHITE);
  startFingerprintSensor();
  displayLockScreen();
  // Set up the parameters for the raw data object
  sensor.setMode(MAX30100_MODE_SPO2_HR);
  sensor.setLedsCurrent(IR_LED_CURRENT, RED_LED_CURRENT);
  sensor.setLedsPulseWidth(PULSE_WIDTH);
  sensor.setSamplingRate(SAMPLING_RATE);
  sensor.setHighresModeEnabled(HIGHRES_MODE);




}

void loop()
{

  fingerprintID = getFingerprintID();
  delay(50);
  if (fingerprintID == 1)
  {
    tft.drawBitmap(30, 35, icon, 60, 60, GREEN);
    delay(2000);
    displayUnlockedScreen();
    delay(2000);
    tft.fillScreen(ST7735_BLACK);
    // Display BPM and O2 titles, these remain on screen, we only erase the trace and the
    // BPM/O2 results, otherwise we can get some flicker
    tft.setTextSize(2);
    tft.setCursor(0, 86);
    tft.print("BPM   O");
    tft.setCursor(92, 86);
    tft.print("%");
    tft.setTextSize(1);
    tft.setCursor(84, 94);
    tft.print("2");                            // The small subscriper 2 of O2
    tft.setCursor(1, 0);
    tft.print("Remote Doctor");
    tft.setTextSize(2);
    tft.drawRect(0, TRACE_MIN_Y - 1, 128, TRACE_HEIGHT + 2, ST7735_BLUE); // The border box for the trace

    for (i = 0 ; i < 1000; i++)
    {



      int16_t Diff = 0;                           // The difference between the Infra Red (IR) and Red LED raw results
      uint16_t ir, red;                           // raw results returned in these
      static float lastx = 1;                     // Last x position of trace
      static int lasty = TRACE_MIDDLE_Y_POSITION; // Last y position of trace, default to middle
      static float x = 1;                         // current x position of trace
      int32_t y;                                  // current y position of trace

      static uint32_t tsLastReport = 0;           // Last time BMP/O2 were checked
      static int32_t SensorOffset = 10000;        // Offset to lowest point that raw data does not go below, default 10000


      pox.update();
      sensor.update();
      if (millis() - tsLastReport > 500)                           // If more than 1 second (1000milliseconds) has past
      { // since getting heart rate and O2 then get some bew values
        tft.fillRect(0, 104, 128, 16, ST7735_BLACK);                // Clear the old values
        BPM = round(pox.getHeartRate());                            // Get BPM
        if ((BPM < 60) | (BPM > 110))                               // If too low or high for a resting heart rate then display in red
          tft.setTextColor(ST7735_RED);
        else
          tft.setTextColor(ST7735_GREEN);                           // else display in green
        tft.setCursor(0, 104);                                      // Put BPM at this position
        tft.print(BPM);                                             // print BPM to screen
        O2 = pox.getSpO2();                                         // Get the O2
        if (O2 < 94)                                                // If too low then display in red
          tft.setTextColor(ST7735_RED);
        else
          tft.setTextColor(ST7735_GREEN);                           // else green
        tft.setCursor(72, 104);                                     // Set print position for the O2 value
        tft.print(O2);
        Serial.print("Heart rate:");
        Serial.print(pox.getHeartRate());
        Serial.print("bpm / SpO2:");
        Serial.print(pox.getSpO2());
        Serial.println("%");// print it to screen
        tsLastReport = millis();                                    // Set the last time values got to current time
      }


 /*     // request raw data from sensor
      if (sensor.getRawValues(&ir, &red))         // If raw data available for IR and Red
      {
        if (red < 1000)                           // No pulse
          y = TRACE_MIDDLE_Y_POSITION;            // Set Y to default flat line in middle
        else
        {
          // Plot our new point
          Diff = (ir - red);                      // Get raw difference between the 2 LEDS
          Diff = Diff - SensorOffset;             // Adjust the baseline of raw values by removing the offset (moves into a good range for scaling)
          Diff = Diff / SCALING;                  // Scale the difference so that it appears at a good height on screen

          // If the Min or max are off screen then we need to alter the SensorOffset, this should bring it nicely on screen
          if (Diff < -HALF_TRACE_HEIGHT)
            SensorOffset += (SCALING * (abs(Diff) - 32));
          if (Diff > HALF_TRACE_HEIGHT)
            SensorOffset += (SCALING * (abs(Diff) - 32));

          y = Diff + (TRACE_MIDDLE_Y_POSITION - HALF_TRACE_HEIGHT);   // These two lines move Y pos of trace to approx middle of display area
          y += TRACE_HEIGHT / 4;
        }

        if (y > TRACE_MAX_Y) y = TRACE_MAX_Y;                         // If going beyond trace box area then crop the trace
        if (y < TRACE_MIN_Y) y = TRACE_MIN_Y;                         // so it stays within
        tft.drawLine(lastx, lasty, x, y, ST7735_YELLOW);              // Plot the next part of the trace
        lasty = y;                                                    // Save where the last Y pos was
        lastx = x;                                                    // Save where the last X pos was
        x += TRACE_SPEED;                                             // Move trace along the display
        if (x > 126)                                                  // If reached end of display then reset to statt
        {
          tft.fillRect(1, TRACE_MIN_Y, 126, TRACE_HEIGHT, ST7735_BLACK); // Blank trace display area
          x = 1;                                                      // Back to start
          lastx = x;
        }
        // Request pulse and o2 data from sensor




      }*/
    }

    Vo = analogRead(ThermistorPin);
    R2 = R1 * (1023.0 / (float)Vo - 1.0);
    logR2 = log(R2);
    T = (1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2));
    Tc = T - 273.15;
    Tf = (Tc * 9.0) / 5.0 + 32.0;

    Serial.print("Temperature: ");
    Serial.print(Tf);
    Serial.print(" F; ");
    Serial.print(Tc);
    Serial.println(" C");


    tft.fillScreen(BLACK);
    tft.drawRect(0, 0, 128, 128, WHITE);

    tft.setCursor(0, 10);
    tft.setTextColor(RED);
    tft.setTextSize(2);
    tft.print("TEMP *F");

    tft.setCursor(5, 50);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.print(Tf);
   
    delay(2000);
    // Send to LoRa
    StaticJsonDocument<200> doc2;
    doc2["member_id"] = PersonID;
    doc2["temperature"] = Tf;
    doc2["Hear_rate"] = BPM;
    doc2["SPO2"] = O2;
    serializeJson(doc2, Serial2);

    i = 0;
    tft.fillScreen(BLACK);

  }
      displayLockScreen();
  if (fingerprintID == 3)
    accessdenied();



}
void displayLockScreen()
{
  tft.drawRect(0, 0, 128, 128, WHITE);
  tft.setCursor(5, 10);
  tft.setTextColor(RED);
  tft.setTextSize(2);
  tft.print("TELEVAIDYA");

  tft.setCursor(10, 100);
  tft.setTextColor(GREEN);
  tft.setTextSize(1);
  tft.print("Waiting for valid \n    fingerprint.");

  tft.drawBitmap(30, 35, icon, 60, 60, WHITE);
}
void startFingerprintSensor()
{

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor");
  }
  Serial.println("Waiting for valid finger...");
}

int getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}

void accessdenied()
{
  tft.fillScreen(BLACK);
  tft.drawRect(0, 0, 128, 128, WHITE);
  tft.setCursor(30, 10);
  tft.setTextColor(RED);
  tft.setTextSize(2);
  tft.print("ACCESS    DENIED");

  tft.setCursor(10, 100);
  tft.setTextColor(GREEN);
  tft.setTextSize(1);
  tft.print("Waiting for valid \n    fingerprint.");

  tft.drawBitmap(30, 35, icon, 60, 60, WHITE);

}


void displayUnlockedScreen()
{
  tft.fillScreen(BLACK);
  tft.drawRect(0, 0, 128, 128, WHITE);

  tft.setCursor(18, 10);
  tft.setTextColor(GREEN);
  tft.setTextSize(2);
  tft.print("UNLOCKED");

  tft.setCursor(5, 50);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.print("TELEVAIDYA");

}
void drawlogo()
{
  tft.drawBitmap(10, 35, microchip, 101, 63, RED);
  delay(500);
  tft.fillScreen(BLACK);
}
