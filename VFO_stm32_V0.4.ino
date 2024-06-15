#include "SPI.h"
#include "Adafruit_ILI9341.h"
#include <Wire.h>
#include <si5351.h>

// Definición de pines para el TFT
#define TFT_CS    PA4
#define TFT_CLK   PA5
#define TFT_MISO  PA6
#define TFT_MOSI  PA7
#define TFT_RST   PB0
#define TFT_DC    PB1
#define AUDIO_IN  PA0
#define TR_PIN    PA1
#define BAT  PA3
#define BLUE            0x008B

// Constantes para la barra de audio
#define SCALE_X_OFFSET 20
#define AUDIO_BAR_Y_OFFSET 70
#define SCALE_WIDTH 200
#define SCALE_HEIGHT 10
#define NUM_SEGMENTS 9

//Definiciones de la FFT
#define TFT_WIDTH 256
#define TFT_HEIGHT 40
#define GRAPH_TOP_MARGIN 4
#define GRAPH_BOTTOM_MARGIN 4
int sampling_period_us;
#include "arduinoFFT.h"

arduinoFFT FFT;

#define CHANNEL PA0
const uint16_t samples = 256;
int samplingFrequency = 10000;
unsigned long microseconds;
int micro1;
unsigned int micro2;
int FFToffset = 29000;
double vReal[samples];
double vImag[samples];
int maxMagnitude = 0.0;
#define NUM_WATERFALL_ROWS 20
int waterfallData[NUM_WATERFALL_ROWS][128] = {0};

// Frecuencia actual
unsigned int currentFrequency = 7000000;

// Inicialización del objeto TFT
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Inicialización del Si5351
Si5351 si5351;

// Rangos de frecuencia
int TR_offset;

// Bandera para cambios de frecuencia
bool change = false;

// Offset de frecuencia para mi radio/mezclador
long int IFoffset = 39994000;

// Segmento actual en la barra de audio
int segment = 0;

// Cadena de la frecuencia anterior
String oldFrequency_string;

// Intervalo de actualización de audio
unsigned long lastAudioUpdate = 0;
unsigned long audioUpdateInterval = 80;

// Variables para construir la cadena de frecuencia
String shz;
String skhz;
String smhz;
String frequency_string;

// Modo de operación (LSB, AM, USB)
String mode = "LSB";
int modeNumber = 0;
// Definición del menu
bool boton = 0;
int amount = 10;
int knob = 1000;
int oldknob = 0;
bool boton1 = 0;
int clockF = 0;
int menu = 0;
bool steps = 0;
int paso = 100;
bool powerString = "SI5351_DRIVE_8MA";
int power = 1;
bool bandC = 0;
bool powerC;
float vBat;
float oldvBat;
int interval = 150;
int debounceT = 15;
int offset2lo;
bool W;
// Definición de la barra de espectro
#define SPECTRUM_X_OFFSET 20
#define SPECTRUM_Y_OFFSET 150
#define SPECTRUM_WIDTH 280
#define SPECTRUM_HEIGHT 60
#define SPECTRUM_MIN_FREQ 26000000UL
#define SPECTRUM_MAX_FREQ 28500000UL


// Arreglo para almacenar los niveles de señal del espectro
int spectrumData[SPECTRUM_WIDTH];

// Valor de entrada de audio
int audioValue;


void setup() {
  //Aceleracion del adc
  adc_set_prescaler(ADC_PRE_PCLK2_DIV_2) ;
  //adc_set_sample_rate(ADC2, ADC_SMPR_7_5);
  adc_set_sample_rate(ADC2, ADC_SMPR_1_5);



  // Inicialización del TFT
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  sampling_period_us = round(1000000 * (1.0 / samplingFrequency));

  // Configuración de pines y habilitación de interrupciones para el control de frecuencia
  pinMode(PB11, INPUT_PULLUP);
  pinMode(PB12, INPUT_PULLUP);
  pinMode(PB15, INPUT_PULLUP);
  pinMode(PA8, INPUT_PULLUP);
  pinMode(PB3, INPUT_PULLUP);
  pinMode(PB4, INPUT_PULLUP);
  pinMode(PB8, INPUT_PULLUP);
  pinMode(PB5, INPUT_PULLUP);
  pinMode(PA9, OUTPUT);
  digitalWrite(PA9, HIGH);
  pinMode(TR_PIN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(PB11), knob_ISR1, FALLING);
  attachInterrupt(digitalPinToInterrupt(PB15), knob_ISR2, FALLING);
  attachInterrupt(digitalPinToInterrupt(PA8), clock1_ISR3, FALLING);
  attachInterrupt(digitalPinToInterrupt(PB3), clock1_ISR4, FALLING);
  attachInterrupt(digitalPinToInterrupt(PB4), boton1_ISR5, FALLING);
  attachInterrupt(digitalPinToInterrupt(PB8), boton2_ISR6, FALLING);
  attachInterrupt(digitalPinToInterrupt(PB5), boton3_ISR7, FALLING);



  tft.drawRect(SCALE_X_OFFSET - 2, AUDIO_BAR_Y_OFFSET - 2, SCALE_WIDTH + 4, SCALE_HEIGHT + 4, ILI9341_WHITE);
  int segmentWidth = SCALE_WIDTH / NUM_SEGMENTS;
  tft.drawRect(SCALE_X_OFFSET, AUDIO_BAR_Y_OFFSET, SCALE_WIDTH, SCALE_HEIGHT, ILI9341_WHITE);
  for (int i = 0; i <= NUM_SEGMENTS; i++) {
    int x = SCALE_X_OFFSET + i * segmentWidth;
    tft.drawFastVLine(x, AUDIO_BAR_Y_OFFSET - 5, SCALE_HEIGHT + 10, ILI9341_WHITE);
    int currentScaleValue =  i;
    // Dibujar el nuevo segmento y el número
    tft.drawFastVLine(x, AUDIO_BAR_Y_OFFSET - 5, SCALE_HEIGHT + 10, ILI9341_WHITE);
    tft.setCursor(x - 10, AUDIO_BAR_Y_OFFSET + SCALE_HEIGHT + 5);
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_WHITE);
    tft.print("S" + String(currentScaleValue));
  }




  // Inicialización de SPI
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

  // Dibujo de la escala en el TFT
  tft.fillRect(0, 110, 320, 240, ILI9341_BLACK);
  drawScale();
  //FFT y sus marcos
  //posicion inicial h, posicion v, posicion
  tft.fillRect(1, 110, 256, 70, BLUE);
  tft.drawRect(0, 110, 258, 80, ILI9341_WHITE);
  int p = 0;
  for (int x = 24; x <= 256; x += 30) {
    p++;
    tft.drawFastVLine(x, 185, 8, ILI9341_WHITE);
    tft.setCursor(x + 2, 200);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(String(p) + "k" );
  }

  // Actualización del reloj y el modo
  clock_update();
  get_mode();

  // Inicialización de la comunicación I2C y carga del mapeo del teclado
  Wire.begin();
  Wire.setClock(400000);
  // Inicialización del Si5351
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA);
  si5351.output_enable(SI5351_CLK0, 1);
  si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_2MA);
  si5351.output_enable(SI5351_CLK1, 1);
  si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_2MA);
  si5351.output_enable(SI5351_CLK2, 1);
  si5351.set_freq((IFoffset ) * SI5351_FREQ_MULT, SI5351_CLK1);

}

// Manejador de interrupción para el control de frecuencia (giro de perilla)
void knob_ISR1() {
  bool dir = digitalRead(PB12);
  if (dir == true) {
    currentFrequency -= paso;
  }
  if (dir == false) {
    currentFrequency += paso;
  }
  change = true;

  if  (currentFrequency < 500000) {
    currentFrequency = 500000;
  }
  if  (currentFrequency > 30500000) {
    currentFrequency = 30500000;
  }
}

// Manejador de interrupción para el control de frecuencia (giro de perilla)
void knob_ISR2() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > debounceT) {
    bool dir = digitalRead(PA8);
    if (dir == true) {
      knob += amount;
    }
    if (dir == false) {
      knob -= amount;
    }
    if  (knob < 0) {
      knob = 0;
    }
    if  (knob > 2500) {
      knob = 2500;
    }
    change = true;
  }
  last_interrupt_time = interrupt_time;

}
void clock1_ISR3() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > debounceT) {
    bool dir = digitalRead(PB15);
    if (dir == true) {
      knob -= amount;
    }
    if (dir == false) {
      knob += amount;
    }
    if  (knob < 0) {
      knob = 0;
    }
    if  (knob > 2500) {
      knob = 2500;
    }
    change = true;
    //dejump = true;
  }
  last_interrupt_time = interrupt_time;

}
void clock1_ISR4() {


  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > debounceT + 50)
  {
    boton = !boton;
    if  (boton) {
      offset2lo = knob * - 1 ;
      mode = "LSB";
    }
    else {
      offset2lo = knob * 1;
      mode = "USB";

    }
    last_interrupt_time = interrupt_time;

  }
  get_mode();
  change = true;


}



void boton1_ISR5() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > debounceT)
  {
    steps = !steps;
    if  (steps) {
      paso = 100 ;
    }
    else {
      paso = 1000;
    }
  }
  last_interrupt_time = interrupt_time;
}




void boton2_ISR6() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > debounceT)
  {
    clockF += 1;
    bandC = true;
    if  (clockF > 10) {
      clockF = 1;
    }
  }
  last_interrupt_time = interrupt_time;
  change = true;
}

void boton3_ISR7() {


  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > debounceT)
  {
    power += 1;
    powerC = true;
    if  (power > 4) {
      power = 1;
    }
  }
  last_interrupt_time = interrupt_time;
  change = true;

}

// Declarar un arreglo para mantener un seguimiento de los valores anteriores
int previousScaleValues[NUM_SEGMENTS + 1] = {0};

void drawScale() {
  int segmentWidth = SCALE_WIDTH / NUM_SEGMENTS;
  for (int i = 0; i <= NUM_SEGMENTS; i++) {
    int x = SCALE_X_OFFSET + i * segmentWidth;
    int currentScaleValue =  i;
    // Comparar el valor actual con el valor anterior
    if (currentScaleValue != previousScaleValues[i]) {
      previousScaleValues[i] = currentScaleValue;
    }
  }
}


// Actualiza la información de frecuencia en el TFT
void clock_update() {
  if (powerC) {
    //si5351.output_enable(SI5351_CLK0, 0);
    switch (power) {
      case 1:
        si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA);
        si5351.output_enable(SI5351_CLK0, 1);
        break;
      case 2:
        si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_4MA);
        si5351.output_enable(SI5351_CLK0, 1);
        break;
      case 3:
        si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_6MA);
        si5351.output_enable(SI5351_CLK0, 1);
        break;
      case 4:
        si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);
        si5351.output_enable(SI5351_CLK0, 1);
        break;
    }
    powerC = LOW;
  }
  if (bandC) {
    switch (clockF) {
      case 1:
        tft.fillRect(10, 10, 80, 40, ILI9341_BLACK);
        currentFrequency = 1800000;// statements
        tft.setCursor(10, 10);                             // Actualización de VOLTAJE
        tft.setTextSize(2);
        tft.setTextColor(ILI9341_GREEN);
        tft.print("160M"); // oldclockF
        break;
      case 2:
        tft.fillRect(10, 10, 80, 40, ILI9341_BLACK);
        currentFrequency = 3500000;// statements
        tft.setCursor(10, 10);                             // Actualización de VOLTAJE
        tft.setTextSize(2);
        tft.setTextColor(ILI9341_GREEN);
        tft.print("80M"); // oldclockF
        break;
      case 3:
        tft.fillRect(10, 10, 80, 40, ILI9341_BLACK);
        currentFrequency = 7000000;// statements
        tft.setCursor(10, 10);                             // Actualización de VOLTAJE
        tft.setTextSize(2);
        tft.setTextColor(ILI9341_GREEN);
        tft.print("40M"); // oldclockF
        break;
      case 4:
        tft.fillRect(10, 10, 80, 40, ILI9341_BLACK);
        currentFrequency = 10000000;// statements
        tft.setCursor(10, 10);                             // Actualización de VOLTAJE
        tft.setTextSize(2);
        tft.setTextColor(ILI9341_GREEN);
        tft.print("30M"); // oldclockF
        break;
      case 5:
        tft.fillRect(10, 10, 80, 40, ILI9341_BLACK);
        currentFrequency = 14000000;// statements
        tft.setCursor(10, 10);                             // Actualización de VOLTAJE
        tft.setTextSize(2);
        tft.setTextColor(ILI9341_GREEN);
        tft.print("20M"); // oldclockF
        break;
      case 6:
        tft.fillRect(10, 10, 80, 40, ILI9341_BLACK);
        currentFrequency = 18000000;// statements
        tft.setCursor(10, 10);                             // Actualización de VOLTAJE
        tft.setTextSize(2);
        tft.setTextColor(ILI9341_GREEN);
        tft.print("17M"); // oldclockF
        break;
      case 7:
        tft.fillRect(10, 10, 80, 40, ILI9341_BLACK);
        currentFrequency = 21000000;// statements
        tft.setCursor(10, 10);                             // Actualización de VOLTAJE
        tft.setTextSize(2);
        tft.setTextColor(ILI9341_GREEN);
        tft.print("15M"); // oldclockF
        break;
      case 8:
        tft.fillRect(10, 10, 80, 40, ILI9341_BLACK);
        currentFrequency = 24500000;// statements
        tft.setCursor(10, 10);                             // Actualización de VOLTAJE
        tft.setTextSize(2);
        tft.setTextColor(ILI9341_GREEN);
        tft.print("12M"); // oldclockF
        break;
      case 9:
        tft.fillRect(10, 10, 80, 40, ILI9341_BLACK);
        currentFrequency = 27000000;// statements
        tft.setCursor(10, 10);                             // Actualización de VOLTAJE
        tft.setTextSize(2);
        tft.setTextColor(ILI9341_GREEN);
        tft.print("11M"); // oldclockF
        break;
      case 10:
        tft.fillRect(10, 10, 80, 40, ILI9341_BLACK);
        currentFrequency = 28000000;// statements
        tft.setCursor(10, 10);                             // Actualización de VOLTAJE
        tft.setTextSize(2);
        tft.setTextColor(ILI9341_GREEN);
        tft.print("10M"); // oldclockF
        break;
    }
    bandC = false;
  }
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
  tft.setCursor(140, 10);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK);
  tft.print(oldknob);
  tft.setCursor(140, 10);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_GREEN);
  tft.print(knob);
  oldknob = knob;
  change = false;
}

// Lee el valor de entrada de audio
void audio_peek() {
  audioValue = maxMagnitude / 200;
  // Calcula el nuevo segmento
  int newSegment = audioValue / (60 / NUM_SEGMENTS);
  maxMagnitude = 10;
  // Limita el nuevo segmento para que no sea mayor que NUM_SEGMENTS - 1
  if (newSegment >= NUM_SEGMENTS) {
    newSegment = NUM_SEGMENTS - 1;
  }

  if (newSegment != segment) {
    tft.fillRoundRect(SCALE_X_OFFSET, AUDIO_BAR_Y_OFFSET, SCALE_WIDTH , SCALE_HEIGHT, 2, ILI9341_BLACK);
    int segmentWidth = SCALE_WIDTH / NUM_SEGMENTS;
    segment = newSegment;
    for (int i = 0; i <= segment; i++) {
      int x = SCALE_X_OFFSET + i * segmentWidth;
      if (i >= NUM_SEGMENTS - 1) {
        tft.fillRoundRect(x + 1, AUDIO_BAR_Y_OFFSET, segmentWidth - 2, SCALE_HEIGHT, 2, ILI9341_RED);
      } else if (i == 6 || i == 7) {
        tft.fillRoundRect(x + 1, AUDIO_BAR_Y_OFFSET, segmentWidth - 2, SCALE_HEIGHT, 2, ILI9341_YELLOW);
      } else {
        tft.fillRoundRect(x + 1, AUDIO_BAR_Y_OFFSET, segmentWidth - 2, SCALE_HEIGHT, 2, ILI9341_GREEN);
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

}

// Muestra el modo de operación en el TFT
void get_mode() {
  tft.fillRoundRect(235, 65, 80, 40, 2, ILI9341_ORANGE);
  tft.setCursor(240, 72);
  tft.setTextSize(4);
  tft.setTextColor(ILI9341_BLACK);
  tft.print(mode);
}


void performFFTAndDrawGraph(int xOffset, int yOffset) {
  microseconds = micros();
  for (int i = 0; i < samples; i++) {
    vReal[i] = analogRead(CHANNEL);
    vImag[i] = 0;
    while (micros() - microseconds < sampling_period_us) {
    }
    microseconds += sampling_period_us;
  }

  FFT = arduinoFFT(vReal, vImag, samples, samplingFrequency);
  FFT.Windowing(FFT_WIN_TYP_RECTANGLE, FFT_FORWARD);
  FFT.Compute(FFT_FORWARD);
  FFT.ComplexToMagnitude();
  unsigned int fft_time = micros() - micro2;
  vReal[0] = 0;
  vReal[1] = 0;

  // Configura los márgenes y dimensiones del gráfico
  int graphWidth = TFT_WIDTH;
  int graphHeight = TFT_HEIGHT - GRAPH_TOP_MARGIN - GRAPH_BOTTOM_MARGIN;
  int startX = xOffset;
  int startY = GRAPH_TOP_MARGIN + graphHeight + yOffset;

  // Calcula el ancho de cada barra en el gráfico
  float barWidth = static_cast<float>(graphWidth) / (samples);

  // Encuentra el valor máximo en los datos de la FFT
  maxMagnitude = 0;
  for (int i = 0; i < samples / 2; i++) {
    if (vReal[i] > maxMagnitude) {
      maxMagnitude = vReal[i];
    }
  }

  // Dibuja las barras de la gráfica con 2 píxeles de ancho
  for (int i = 0; i < samples / 2; i++) {
    // Calcula la altura de la barra según el valor de la FFT
    float altura = maxMagnitude;
    if (altura < 3000) {
      altura = 3000;
    }
    int barHeight = static_cast<int>((vReal[i] / altura) * graphHeight);

    // Calcula la posición de inicio de la barra
    int x = startX + static_cast<int>(i * barWidth * 2); // Multiplica por 2 para el doble de ancho
    int y1 = startY - barHeight;

    // Mapea el valor de intensidad para la gráfica de barras
    //int mappedValue = map(vReal[i], 0, 4000, 0, 255);  // Mapear a un rango de 0 a 255
    //uint16_t color = intensityToColor(mappedValue);

    // Borra la gráfica anterior con barras azules de 2 píxeles de ancho
    tft.fillRect(x, yOffset + 1, 2, TFT_HEIGHT - 5, BLUE);


    // Dibuja la nueva barra como una línea vertical de 2 píxeles de ancho
    tft.fillRect(x, y1, 2, barHeight, ILI9341_GREEN);  // se puede usar la variable color para usar el color de intensidad

  }

  if (W) {
    for (int col = 0; col < 128; ++col) {
      int mappedValue = map(vReal[col], 0, 7000, 0, 255);  // Solo mapear a un rango de 0 a 255
      waterfallData[NUM_WATERFALL_ROWS - 1][col] = mappedValue;
    }

    for (int row = 0; row < NUM_WATERFALL_ROWS - 1; ++row) {
      for (int col = 0; col < 128; ++col) {
        waterfallData[row][col] = waterfallData[row + 1][col];
      }
    }

    for (int row = 0; row < NUM_WATERFALL_ROWS; ++row) {
      for (int col = 0; col < 128; ++col) {
        int intensity = waterfallData[row][col];
        uint16_t color = intensityToColor(intensity);

        // Dibujar un rectángulo de 2x2 píxeles
        tft.fillRect((col * 2) + xOffset, TFT_HEIGHT - 1 - (row * 2) + yOffset + 38, 2, 2, color);
      }
    }

    W = 0;
  }
}



uint16_t intensityToColor(int intensity) {
  uint8_t r, g, b;

  if (intensity == 0) {
    // Ausencia de señal (negro)
    r = 0;
    g = 0;
    b = 0;
  } else if (intensity < 64) {
    // Azul oscuro a azul
    r = 0;
    g = 0;
    b = map(intensity, 0, 63, 0, 255);
  } else if (intensity < 128) {
    // Azul a cian
    r = 0;
    g = map(intensity, 64, 127, 0, 255);
    b = 255;
  } else if (intensity < 192) {
    // Cian a amarillo
    r = map(intensity, 128, 191, 0, 255);
    g = 255;
    b = map(intensity, 128, 191, 255, 0);
  } else {
    // Amarillo a rojo
    r = 255;
    g = map(intensity, 192, 255, 255, 0);
    b = 0;
  }

  return tft.color565(r, g, b);
}


void loop() {
  // Actualización de audio y frecuencia
  unsigned long currentMillis = millis();

  if (currentMillis - lastAudioUpdate >= audioUpdateInterval) {
    performFFTAndDrawGraph(1, 110);
    W = 1;
    audio_peek();
    interval++;
    if (interval >= 150) {
      vBat = analogRead(BAT);
      interval = 0;
      vBat = vBat * .00892;
      tft.setCursor(240, 10);                             // Actualización de VOLTAJE
      tft.setTextSize(2);
      tft.setTextColor(ILI9341_BLACK);
      tft.print(oldvBat); // oldclockF
      tft.print("V");
      tft.setCursor(240, 10);
      tft.setTextSize(2);
      tft.setTextColor(ILI9341_GREEN);
      tft.print(vBat );
      tft.print("V");
      oldvBat = vBat;
    }
    lastAudioUpdate = currentMillis;
    if (change == true) {
      clock_update();
      si5351.set_freq((  IFoffset + currentFrequency - 2000) * SI5351_FREQ_MULT, SI5351_CLK0);
      si5351.set_freq((IFoffset + offset2lo ) * SI5351_FREQ_MULT, SI5351_CLK1);
    }
  }
}
