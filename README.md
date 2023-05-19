# trellis_astro_keypad
Arduino IDE code to program an Arduino Leonoardo or compatible to use an Adafruit Trellis I2C keypad as an input device for logging astronomical observations.

Device appears as an HID (keyboard). Most keypresses simply result in one or more characters being 'typed' as if on a keyboard, but there are a few special cases (e.g. <alt><tab>) for the assumed Linux, Emacs, and Kstars environment the keypad is designed for. 
Key bindings and pins used are documented in code comments. Software includes reading pin interrupts used for a rotary encoder for zooming the view in and out (issues <ctrl>+ and <crtl>- characters)
  Trellis led brightness is hard coded (there is a version of this somewhere that reads a potentiometer to set brighness that I can't find).
  
