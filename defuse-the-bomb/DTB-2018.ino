/*
   DEFUSE-THE-BOMB

   Arduino Mega sketch to control a bomb defusing game, inspired by DEFCON game.
   Created for Sorint Download event Sep-2018

   The bomb is made of two pieces (that will be referred in the code):
   - Souriau device
   - Autodial device

   by red5heep

   boom LED codes:
    1: wrong order
    2: time over
    3: something has been messed up
    4: tilt sensor
    5: light sensor
    6: power removed
    7: magnet

   $Id: DTB-2018.ino,v 1.20 2018/09/16 11:15:44 cesare Exp cesare $
*/

#include "music.h"

//
// Global settings
//
int debug = 0;       // NOTE: with debug level > 1, the timing can be impacted; effects on rotary disc and magnet read
//       are expected

int steps = 7;
int steps_ok = 0;

bool generalStatus = false;
bool tiltStatus = false;
bool magneticStatus = false;
bool lightStatus = false;

bool action[7] = { false, false, false, false, false, false, false };

// Time variables
unsigned long startTime;
unsigned long maxTime = 300000;   // milliseconds

// Power variables
const int checkPowerPin = A0;
const int powerLimit = 850;

// Beep variables
const int piezoPin = 8;
unsigned long previousBeep = 0;
unsigned long beepPause = 2000;

// Bomb LEDs
const int redLedPin = 45;
const int yellowLedPin = 46;
const int greenLedPin = 47;

// Autodial LEDs
const int busyLineLedPin = 34;
const int freeLineLedPin = 35;
const int wifiLedPin = 36;

// Step 1 variables
const int switch1Pin = 22;
const int switch1LedPin = 23;

// Step 2 variables
const int switch2Pin = 27;

// Step 3 variables
const int switch3Pin = 28;

// Step 4 variables
const int switch4Pin = 30;
const int lightSensorPin = A3;
const int lightLimit = 100;

// Step 5 variables
const int magneticSwitchPin = 31;
const int magneticCountMax = 12000;
int magneticCount = 0;
bool previousMagStatus = false;


// Step 6 variables
const int switch6Pin = 37;
const int wifiBoardPin = 53;

// Step 7 variables
const int switch7Pin = 38;

// Step 8 variables
const int switch8Pin = 40;
char safeNumber[] = "035112233";
char calledNumber[] = "\0\0\0\0\0\0\0\0\0\0";
int numberIndex = 0;

// Tilt variables
const int tilt1Pin = 24;            // Souriau
const int tilt2Pin = 26;            // Souriau
const int tilt3Pin = 52;            // MCU
const int tilt4Pin = 32;            // Autodial
const int tilt5Pin = 33;            // Autodial
const int tiltCountMax = 30;
int tiltCount = 0;
bool previousTiltStatus = false;

// Unused switches
const int souSelectorPin = 29;
const int souAnalogKnob1 = A1;
const int souAnalogKnob2 = A2;
const int autodialUnusedPin = 2;
const int autodialAnalogKnob1 = A4;
const int autodialAnalogKnob2 = A5;

void setup() {
  Serial.begin(19200);
  if ( debug >= 1 )
  {
    Serial.println("\n\n\nInitializing bomb...");
  }

  startTime = millis();

  // Tilt sensors
  pinMode(tilt1Pin, INPUT);
  pinMode(tilt2Pin, INPUT);
  pinMode(tilt3Pin, INPUT);
  pinMode(tilt4Pin, INPUT);
  pinMode(tilt5Pin, INPUT);

  // Autodial LEDs
  pinMode(busyLineLedPin, OUTPUT);
  pinMode(freeLineLedPin, OUTPUT);
  pinMode(wifiLedPin, OUTPUT);
  digitalWrite(busyLineLedPin, HIGH);
  digitalWrite(freeLineLedPin, LOW);
  digitalWrite(wifiLedPin, LOW);

  // Step 1
  pinMode(switch1Pin, INPUT);
  pinMode(switch1LedPin, OUTPUT);
  digitalWrite(switch1LedPin, HIGH);
  delay(200);
  digitalWrite(switch1LedPin, LOW);

  // Step 2
  pinMode(switch2Pin, INPUT);

  // Step 3
  pinMode(switch3Pin, INPUT);

  // Step 4
  pinMode(switch4Pin, INPUT);

  // Step 5
  pinMode(magneticSwitchPin, INPUT);

  // Step 6
  pinMode(switch6Pin, INPUT);
  pinMode(wifiBoardPin, OUTPUT);
  digitalWrite(wifiBoardPin, HIGH);

  // Step 7
  pinMode(switch7Pin, INPUT);

  // Step 8
  pinMode(switch8Pin, INPUT);

  // Initialize other Pins
  pinMode(souSelectorPin, INPUT);
  pinMode(autodialUnusedPin, INPUT);

  // Init bomb brain LEDs
  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(yellowLedPin, OUTPUT);

  digitalWrite(redLedPin, HIGH);
  delay(500);
  digitalWrite(redLedPin, LOW);
  digitalWrite(yellowLedPin, HIGH);
  delay(500);
  digitalWrite(yellowLedPin, LOW);
  digitalWrite(greenLedPin, HIGH);
  delay(500);
  digitalWrite(greenLedPin, LOW);

  // TODO: Test all the sensors/functions before starting

  digitalWrite(yellowLedPin, HIGH);
}

void loop() {
  unsigned long currentTime = millis();

  // Manage beep
  if ( ( currentTime - previousBeep ) > beepPause )
  {
    if ( magneticStatus == false )
    {
      // Normal beep
      tone(piezoPin, 5000, 100);
    }
    else
    {
      // Come on with that magnet
      tone(piezoPin, 8000, 100);
    }

    previousBeep = currentTime;
    Serial.println("beep");
  }

  // Manage remaining time
  if ( ( currentTime - startTime ) > maxTime )
  {
    // Too late guys, bye bye
    boom("too late, time is over", 2);
  }

  // Check Power Source
  if ( analogRead(checkPowerPin) < powerLimit )
  {
    boom("Don't touch the power plug", 6);
  }

  // *** Start sensor check sequence ***
  action[0] = checkSwitch_step1();
  action[1] = checkSwitch_step2();
  action[2] = checkSwitch_step3();
  action[3] = checkSwitch_step4();
  action[4] = checkSwitch_step6();
  action[5] = checkSwitch_step7();
  action[6] = checkSwitch_step8();

  // *** End sensor check sequence ***

  // *** Start status sensor checks ***
  if ( action[5] != true )                   // Sensor check is disabled when "Line release" button is pressed
                                             // This makes the rotary disc dial read more reliable
  {
    lightStatus = checkLightSensor();        // Part of step 4
    magneticStatus = checkMagSensor();       // This is step 5

    tiltStatus = checkTiltSensors();
    generalStatus = checkUnusedSwitches();

    // Check the general bomb status
    if ( generalStatus == true )
    {
      boom("You messed up something", 3);
    }

    // Check the status of the magnetic switch
    if ( magneticStatus != previousMagStatus )
    {
      // If status changes, reset the counter
      magneticCount = 0;
    }
    if ( magneticStatus == true )
    {
      magneticCount++;

      if ( magneticCount >= magneticCountMax )
      {
        boom("Be faster with that magnet", 7);
      }

      previousMagStatus = magneticStatus;
    }

    // Check the tilt sensors
    if ( tiltStatus != previousTiltStatus )
    {
      // If status changes, reset the counter
      tiltCount = 0;
    }
    if ( tiltStatus == true )
    {
      tiltCount++;
      if ( tiltCount >= tiltCountMax )
      {
        boom("Be careful, it's a bomb", 4);
      }

      previousTiltStatus = tiltStatus;
    }

    // Check the light sensor
    if (lightStatus == true )
    {
      boom("Too much light", 5);
    }
  }
  // *** End status sensor checks

  // Verify current situation for defusing sequence
  steps_ok = 0;

  for (int i = 0; i < steps; i++ )
  {
    if ( action[i] == true && i > steps_ok )
    {
      boom("Wrong order...", 1);
    }

    if ( action[i] == true && i <= steps_ok )
    {
      steps_ok += 1;
      if ( debug >= 2 )
      {
        Serial.println(".");
      }

      if ( steps_ok == 7 )
      {
        defused();
      }
    }
  }
}

// Explode!
void boom(const char *message, int boomCode)
{
  if ( debug >= 1 )
  {
    Serial.print("BOOOOOOM: ");
    Serial.println(message);
  }

  digitalWrite(yellowLedPin, LOW);
  digitalWrite(redLedPin, HIGH);

  tone(piezoPin, 5000, 10000);
  // Pause forever

  while ( 1 == 1 )                // FIXME
  {
    for ( int i = 0; i < boomCode; i++ )
    {
      digitalWrite(yellowLedPin, HIGH);
      delay(500);
      digitalWrite(yellowLedPin, LOW);
      delay(500);
    }
    delay(3000);
  }
}

// Defused!
void defused(void)
{
  if ( debug >= 1 )
  {
    Serial.println("DEFUSED!");
  }

  digitalWrite(yellowLedPin, LOW);
  digitalWrite(greenLedPin, HIGH);

  // Pause forever
  while ( 1 == 1 )
  {
    playMusic();
    delay(3000);
  }
}

// Check function generic button
bool checkSwitch_step1(void)
{
  int switchState;

  // Read the status of switch
  switchState = digitalRead(switch1Pin);
  if ( debug >= 2 )
  {
    Serial.print("Switch Step 1:");
    Serial.println(switchState);
  }

  if ( switchState == HIGH )
  {
    // Turn on LED
    digitalWrite(switch1LedPin, HIGH);

    return true;
  } else {
    // Turn off LED
    digitalWrite(switch1LedPin, LOW);

    return false;
  }
}

bool checkSwitch_step2(void)
{
  int switchState;

  // Read the status of switch
  switchState = digitalRead(switch2Pin);
  if ( debug >= 2 )
  {
    Serial.print("Switch Step 2:");
    Serial.println(switchState);
  }

  if ( switchState == HIGH )
  {
    return true;
  } else {
    return false;
  }
}

bool checkSwitch_step3(void)
{
  int switchState;

  // Read the status of switch
  switchState = digitalRead(switch3Pin);
  if ( debug >= 2 )
  {
    Serial.print("Switch Step 3:");
    Serial.println(switchState);
  }

  if ( switchState == HIGH )
  {
    return true;
  } else {
    return false;
  }
}

bool checkSwitch_step4(void)
{
  int switchState;

  // Read the status of switch
  switchState = digitalRead(switch4Pin);
  if ( debug >= 2 )
  {
    Serial.print("Switch Step 4:");
    Serial.println(switchState);
  }

  if ( switchState == HIGH )
  {
    return false;
  } else {
    return true;
  }
}

bool checkSwitch_step6(void)
{
  int switchState;

  // Read the status of switch
  switchState = digitalRead(switch6Pin);
  if ( debug >= 2 )
  {
    Serial.print("Switch Step 6:");
    Serial.println(switchState);
  }

  if ( switchState == HIGH )
  {
    digitalWrite(wifiLedPin, LOW);            // Turn off LED
    digitalWrite(wifiBoardPin, HIGH);         // Turn off Wifi board
    return false;
  } else {
    digitalWrite(wifiLedPin, HIGH);          // Turn on LED
    digitalWrite(wifiBoardPin, LOW);         // Turn on Wifi board
    return true;
  }
}

bool checkSwitch_step7(void)
{
  int switchState;

  // Read the status of switch
  switchState = digitalRead(switch7Pin);
  if ( debug >= 2 )
  {
    Serial.print("Switch Step 7:");
    Serial.println(switchState);
  }

  if ( switchState == HIGH )
  {
    digitalWrite(busyLineLedPin, HIGH);
    digitalWrite(freeLineLedPin, LOW);
    numberIndex = 0;        // Reset the counter of the number dialed with rotary disc
    return false;
  } else {
    digitalWrite(busyLineLedPin, LOW);
    digitalWrite(freeLineLedPin, HIGH);
    return true;
  }
}

bool checkSwitch_step8(void)
{
  // This is the check for the rotary dial
  int switchState = 999;

  // Read the status of switch
  switchState = readDialDisc();
  if ( debug >= 2 )
  {
    Serial.print("Switch Step 8:");
    Serial.println(switchState);
  }

  if ( switchState != 999 )
  {
    // Check if the previous step has been completed
    if ( action[5] != true )
    {
      // Go away and die!
      return true;
    }
    calledNumber[numberIndex] = switchState + 0x30;
    numberIndex++;

    if ( numberIndex == strlen(safeNumber) )
    {
      numberIndex = 0;

      // Check if the number is correct
      if ( strncmp(calledNumber, safeNumber, strlen(safeNumber)) == 0 )
      {
        // Great job!!
        return true;
      }
      else
      {
        boom(" You called the Wrong Number", 3);
      }
    }
  }
  return false;
}

// Check status of light sensor
bool checkLightSensor(void)
{
  int switchState;

  // Read the status of switch
  switchState = analogRead(lightSensorPin);
  if ( debug >= 2 )
  {
    Serial.print("Light Sensor:");
    Serial.println(switchState);
  }

  if ( switchState > lightLimit )
  {
    return true;
  } else {
    return false;
  }
}

// Check status of magnetic sensor
bool checkMagSensor(void)
{
  int switchState;

  // Read the status of switch
  switchState = digitalRead(magneticSwitchPin);
  if ( debug >= 2 )
  {
    Serial.print("Magnetic Sensor:");
    Serial.println(switchState);
  }

  if ( switchState == HIGH )
  {
    return true;
  } else {
    return false;
  }
}

// Check status of tilt sensors
bool checkTiltSensors(void)
{
  if ( digitalRead(tilt1Pin) == 1 )
  {
    return true;
  }
  if ( digitalRead(tilt2Pin) == 1 )
  {
    return true;
  }
  if ( digitalRead(tilt3Pin) == 1 )
  {
    return true;
  }
  if ( digitalRead(tilt4Pin) == 1 )
  {
    return true;
  }
  if ( digitalRead(tilt5Pin) == 1 )
  {
    return true;
  }

  return false;
}

// Check status of unused switches
bool checkUnusedSwitches(void)
{
  int switchState;

  // *** Read the status of Souriau Selector ***
  switchState = digitalRead(souSelectorPin);
  if ( debug >= 2 )
  {
    Serial.print("Sou Selector Pin:");
    Serial.println(switchState);
  }
  if ( switchState == HIGH )
  {
    return true;
  }

  // *** Read the status of Souriau Analog Knob 1 ***
  switchState = analogRead(souAnalogKnob1);
  if ( debug >= 2 )
  {
    Serial.print("Sou Analog Knob 1:");
    Serial.println(switchState);
  }

  if ( switchState > 520 )
  {
    return true;
  }

  // *** Read the status of Souriau Analog Knob 2 ***
  /*
   * FIXME: this knob physically broken during the event, I had to disable it
   * 
  switchState = analogRead(souAnalogKnob2);
  if ( debug >= 2 )
  {
    Serial.print("Sou Analog Knob 2:");
    Serial.println(switchState);
  }
  if ( switchState > 500 )
  {
    return true;
  }
  */

  // *** Read the status of Autodial unused switches ***
  switchState = digitalRead(autodialUnusedPin);
  if ( debug >= 2 )
  {
    Serial.print("Autodial unused switches:");
    Serial.println(switchState);
  }
  if ( switchState == HIGH )
  {
    return true;
  }

  // *** Read the status of Autodial Analog Knob 1 ***
  switchState = analogRead(autodialAnalogKnob1);
  if ( debug >= 2 )
  {
    Serial.print("Autodial Analog Knob 1:");
    Serial.println(switchState);
  }

  if ( switchState < 1000 )
  {
    return true;
  }

  // *** Read the status of Autodial Analog Knob 2 ***
  switchState = analogRead(autodialAnalogKnob2);
  if ( debug >= 2 )
  {
    Serial.print("Autodial Analog Knob 2:");
    Serial.println(switchState);
  }

  if ( switchState < 1000 )
  {
    return true;
  }

  return false;
}

// Read the rotary phone dial disc
int readDialDisc(void)
{
  int count = 0;
  int waitAWhile = 0;
  int previousRead = 0;

  int reading = digitalRead(switch8Pin);

  while ( reading != previousRead )
  {
    if ( reading == HIGH )
    {
      count++;
    }
    previousRead = reading;
    // Wait until situaion change
    while ( ( reading = digitalRead(switch8Pin) ) == previousRead && waitAWhile < 75 )
    {
      waitAWhile++;
      delay(1);
    }
    waitAWhile = 0;
  }

  if ( count != 0 )
  {
    // Number has been selected
    if ( debug >= 1 )
    {
      Serial.print("Rotary disc Number: ");
      Serial.println(count % 10);
    }

    // Mod 10 because 0 is 10 pulses
    return (count % 10);
  }

  // No numbers dialed
  return (999);
}
