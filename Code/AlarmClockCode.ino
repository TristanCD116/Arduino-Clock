#include <LCD_I2C.h>
// Include the LCD_I2C library to interface with the LCD using I2C communication
// See "LCD_I2C.h" documentation

#define BUTTON_PRESSED HIGH
#define LCD_COLUMNS 16
#define LCD_ROWS 2
#define DEBOUNCE_DELAY 125
#define MESSAGE_DELAY 1000
#define TONE_DELAY 100
// The compiler will replace references to these constants with the defined value at compile time

const int alarmPin = 8;
const int togglePin = 9;
const int incrementPin = 10;
const int piezoPin = 11;
// Define pin numbers for alarm control, mode toggle, minute/hour increment, and piezo buzzer

const long millisInMin = 60000;
const int minInHrs = 60;
const int hrsInDay = 24;
// Define time conversion constants: milliseconds in a minute, minutes in an hour, hours in a day

const byte lineR[8] = {
  B00111,
  B00111,
  B00111,
  B00111,
  B00111,
  B00111,
  B00111,
  B00111
};

const byte lineL[8] = {
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100
};

const byte lineU[8] = {
  B11111,
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

const byte lineD[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111
};

const byte lineM[8] = {
  B11111,
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111
};

const byte dot[8] = {
  B00000,
  B00000,
  B00100,
  B01110,
  B01110,
  B00100,
  B00000,
  B00000
};

const byte alarm[8] = {
  B00000,
  B01110,
  B10101,
  B10111,
  B10001,
  B01110,
  B00000,
  B00000
};

const byte null[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};
// Custom characters for seven segment display lcd
// Note: Each number in the byte array represents a single pixel on the LCD, allowing us to design custom segments 

enum modes { NORMAL_MODE, // The different setting modes offered by the UI
             SETTING_HRS,
             SETTING_MIN,
             CONFIRM };

modes settingMode = NORMAL_MODE;  // // Initialize the mode to NORMAL_MODE, which displays the current time

unsigned long previousTime = 0; // Global variable to keep track of the last time update used in the loop function

int min, hrs; // Variables to store current minutes and hours
int alarmMin, alarmHr, settingMin, settingHr; // Variables for alarm and settings

bool alarmOn = false; // Flag to indicate if the alarm is on

LCD_I2C lcd(0x27, LCD_COLUMNS, LCD_ROWS); // // Create an LCD object with the I2C address 0x27 and specified dimensions

void setup() {
  Serial.begin(9600); // Initialize serial communication for debugging

  lcd.begin(); // Initialize the LCD
  lcd.backlight(); // Turn on the backlight
  lcd.leftToRight(); // Set text direction to left-to-right
  lcd.setCursor(0, 0); // Set cursor to the top-left corner

  lcd.createChar(0, lineR);
  lcd.createChar(1, lineL);
  lcd.createChar(2, lineU);
  lcd.createChar(3, lineD);
  lcd.createChar(4, lineM);
  lcd.createChar(5, dot);
  lcd.createChar(6, alarm);
  lcd.createChar(7, null);
  // load the custom characters
  
  pinMode(togglePin, INPUT);
  pinMode(incrementPin, INPUT);
  pinMode(alarmPin, INPUT);
  // define the input pins for the clock's buttons 

  alarmMin, alarmHr = 0;
  settingMin, settingHr = 0;
  // Initialize alarm and setting times to 0

  lcdWrite(0, 0); // Display the initial time on the LCD
}

void loop() {

  unsigned long timeElapsed = millis() - previousTime; 
  // Calculate the time elapsed since the last update

  if (timeElapsed >= millisInMin) {
    previousTime = millis();
    min++;

    if (min >= minInHrs) {
      hrs++;
      min = 0;

      if (hrs >= hrsInDay) {
        hrs = 0;
      }
    }
    if (settingMode == NORMAL_MODE) {
      lcdWrite(min, hrs); // Print the current time
    }
  }
  settings(); // Handle time and alarm settings
  alarmFunctions(); // Check and trigger alarm if necessary

  /* 
  The millis() function returns the number of milliseconds since the Arduino board began running the current program.
  It wraps around every ~49.7 days. To handle this, we calculate the time elapsed based on the difference between the current
  and previous millis() values.
  */
}

void lcdWrite(int minutes, int hours) {

  char buffer[8]; // character array

  sprintf(buffer, "%02d:%02d", hours, minutes);
  // Covert the current minutes and hours into a zero padded string of format "hh:mm" andd store in buffer
  lcd.clear(); // Clear the LCD display

  for (int i = 0; i <= 6; i++) {

    switch (buffer[i]) {
      case '0':
        digitPrint(i, 0, 3, 1, 0, 2, 1);
        break;

      case '1':
        digitPrint(i, 7, 0, 1, 7, 0, 1);
        break;

      case '2':
        digitPrint(i, 0, 3, 3, 2, 4, 1);
        break;

      case '3':
        digitPrint(i, 3, 3, 1, 2, 4, 1);
        break;

      case '4':
        digitPrint(i, 7, 7, 1, 0, 3, 1);
        break;

      case '5':
        digitPrint(i, 3, 3, 1, 0, 4, 2);
        break;

      case '6':
        digitPrint(i, 0, 4, 1, 0, 2, 7);
        break;

      case '7':
        digitPrint(i, 7, 7, 1, 2, 2, 1);
        break;

      case '8':
        digitPrint(i, 0, 3, 1, 0, 4, 1);
        break;

      case '9':
        digitPrint(i, 7, 7, 1, 0, 4, 1);
        break;

      case ':':
        digitPrint(i, 7, 5, 7, 7, 5, 7);
        break;
    }
  }
  // Iterate through the char array and print the custom character each element corresponds to on the LCD. 
}

void digitPrint(int i, int seg1, int seg2, int seg3, int seg4, int seg5, int seg6) {

  const int digitSize = 3; // Size of each digit on the LCD (3 columns)

  lcd.setCursor(i * digitSize, 1);  // Set cursor position for the top row of the digit
  lcd.write(seg1);
  lcd.write(seg2);
  lcd.write(seg3);

  lcd.setCursor(i * digitSize, 0);  // Set cursor position for the bottom row of the digit
  lcd.write(seg4);
  lcd.write(seg5);
  lcd.write(seg6);
  // Print the segments of the given symbols. 
}


void settings() {

  if (digitalRead(togglePin) == BUTTON_PRESSED) {
    while (digitalRead(togglePin) == BUTTON_PRESSED)
      ; // Wait for button release

    settingMode = static_cast<modes>(settingMode + 1); // Switch to the next setting mode
    lcdWrite(settingMin, settingHr);

    if (settingMode > CONFIRM) {  // Return to NORMAL_MODE if beyond CONFIRM
      settingMode = NORMAL_MODE;
    }
  }

  if (settingMode != NORMAL_MODE) {

    if (settingMode == SETTING_HRS) {
      lcd.setCursor(6, 0);
      lcd.print('<'); // Indicate hour setting

      if (digitalRead(incrementPin) == BUTTON_PRESSED) {
        delay(DEBOUNCE_DELAY); // Play a short tone to indicate button press
        settingHr++; // Wait for debounce period

        if (settingHr >= hrsInDay) {
          settingHr = 0; // Wrap around to 0 if hours exceed 24
        }
        lcdWrite(settingMin, settingHr); 
      }
    }

    else if (settingMode == SETTING_MIN) {
      lcd.setCursor(15, 0);
      lcd.print('<'); // Indicate hour setting

      if (digitalRead(incrementPin) == BUTTON_PRESSED) {
        tone(piezoPin, 3000, 20);
        delay(DEBOUNCE_DELAY);
        settingMin++; // Wrap around to 0 if minutes exceed 60

        if (settingMin >= minInHrs) {
          settingMin = 0;
        }
        lcdWrite(settingMin, settingHr); // Update LCD with new settings
      }
    }

    else if (settingMode == CONFIRM) {
      // Confirmation mode where user can finalize or cancel settings
      lcd.clear();
      lcd.print("CONFIRM AS:    ");
      lcd.setCursor(0, 1);
      lcd.println("-ALARM -TIME -EX");

      while (1) {
        // wait for user input 
        if (digitalRead(togglePin) == BUTTON_PRESSED) {
          // Confirm and set the time
          hrs = settingHr;
          min = settingMin;

          settingMode = NORMAL_MODE; // Return to normal mode

          lcd.clear();
          lcd.print(" TIME ADJUSTED!"); // Display message for a short period
          delay(MESSAGE_DELAY);
          lcdWrite(min, hrs); // Update LCD with the new time
          break;
        }

        else if (digitalRead(alarmPin) == BUTTON_PRESSED) {
          // Set the alarm
          alarmHr = settingHr;
          alarmMin = settingMin;
          settingMode = NORMAL_MODE; // Return to normal mode

          lcd.clear();
          lcd.print("   ALARM SET!"); // display message as before
          delay(MESSAGE_DELAY);
          lcdWrite(min, hrs);
          break;
        }

        else if (digitalRead(incrementPin) == BUTTON_PRESSED) {
          settingMode = NORMAL_MODE;
           // Cancel the setting

          lcd.clear();
          lcd.print("   CANCELLED!  "); // display message as before
          delay(MESSAGE_DELAY);
          lcdWrite(min, hrs);
          break;
        }
        delay(DEBOUNCE_DELAY); // Debounce delay for the button to avoid double presses
      }
    }
  } else {
    // If not in setting mode, update the settings with current time, ready to be adjusted if need be
    settingHr = hrs;
    settingMin = min;
  }
}

void alarmFunctions() {

  if (digitalRead(alarmPin) == BUTTON_PRESSED && settingMode == NORMAL_MODE) {
    while (digitalRead(alarmPin) == BUTTON_PRESSED)
      ; // Wait for button release
    alarmOn = !alarmOn; // Toggle alarm state

    if (alarmOn == true) {
      lcdWrite(alarmMin, alarmHr); // Display alarm time
      delay(MESSAGE_DELAY); 
      lcdWrite(min, hrs); 
    }
  }

  if (alarmOn == true && settingMode == NORMAL_MODE) {
    // Indicate if the alarm is active
    lcd.setCursor(16, 2);
    lcd.write(6); // Display custom alarm character
  }

  else {
    lcd.setCursor(16, 2);
    lcd.write(" "); // Clear alarm indicator if alarm is off
  }

  if (alarmMin == min && alarmHr == hrs && alarmOn == true && settingMode == NORMAL_MODE) {
     // Trigger the alarm if the current time matches the alarm time
    tone(piezoPin, 2000, 20); // Play alarm tone
    delay(TONE_DELAY); // Wait for tone duration
  }
}