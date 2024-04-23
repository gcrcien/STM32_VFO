#include "arduinoFFT.h"

#define CHANNEL_I PA0
#define CHANNEL_Q PA1
const uint16_t samples = 256; // Este valor DEBE SIEMPRE ser una potencia de 2
const double samplingFrequency = 100000; // Hz, debe ser inferior a 10000 debido al ADC
unsigned int sampling_period_us;
unsigned long microseconds;

double vRealI[samples];
double vImagI[samples];
double vRealQ[samples];
double vImagQ[samples];

ArduinoFFT<double> FFT_I = ArduinoFFT<double>(vRealI, vImagI, samples, samplingFrequency);
ArduinoFFT<double> FFT_Q = ArduinoFFT<double>(vRealQ, vImagQ, samples, samplingFrequency);

void setup() {
  sampling_period_us = round(1000000 * (1.0 / samplingFrequency));
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Ready");
}

void loop() {
  // Muestreo de la señal en fase (I)
  microseconds = micros();
  for (int i = 0; i < samples; i++) {
    vRealI[i] = analogRead(CHANNEL_I);
    vImagI[i] = 0;
    while (micros() - microseconds < sampling_period_us) {
      // Bucle vacío
    }
    microseconds += sampling_period_us;
  }

  // Muestreo de la señal en cuadratura (Q)
  microseconds = micros();
  for (int i = 0; i < samples; i++) {
    vRealQ[i] = analogRead(CHANNEL_Q);
    vImagQ[i] = 0;
    while (micros() - microseconds < sampling_period_us) {
      // Bucle vacío
    }
    microseconds += sampling_period_us;
  }

  // Cálculo de la FFT para la señal en fase (I)
  FFT_I.compute(FFTDirection::Forward);
  FFT_I.complexToMagnitude();

  // Cálculo de la FFT para la señal en cuadratura (Q)
  FFT_Q.compute(FFTDirection::Forward);
  FFT_Q.complexToMagnitude();

  // Suma de las magnitudes de las FFTs de I y Q
  double magnitudes[samples / 2];
  for (int i = 0; i < samples / 2; i++) {
    magnitudes[i] = sqrt(pow(vRealI[i], 2) + pow(vRealQ[i], 2)); // suma de cuadrados de magnitudes
  }

  // Imprimir la magnitud vs la frecuencia de la señal resultante
  Serial.println("Magnitud vs Frecuencia:");
  for (int i = 0; i < samples / 2; i++) {
    double frequency = ((i * 1.0 * samplingFrequency) / samples);
    Serial.print(frequency, 6);
    Serial.print(" Hz ");
    Serial.println(magnitudes[i], 4);
  }

  delay(1000); // Espera un segundo antes de la siguiente iteración
}
