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

This is a work in progress feel free to give me any advice an/or ask any questions

Gustavo :D
