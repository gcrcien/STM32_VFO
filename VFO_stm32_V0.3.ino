#include "SPI.h"
#include "Adafruit_ILI9341.h"
#include <Wire.h>
#include <si5351.h>
#include "I2CKeyPad.h"

// Definición de pines para el TFT
#define TFT_CS    PA4
#define TFT_CLK   PA5
#define TFT_MISO  PA6
#define TFT_MOSI  PA7
#define TFT_RST   PB0
#define TFT_DC    PB1
#define AUDIO_IN  PA0

// Constantes para la barra de audio
#define SCALE_X_OFFSET 20
#define AUDIO_BAR_Y_OFFSET 70
#define SCALE_WIDTH 200
#define SCALE_HEIGHT 20
#define NUM_SEGMENTS 9

// Frecuencia actual
unsigned long currentFrequency = 27455000;

// Inicialización del objeto TFT
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Dirección del teclado I2C
const uint8_t KEYPAD_ADDRESS = 0x20;
I2CKeyPad keyPad(KEYPAD_ADDRESS);

// Inicialización del Si5351
Si5351 si5351;

// Rangos de frecuencia
unsigned long minFrequency = 26000000;     // Frecuencia mínima de 26 MHz
unsigned long maxFrequency = 28500000;     // Frecuencia máxima de 28.5 MHz
unsigned long frequencyStep = 1000;

// Mapeo de teclas del teclado
char keymap[19] = "D#0*C987B654A321NF";

// Variables para la entrada de frecuencia desde el teclado
String inputNumber = "";
bool inputMode = false;

// Bandera para cambios de frecuencia
bool change = false;

// Offset de frecuencia para mi radio/mezclador
long int IFoffset = 10696000; 

// Segmento actual en la barra de audio
int segment = 0;

// Cadena de la frecuencia anterior
String oldFrequency_string;

// Intervalo de actualización de audio
unsigned long lastAudioUpdate = 0;
unsigned long audioUpdateInterval = 50;

// Variables para construir la cadena de frecuencia
String shz;
String skhz;
String smhz;
String frequency_string;

// Modo de operación (LSB, AM, USB)
String mode = "LSB";
int modeNumber = 0;

// Definición de la barra de espectro
#define SPECTRUM_X_OFFSET 20
#define SPECTRUM_Y_OFFSET 150
#define SPECTRUM_WIDTH 280
#define SPECTRUM_HEIGHT 60
#define SPECTRUM_MIN_FREQ 26000000UL
#define SPECTRUM_MAX_FREQ 28500000UL

// Segmento actual en el espectro
int newSegment;

// Arreglo para almacenar los niveles de señal del espectro
int spectrumData[SPECTRUM_WIDTH];

// Valor de entrada de audio
int audioValue;

// Bandera para la exploración de frecuencias
bool scan = false;

void setup() {
  // Inicialización del TFT
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);

  // Configuración de pines y habilitación de interrupciones para el control de frecuencia
  pinMode(PB12, INPUT_PULLUP);
  pinMode(PB13, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PB12), clock1_ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(PB13), clock1_ISR2, FALLING);

  // Inicialización de SPI
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));

  // Dibujo de la escala en el TFT
  tft.fillRect(0, 110, 320, 240, ILI9341_BLUE);
  drawScale();

  // Actualización del reloj y el modo
  clock_update();
  get_mode();

  // Inicialización de la comunicación I2C y carga del mapeo del teclado
  Wire.begin();
  Wire.setClock(400000);
  keyPad.loadKeyMap(keymap);

  // Inicialización del Si5351
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_4MA);
  si5351.output_enable(SI5351_CLK0, 1);
}

// Manejador de interrupción para el control de frecuencia (giro de perilla)
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

// Manejador de interrupción para el control de frecuencia (giro de perilla)
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

// Dibuja la escala en el TFT
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

// Actualiza la visualización del número de entrada en el TFT
void updateInputNumberDisplay() {
  tft.setCursor(30, 10);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_GREEN);
  tft.print(inputNumber);
}

// Actualiza la información de frecuencia en el TFT
void clock_update() {
  f_string();
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

// Lee el valor de entrada de audio
void audio_peek() {
  audioValue = analogRead(AUDIO_IN);

  // Calcula el nuevo segmento
  newSegment = audioValue / (600 / NUM_SEGMENTS);

  // Limita el nuevo segmento para que no sea mayor que NUM_SEGMENTS - 1
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

// Construye la cadena de frecuencia en formato MHz, kHz y Hz
void f_string() {
  int mhz = currentFrequency / 1000000;
  int khz = (currentFrequency % 1000000) / 1000;
  int hz = currentFrequency - ((mhz * 1000000) + (khz * 1000));

  smhz = String(mhz);
  if (mhz < 10 && mhz >= 1) {
    smhz = " " + String(mhz);
  }

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

// Procesa la entrada del teclado
void keypadInput() {
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
  if (ch == '*') {
    scan = !scan;
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

// Muestra el modo de operación en el TFT
void get_mode() {
  tft.fillRoundRect(235, 65, 80, 40, 2, ILI9341_ORANGE);
  tft.setCursor(240, 72);
  tft.setTextSize(4);
  tft.setTextColor(ILI9341_BLACK);
  tft.print(mode);
}

// Dibuja el espectro en el TFT
void drawSpectrum() {
  tft.fillRoundRect(70, 150, 80, 40, 2, ILI9341_BLACK);

  for (int x = 0; x < SPECTRUM_WIDTH; x++) {
    int freq = map(x, 0, SPECTRUM_WIDTH, SPECTRUM_MIN_FREQ, SPECTRUM_MAX_FREQ);
    int level = map(spectrumData[x], 0, 1023, SPECTRUM_Y_OFFSET + SPECTRUM_HEIGHT, SPECTRUM_Y_OFFSET);
    tft.drawFastVLine(SPECTRUM_X_OFFSET + x, level, SPECTRUM_Y_OFFSET + SPECTRUM_HEIGHT - level, ILI9341_RED);
  }
}

void loop() {
  // Exploración de frecuencias
  if (scan) {
    if (currentFrequency <= maxFrequency) {
      currentFrequency += frequencyStep;
      si5351.set_freq((currentFrequency - IFoffset ) * SI5351_FREQ_MULT, SI5351_CLK0);
      clock_update();
      audio_peek();
      delay(20);
      if (audioValue > 60) {
        scan = !scan;
      }
    }
    if (currentFrequency > maxFrequency) {
      scan = !scan;
    }
  }

  // Actualización de audio y frecuencia
  unsigned long currentMillis = millis();
  if (currentMillis - lastAudioUpdate >= audioUpdateInterval) {
    audio_peek();
    lastAudioUpdate = currentMillis;
    if (change == true) {
      clock_update();
      si5351.set_freq((currentFrequency - IFoffset ) * SI5351_FREQ_MULT, SI5351_CLK0);
    }
  }

  // Procesamiento del teclado
  if (keyPad.isPressed()) {
    keypadInput();
    updateInputNumberDisplay();
    delay(200);
    if (!inputMode) {
      tft.fillRect(20, 5, 270, 25, ILI9341_BLACK);
      inputNumber = "";
    }
  }
}
