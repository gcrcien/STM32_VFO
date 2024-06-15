# STM32_VFO
VFO for si5351, stm32 and ili9341 2.8" spi tft 

This is the start of a easy to make VFO to use with existing radios or even make your own,
the system interfaces an optical encoder on pins B12 and B13, a big SPI display a clasic tft 2.8 ILI9341,
a matrix 4x4 Keypad using I2C interface with PCF8574, i know the screen has touch inputs
however i like to have a tactile input and the keypad just feels better.

Te MCU is constantly monitoring a few things, for example the audio level on receive.
Some radios use the same meter for audio receive and modulation measurement which this radio would be suited for.

![WhatsApp Image 2023-10-15 at 18 07 27 (1)](https://github.com/gcrcien/STM32_VFO/assets/126195505/389e7bd7-a213-4c23-8de0-13e46e9e6645)

The system is currently coded with spanish languaje but this would be a simple task to translate.
they Keypad input is activated with the "A" key, asks for a frequency in Hz, when this key is pressed again, and if the frequency is 
whithin acceptable range, the value is passed to the VFO and is desplayed.

![WhatsApp Image 2023-11-06 at 07 51 49](https://github.com/gcrcien/STM32_VFO/assets/126195505/0a6e8a0d-3119-4dd5-97f6-0e718aa8ebff)

Recently i was able to include a simple spectrum view, its just a FFT of the input of my radio, this needs to be downconverted to place the center frequency
of the FFT in the 40khz region, the system shows a 80khz wide view around your frequency of interest, in my case i just tapped my receiver before the IF filter,
this is because the filter is too narrow for more than the desired 2500hz wide area of interest for audio listening.
Its not ideal, the signal is kept into acceptable levels cause i connected the downconverter after the variable gain amp which is controlled by the agc, 
so the signals of interest basically disapear out of the fft view if they are a bit high. Need to fix this, a posible way to do it is basically make a 
mini receiver, so a receiver within a receiver.

Lattest update i have been working on a PCB for the whole project, it handles most fo the VFO part of the system and also includes a Tayloe detector for a realtime sprectrum graph, still needs to be tested, uploaded the Gerber files, PDF and a sch from altium, it also has an IQ output via the 3.5mm jack to connect to sdr++ and act as a SDR via the sound card.
![image](https://github.com/gcrcien/STM32_VFO/assets/126195505/5107c794-6de5-4f93-970a-8c545f4693ce)

Latest version includes a small waterfall display of the sampled audio 

[![Watch the video](https://img.youtube.com/vi/gbz6IhAs6Uw/0.jpg)](https://youtu.be/gbz6IhAs6Uw)

More work ahead

![18d13fbe-9281-404b-9812-ee0a2d2707f5](https://github.com/gcrcien/STM32_VFO/assets/126195505/afec86c1-1495-4aa1-930a-9819f65e42c4)

This is a work in progress feel free to give me any advice and/or ask any questions

Gustavo :D

