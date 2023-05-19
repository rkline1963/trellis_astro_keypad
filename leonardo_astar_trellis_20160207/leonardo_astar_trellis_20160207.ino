//Keypad and rotary encoder code for Astronomy Uses
//Based on v4 of the _wiichuck_trellis code but removing the wiichcuk mouse code
//April 2012, modifications January 2015
//red-+5, white-gnd, white-sda, yllw-scl

#include <Wire.h>
#include "Adafruit_Trellis.h"
//Includes below were for the Adafruit unified DHT which has completely different classes
//defined than the basic DHT library for reasons not apparent to me just using basic DHT for now
//#include <Adafruit_Sensor.h>
#include <DHT.h>
//#include <DHT_U.h>

#define DHTTYPE DHT22 
#define TEMPTYPE 1
#define DHTPIN 4 // use pin 4 since 0 and 1 are the interrupt pins for rotary encoder and 
// pins 2 and 3 used for I2C on the Leonardo and Micro
#define POTPIN A7
float h;
float t;

DHT dht(DHTPIN, DHTTYPE);

// Rotary encoder parameters using pins 0 and 1 to be able to assign leonardo interrupts;
int aPin = 0;
int bPin = 1;
volatile int encoderChange = false;

// Wii nunchuck parameters 
// For OSX change below to char ctrlKey= KEY_LEFT_GUI;
char ctrlKey = KEY_LEFT_CTRL;
int loop_cnt=0;

int led = 13;

// Variables that will be checked for output of millis to determine when to read data over i2c
long nextMouse = 0;
long nextTrellis = 0;
long nextTemp = 0;

// Keep hooks to do something for a mouse every 10ms and Adafruit Trellis examp reads every 30ms
// Plan to add PSP analog stick mouse later
int incrMouse = 10;
int incrTrellis= 30;
int incrTemp = 2000; // DHT22 wants to be read no more than once every 2 seconds

// parameters for reading the joystick:
int range = 40;               // output range of X or Y movement
int threshold = range/10;      // resting threshold
int center = range/2;         // resting position value

boolean mouseIsActive = false;    // whether or not to control the mouse
int lastSwitchState = LOW;        // previous switch state


// Trellis keypad parameters

uint8_t numLit = 4;
uint8_t alwaysLit[] = {3,7,12,14};
// padBrightness 8 for debugging indoors, use 1 or 2 for astronomy keypad use
uint8_t padBrightness = 8;

//State of the trellis pad leds
int ledState = 0;
//State of <alt> key on trellis keypad
int altPressed = 0;
//State variable for certain alt keys
int oddKey = 0;
// current time from millis command
long currentMillis = 0;
long previousMillis = 0;
// Length of time to keep all trellis keypad leds lit if no activity
long interval = 10000;

String tempStr;
/* strings to issue for each keypad press some, key handler modifies some of 
 these notably key 12 which is used as an <alt> key and changes some of the other
 key behaviors and key 11 which issues a <shift>F9 for an emacs key binding 
   7      8      9      M
 
   4      5      6      NGC
 
   1      2      3    "<shift>F9, "
 
 <alt>    0   <ctrl>f  <enter>
 
 If the <alt> key is held down the key map is the following
   7      8      9     <ctrl>+
 
   4      5      6     <ctrl>-
 
   1      2      3     <log temp from DHT sensor>
 
 <alt>    0  <alt><tab> <esc>q
 
 Most key presses issue a key press and release but the <alt><tab>
 <ctrl>+ and <ctrl>- issues a key press and only issue the key release 
 when they are released so holding them will cause them to be repeated
 as if the key combo were held down on a regular key board.  Holding the 
 <alt> key down and pressing and releasing the <alt><tab> key will behave
 as with a regular keyboard and let you cycle through open application 
 windows to switch to
 */
char * keyStrings[]={"7","8","9","M","4","5","6","NGC",
  "1","2","3",", ","","0","f","q"};
  
Adafruit_Trellis matrix0 = Adafruit_Trellis();

// uncomment the below to add 3 more matrices
/*
Adafruit_Trellis matrix1 = Adafruit_Trellis();
Adafruit_Trellis matrix2 = Adafruit_Trellis();
Adafruit_Trellis matrix3 = Adafruit_Trellis();
// you can add another 4, up to 8
*/

// Just one
Adafruit_TrellisSet trellis =  Adafruit_TrellisSet(&matrix0);
// or use the below to select 4, up to 8 can be passed in
//Adafruit_TrellisSet trellis =  Adafruit_TrellisSet(&matrix0, &matrix1, &matrix2, &matrix3);

// set to however many you're working with here, up to 8
#define NUMTRELLIS 1

#define numKeys (NUMTRELLIS * 16)

// Connect Trellis Vin to 5V and Ground to ground.
// Connect the INT wire to pin #A2 (can change later!)
// Trellis example codes says it doesn't use the INTPIN so it is not connected
#define INTPIN A2
// Connect I2C SDA pin to your Arduino SDA line
// Connect I2C SCL pin to your Arduino SCL line
// All Trellises share the SDA, SCL and INT pin! 
// Even 8 tiles use only 3 wires max



void setup() {
  pinMode(aPin, INPUT_PULLUP);
  attachInterrupt(2, encoderISR, CHANGE);
  pinMode(bPin, INPUT_PULLUP);
  attachInterrupt(3, encoderISR, CHANGE);
  
  pinMode(led, OUTPUT);
  /* delay and led light from debugging 
  digitalWrite(led, HIGH);
  delay(2000);
  digitalWrite(led, LOW);
  */
  
   // INT pin requires a pullup
  pinMode(INTPIN, INPUT);
  digitalWrite (INTPIN, HIGH);
  
  // begin() with the addresses of each panel in order
  // I find it easiest if the addresses are in order
  trellis.begin(0x74);  // only one, address bit two pads shorted on mine
  // trellis.begin(0x70, 0x71, 0x72, 0x73);  // or four!

  // light up all the LEDs in order
  for (uint8_t i=0; i<numKeys; i++) {
    trellis.setLED(i);
    trellis.writeDisplay();    
    delay(50);
  }  
  // then turn them off
  for (uint8_t i=0; i<numKeys; i++) {
    trellis.clrLED(i);
    trellis.writeDisplay();    
    delay(50);
  }
  // Turn on the LEDs always kept on
  padLEDMostlyOff();
  /* set brightness (for astronomy use this will be 0-2 for debugging indoors use 8
  add adjustable brightness later */

// Function reads analog pin connected to potentiometer to set brightness
  setPadBrightness();


  
  // take control of the mouse:
//  Mouse.begin();
  Keyboard.begin();
  Serial.begin(9600);
  Serial.println("Combined Trellis Keypad and zoom knob Demo");
//  matrix.begin(0x70);
  nextMouse = millis() + incrMouse;
  nextTrellis = millis() + incrTrellis;
  
// Initialize DHT temperature sensor
  dht.begin(); //Start temperature sensor
  delay(incrTemp);
  updateTemp();
}


void loop() {
   currentMillis = millis();
   if ( currentMillis >= nextMouse ) 
   {
//     someMouse();
     nextMouse += incrMouse;
   }
  if ( currentMillis >= nextTrellis )
  {
    setPadBrightness();
    readTrellis();
    nextTrellis += incrTrellis;
  }
  if (currentMillis >= nextTemp) 
  {
    updateTemp();
    nextTemp += incrTemp;
  }
  if (encoderChange)
  {
    int change = getEncoderTurn();
    if (change>0)
    {
      //Release all keys and reset state variables for trellis keypad
      Keyboard.releaseAll();
      altPressed = 0;
      oddKey = 0;
      // Now do a <ctrl>-
      Keyboard.press(KEY_LEFT_CTRL);
      Keyboard.write('-');
      Keyboard.release(KEY_LEFT_CTRL);
    } else if (change<0) {
      Keyboard.releaseAll();
      altPressed = 0;
      oddKey = 0;
      Keyboard.press(KEY_LEFT_CTRL);
      Keyboard.write('+');
      Keyboard.release(KEY_LEFT_CTRL);
    }
  }
}

void updateTemp() {
  // change from version in rgblcd_DHT_temp sketch which checked time in this function
  // nextTemp is checked in main loop
  h = dht.readHumidity();
  t = dht.readTemperature(TEMPTYPE);
/*    lcd.setCursor(5,2);
    lcd.print(t);
    lcd.setCursor(5,3);
    lcd.print(h); */
  Serial.print("Humidity: ");
  if ( isnan(h) ) {
    Serial.print("BAD");
  } else {
    Serial.print(h);
  }
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  if ( isnan(t) ) {
    Serial.print("BAD");
  } else {
    Serial.print(t);
  }
  Serial.print("\n");
}
  
void encoderISR()
  {
    encoderChange = true;
  }

int getEncoderTurn()
{
  // return -1, 0, or +1
  static int oldA = LOW;
  static int oldB = LOW;
  int result = 0;
  int newA = digitalRead(aPin);
  int newB = digitalRead(bPin);
  if (newA != oldA || newB != oldB)
  {
    // something has changed
    if (oldA == LOW && newA == HIGH)
    {
      result = -(oldB * 2 - 1);
    }
  }
  oldA = newA;
  oldB = newB;
  encoderChange = false;
  return result;
} 

void readTrellis() {
  // If a button was just pressed or released...
  if (trellis.readSwitches()) {
    if (ledState) {
      // leds on so reset time until most leds will be turned off
      previousMillis = currentMillis;
    } else {
      // turn all pad leds on
      ledState = 1;
      padLEDOn();
      previousMillis = currentMillis;
    }
    // Was the alt key pressed previously or is it pressed now
    if (trellis.justReleased(12)) {
      // Releasing <alt> key clears all the key presses
      Keyboard.releaseAll();
      altPressed = 0;
    } else if (altPressed==-1) {
        if (trellis.justReleased(3)) {
          Keyboard.release('+');
          Keyboard.release(KEY_LEFT_CTRL);
          Keyboard.press(KEY_LEFT_ALT);
          altPressed = 1;
          oddKey = 0;
        } else if (trellis.justReleased(7)) {
          Keyboard.release('-');
          Keyboard.release(KEY_LEFT_CTRL);
          Keyboard.press(KEY_LEFT_ALT);
          altPressed = 1;
          oddKey = 0;
        }
    } else if (altPressed == 1) {
        if (trellis.justReleased(14)) {
          Keyboard.release(KEY_TAB);
        }
    } else if (trellis.justReleased(14)) {
      /* if we get here is should be just the case of the find key with no alt
      pressed but if alt-M or alt-NGC are pressed before the find something odd may
      happen */
      Keyboard.release('f');
      Keyboard.release(KEY_LEFT_CTRL);
    }
        
 
    // go through every button, but check the alt key first
    if (trellis.justPressed(12)) {
      altPressed = 1;
      Keyboard.press(KEY_LEFT_ALT);
    }
    for (uint8_t i=0; i<numKeys; i++) {
      // if it was pressed...
      if (trellis.justPressed(i) && !oddKey) {
	Serial.print("v"); Serial.println(i);
        oddKey = keyHandler(i);
      }
    }
  } else {
    if (currentMillis-previousMillis > interval) {
      // turn off most of the pad leds
      ledState = 0;
      padLEDMostlyOff();
    }
  }
}

void padLEDOn() {
  for (uint8_t i=0; i<numKeys; i++) {
    trellis.setLED(i);
  }
  trellis.writeDisplay();
}

void padLEDMostlyOff() {
  for (uint8_t i=0; i<numKeys; i++) {
    trellis.clrLED(i);
  }
  for (uint8_t i=0; i<numLit; i++) {
    trellis.setLED(alwaysLit[i]);
  }
  trellis.writeDisplay();
}

void kbdWrite(String keyStr) {
  if (keyStr.length()) {
    for (uint8_t i=0; i<keyStr.length(); i++) {
      Keyboard.write(keyStr[i]);
    }
  }
}

int keyHandler(uint8_t key) {
  String tmpStr;
  // Returns 1 if one of the odd alt key cases releases a pressed alt key else 0
  switch (key) {

    case 3:
    /* If <alt> key pressed release it and issue a zoom in character combo 
    <ctrl>+ change altPressed to -1 to note the odd case else issue ', M' for
    a Messier object reference */
      if (altPressed) {
        Keyboard.release(KEY_LEFT_ALT);
        Keyboard.press(KEY_LEFT_CTRL);
        Keyboard.press('+');
        altPressed = -1;
      } else {
        tmpStr = String(keyStrings[key]);
        kbdWrite(tmpStr);
      }
      break;
    case 7:
    /* If alt key pressed release it and issue a zoom out character combo <ctrl>-
   change altPressed to -1 to note the odd case else issue ', NGC' for an NGC 
   object reference */
      if (altPressed) {
        Keyboard.release(KEY_LEFT_ALT);
        Keyboard.press(KEY_LEFT_CTRL);
        Keyboard.press('-');
        altPressed = -1;
        } else {
        tmpStr = String(keyStrings[key]);
        kbdWrite(tmpStr);
      }
      break;
    case 11:
      if (altPressed) {
        // Log humidity and temperature
        Keyboard.release(KEY_LEFT_ALT);
        tmpStr = String(int(h));
        tmpStr += "% RH ";
        tmpStr += String(int(t));
        tmpStr += ".";
        tmpStr += String(int((t-int(t))*10.0+0.5));
        if (TEMPTYPE == 1) {
          tmpStr += "F"; //Fahrenheit
        } else {
          tmpStr += "C"; //Celsius
        }
        kbdWrite(tmpStr);
        Keyboard.press(KEY_LEFT_ALT);
      } else {
        tmpStr = String(keyStrings[key]);
        Keyboard.write(' ');
        Keyboard.press(KEY_LEFT_SHIFT);
        Keyboard.write(KEY_F9);
        Keyboard.release(KEY_LEFT_SHIFT);
        kbdWrite(tmpStr);
      }
      break;
    case 14:
      if (altPressed) {
        Keyboard.press(KEY_TAB);
      } else {
        Keyboard.press(KEY_LEFT_CTRL);
        Keyboard.press('f');
      }
      break;
    case 15:
      if (altPressed) {
        Keyboard.release(KEY_LEFT_ALT);
        Keyboard.write(KEY_ESC);
        tmpStr = String(keyStrings[key]);
        kbdWrite(tmpStr);
        Keyboard.press(KEY_LEFT_ALT);
      } else {
        Keyboard.write(KEY_RETURN);
      }
      break;
    default:
      tmpStr = String(keyStrings[key]);
      kbdWrite(tmpStr);
  }
  if (altPressed == -1) {
    return 1;
  } else {
    return 0;
  }
}

void someMouse() {
  return;
}

void setPadBrightness() {
  padBrightness = map(analogRead(POTPIN),0,1023,0,16);
  if (padBrightness==16) padBrightness=15;
  trellis.setBrightness(padBrightness);
}
 

