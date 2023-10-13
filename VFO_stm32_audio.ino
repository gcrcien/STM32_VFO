#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

#define TFT_CS    PA4
#define TFT_CLK   PA5
#define TFT_MISO  PA6
#define TFT_MOSI  PA7
#define TFT_RST   PB0
#define TFT_DC    PB1
#define AUDIO_IN  PA0

#define SCALE_X_OFFSET 20
#define AUDIO_BAR_Y_OFFSET 70 // Ajusta la posición vertical de la barra de audio
#define SCALE_WIDTH 200
#define SCALE_HEIGHT 20
#define NUM_SEGMENTS 8

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

long currentFrequency = 27000000;
bool change = false;
int segment = 0; // Variable para el número de segmentos
String oldFrequency_string;
unsigned long lastAudioUpdate = 0; // Última actualización de la barra de audio
unsigned long audioUpdateInterval = 50; // Intervalo de actualización en milisegundos (10 veces por segundo)
String shz;
String skhz;
String smhz;
String frequency_string;

void setup() {
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  pinMode(PB12, INPUT_PULLUP);
  pinMode(PB13, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PB12), clock1_ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(PB13), clock1_ISR2, FALLING);
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));

  drawScale();
}

void clock1_ISR() {
  bool dir = digitalRead(PB13);
  if (dir == true) {
    currentFrequency += 10;
  }
  if (dir == false) {
    currentFrequency -= 10;
  }
  change = true;
}

void clock1_ISR2() {
  bool dir = digitalRead(PB12);
  if (dir == true) {
    currentFrequency -= 10;
  }
  if (dir == false) {
    currentFrequency += 10;
  }
  change = true;
}

void drawScale() {
  tft.drawRect(SCALE_X_OFFSET - 2, AUDIO_BAR_Y_OFFSET - 2, SCALE_WIDTH + 4, SCALE_HEIGHT + 4, ILI9341_WHITE);
  int segmentWidth = SCALE_WIDTH / NUM_SEGMENTS;
  int scaleValue = 0;
  tft.drawRect(SCALE_X_OFFSET, AUDIO_BAR_Y_OFFSET, SCALE_WIDTH, SCALE_HEIGHT, ILI9341_WHITE);
  for (int i = 0; i <= NUM_SEGMENTS; i++) {
    int x = SCALE_X_OFFSET + i * segmentWidth;
    tft.drawFastVLine(x, AUDIO_BAR_Y_OFFSET - 5, SCALE_HEIGHT + 10, ILI9341_WHITE);
    tft.setCursor(x - 10, AUDIO_BAR_Y_OFFSET + SCALE_HEIGHT + 5);
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_WHITE);
    tft.print("S" + String(scaleValue));
    scaleValue += 9 / NUM_SEGMENTS;
  }
}

void loop() {
  if (change == true) {
    clock_update();
  }

  unsigned long currentMillis = millis();
  if (currentMillis - lastAudioUpdate >= audioUpdateInterval) {
    audio_peek(); // Actualiza la barra de medición de audio
    lastAudioUpdate = currentMillis;
  }
}

void clock_update() {
  f_string();

  // tft.fillRect(1, 30, 290, 28, ILI9341_BLACK);
  tft.setCursor(20, 30);
  tft.setTextSize(4);
  tft.setTextColor(ILI9341_BLACK);
  tft.print(oldFrequency_string);
  tft.setCursor(20, 30);
  tft.setTextSize(4);
  tft.setTextColor(ILI9341_GREEN);
  tft.print(frequency_string);
  oldFrequency_string = frequency_string;
  change = false;
}

void audio_peek() {
  int audioValue = analogRead(AUDIO_IN);
  int newSegment = audioValue / (4095 / NUM_SEGMENTS);

  if (newSegment != segment) {
    tft.fillRect(SCALE_X_OFFSET, AUDIO_BAR_Y_OFFSET, SCALE_WIDTH, SCALE_HEIGHT, ILI9341_BLACK);

    int segmentWidth = SCALE_WIDTH / NUM_SEGMENTS;
    segment = newSegment;

    for (int i = 0; i <= segment; i++) {
      int x = SCALE_X_OFFSET + i * segmentWidth;
      if (i >= NUM_SEGMENTS - 2) {
        tft.fillRect(x, AUDIO_BAR_Y_OFFSET, segmentWidth, SCALE_HEIGHT, ILI9341_RED);
      } else if (i == 6 || i == 7) {
        tft.fillRect(x, AUDIO_BAR_Y_OFFSET, segmentWidth, SCALE_HEIGHT, ILI9341_YELLOW);
      } else {
        tft.fillRect(x, AUDIO_BAR_Y_OFFSET, segmentWidth, SCALE_HEIGHT, ILI9341_GREEN);
      }
    }
  }
}

void f_string() {

  int mhz = currentFrequency / 1000000;
  int khz = (currentFrequency % 1000000) / 1000;
  int hz = currentFrequency - ((mhz * 1000000) + (khz * 1000));
  // skhz = String(khz);
  smhz = String(mhz);
  if (mhz < 10 && mhz >= 1) {
    smhz = " " + String(mhz);
  }
  //##############  HZ
  if (hz >= 100) {
    shz = "," + String(hz);
  }
  if (hz < 100 && hz > 10) {
    shz = ",0" + String(hz);
  }
  if (hz < 10 && hz >= 1) {
    shz = ",00" + String(hz);
  }
  if (hz == 0) {
    shz = ",000//";
  }
  //#################### KHZ

  if (khz >= 100) {
    skhz = "," + String(khz);
  }
  if (khz < 100 && khz > 10) {
    skhz = ",0" + String(khz);
  }
  if (khz < 10 and khz >= 1) {
    skhz = ",00" + String(khz);
  }
  if (khz == 0) {
    skhz = ",000";
  }

  frequency_string = smhz + skhz + shz;
}
