#include "SPI.h"
#include "Adafruit_ILI9341.h"
#include "Wire.h"
#include "I2CKeyPad.h"

#define TFT_CS    PA4
#define TFT_CLK   PA5
#define TFT_MISO  PA6
#define TFT_MOSI  PA7
#define TFT_RST   PB0
#define TFT_DC    PB1
#define AUDIO_IN  PA0

#define SCALE_X_OFFSET 20
#define AUDIO_BAR_Y_OFFSET 70
#define SCALE_WIDTH 200
#define SCALE_HEIGHT 20
#define NUM_SEGMENTS 9

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

const uint8_t KEYPAD_ADDRESS = 0x20;
I2CKeyPad keyPad(KEYPAD_ADDRESS);

char keymap[19] = "D#0*C987B654A321NF";
String inputNumber = "";
bool inputMode = false;

long currentFrequency = 27000000;
bool change = false;
int segment = 0;
String oldFrequency_string;
unsigned long lastAudioUpdate = 0;
unsigned long audioUpdateInterval = 50;
String shz;
String skhz;
String smhz;
String frequency_string;
String mode = "LSB";
int modeNumber = 0;

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
  tft.fillRect(0, 110, 320, 240, ILI9341_BLUE);
  drawScale();
  clock_update();
  get_mode();
  Wire.begin();
  Wire.setClock(400000);
  keyPad.loadKeyMap(keymap);
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


  unsigned long currentMillis = millis();
  if (currentMillis - lastAudioUpdate >= audioUpdateInterval) {
    audio_peek();
    lastAudioUpdate = currentMillis;
    if (change == true) {
      clock_update();
    }
  }

  if (keyPad.isPressed()) {

    keypadInput() ;
    updateInputNumberDisplay();
    delay(200);
    if (!inputMode) {
      tft.fillRect(20, 5, 270, 25, ILI9341_BLACK);//<<<<<<<<<<<<<<<<<<< aqui se borra el input number despues del enter
      inputNumber = "";
    }
  }
}

void updateInputNumberDisplay() {
  tft.setCursor(30, 10);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_GREEN);
  tft.print(inputNumber);
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

  // Calcular el nuevo segmento
  int newSegment = audioValue / (4095 / NUM_SEGMENTS);

  // Limitar el nuevo segmento para que no sea mayor que NUM_SEGMENTS - 1
  if (newSegment >= NUM_SEGMENTS) {
    newSegment = NUM_SEGMENTS - 1;
  }

  if (newSegment != segment) {
    tft.fillRoundRect(SCALE_X_OFFSET, AUDIO_BAR_Y_OFFSET, SCALE_WIDTH, SCALE_HEIGHT, 2, ILI9341_BLACK);

    int segmentWidth = SCALE_WIDTH / NUM_SEGMENTS;
    segment = newSegment;

    for (int i = 0; i <= segment; i++) {
      int x = SCALE_X_OFFSET + i * segmentWidth;
      if (i >= NUM_SEGMENTS - 1) {
        tft.fillRoundRect(x, AUDIO_BAR_Y_OFFSET, segmentWidth, SCALE_HEIGHT, 2, ILI9341_RED);
      } else if (i == 6 || i == 7) {
        tft.fillRoundRect(x, AUDIO_BAR_Y_OFFSET, segmentWidth, SCALE_HEIGHT, 2, ILI9341_YELLOW);
      } else {
        tft.fillRoundRect(x, AUDIO_BAR_Y_OFFSET, segmentWidth, SCALE_HEIGHT, 2, ILI9341_GREEN);
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
    shz = ",000";
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

void keypadInput()  {
  int oldClock = currentFrequency;
  char ch = keyPad.getChar();
  int key = keyPad.getLastKey();
  if (ch == 'B') {
    modeNumber++;
    if (modeNumber == 0) {
      mode = "LSB";
    }
    if (modeNumber == 1) {
      mode = "AM";
    }
    if (modeNumber == 2) {
      mode = "USB";
    }
    if (modeNumber >= 3) {
      modeNumber = 0;
      mode = "LSB"; // Asigna "LSB" cuando modeNumber vuelve a 0
    }
    get_mode();
  }


  if (ch == 'A') {

    if (!inputMode) {
      inputMode = true;
      inputNumber = "";
    } else {
      int number = inputNumber.toInt();
      inputMode = false;

      change = true;
      if (number > 1000000 && number < 50000000) {
        currentFrequency = number;


      }
      else if (number < 1000000 || number > 50000000) {
        currentFrequency = oldClock;

      }
    }
  } else if (inputMode) {
    if (isdigit(ch)) {
      inputNumber += ch;
    }
  }
  if (inputMode) {
    tft.fillRect(270, 25, 60, 30, ILI9341_GREEN);
    tft.setCursor(150, 12);
    tft.setTextSize(1.5);
    tft.setTextColor(ILI9341_GREEN);
    tft.print("Introduzca frecuencia");


  }
  if (!inputMode) {
    tft.fillRect(270, 25, 60, 30, ILI9341_BLACK);

  }


}


void get_mode() {
  tft.fillRoundRect(235, 65, 80, 40, 2, ILI9341_ORANGE);
  tft.setCursor(240, 72);
  tft.setTextSize(4);
  tft.setTextColor(ILI9341_BLACK);
  tft.print(mode);
}

