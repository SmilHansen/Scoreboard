/* SLAVECODE - LED CTRL || COM 4 */

#define SLAVE_ADR 10 // Adressen der bliver sat på dette modul - denne skal være ens i begge kode-stykker
#define RCLK_LATCH_PIN 4
#define SRCLK_1_PIN 5
#define SRCLK_2_PIN 6
#define SRCLK_3_PIN 7
#define SER_1_DATA_PIN 8
#define SER_2_DATA_PIN 9
#define SER_3_DATA_PIN 10
#define TIME_DELAY 499
#define LEFTMOST true
#define RIGHTMOST false

#include <Wire.h>

int LEDPIN = 13;
int dataBuff[2];
boolean freshData = false;
unsigned long my_time, my_time_diff;
boolean toggel = false;

int ude_point = 0;
int hjemme_point = 0;
int display_time[2] = {0, 0};
boolean time_running = false;

static unsigned long lWaitMillis;

/* Array (tal) der indeholder info om hvilke LED der skal tændes */
/* NB. disse skal stemme overens med tilslutningen af LED */
/* PIN - LAYOUT || NB. PIN 8 ER KOLON */
/*          */
/*   1111   */
/*  6    2  */
/*  6    2  */
/*   3333   */
/*  7    4  */
/*  7    4  */
/*   5555   */
/*          */
/************/

byte tal[11] = {  B01111011, // 0
                  B00001010, // 1
                  B01010111, // 2
                  B00011111, // 3
                  B00101110, // 4
                  B00111101, // 5
                  B01111101, // 6
                  B00001011, // 7
                  B01111111, // 8
                  B00101111, // 9
                  B01110101  // Visuel "E" for "Error"
               };

unsigned int d1, d2;

const int clockPin = 4;   //SRCLK / SH_CP
const int latchPin = 5;   //RCLK  / ST_CP
const int dataPin = 6;    //SER   / DS

void setup() {
  pinMode(RCLK_LATCH_PIN, OUTPUT);
  pinMode(SER_1_DATA_PIN, OUTPUT);
  pinMode(SER_2_DATA_PIN, OUTPUT);
  pinMode(SER_3_DATA_PIN, OUTPUT);
  pinMode(SRCLK_1_PIN, OUTPUT);
  pinMode(SRCLK_2_PIN, OUTPUT);
  pinMode(SRCLK_3_PIN, OUTPUT);
  Serial.begin(9600);
  Serial.println("Begyndt:\n");
  pinMode (LEDPIN, OUTPUT);
  // Start the I2C Bus as Slave with adress SLAVE_ADR
  Wire.begin(SLAVE_ADR);

  lWaitMillis = millis() + TIME_DELAY;  // initial setup

  // Attach a function to trigger when something is received. (has to be last thing, as it can interupt)
  Wire.onReceive(receiveEvent);
}

void loop() {
  if (freshData) {
    registerWrite(dataBuff[0], dataBuff[1], toggel);
    freshData = false;
    toggel = false;
  }

  if ( (long)( millis() - lWaitMillis ) >= 0)
  {
    // millis is now later than my 'next' time
    lWaitMillis += TIME_DELAY - (millis() - lWaitMillis);  // carries when arithmetic causes owerflow!

    toggel = !toggel;

    // kinda nested for-loop, inner part w d2, outer w d1!
    if (toggel) {
      d2++;
      if (d2 >= 10) {
        d1++;
        d2 = 0;
      }
      if (d1 >= 10) {
        d1 = 0;
      }
    }
    registerWrite(d2, d1, toggel);
  }
  else
  {
    // this is the part before the timer-loop is done (!)
  }
  /* COUNT-UP/DOWN TEST
  for (int i = 0; i < 105; i++) {
    Serial.print(10 * display_time[0] + display_time[1]++, DEC);
    ctrl_time_overflow();
    Serial.print(" ");
    delay(10);
  }
  Serial.print("\n\n");
  display_time[0] = 9; display_time[1] = 9;
  for (int i = 99; i > -3; i--) {
    Serial.print(10 * display_time[0] + display_time[1]--, DEC);
    ctrl_time_overflow();
    Serial.print(" ");
    delay(10);
  }
  delay(100000); */

  for (int i = 0; i < 99; i++) {
    hjemmePointToBoard();
    udePointToBoard();
    delay(1000);
    hjemme_point++;
    ude_point++;
  }
}

// This method sends bits to the shift register
void registerWrite(int showNumber1, int showNumber2, boolean toggelBlinker) {
  // turn off the output so the pins don't light up
  // while you're shifting bits:
  digitalWrite(RCLK_LATCH_PIN, LOW);

  // shift the bits out:
  if (toggel) {
    shiftOut(SER_1_DATA_PIN, SRCLK_1_PIN, MSBFIRST, tal[showNumber2] | B10000000 ); // forreste ciffer først
    shiftOut(SER_1_DATA_PIN, SRCLK_1_PIN, MSBFIRST, tal[showNumber1] | B10000000 );
  } else {
    shiftOut(SER_1_DATA_PIN, SRCLK_1_PIN, MSBFIRST, tal[showNumber2] );
    shiftOut(SER_1_DATA_PIN, SRCLK_1_PIN, MSBFIRST, tal[showNumber1] );
  }
  digitalWrite(RCLK_LATCH_PIN, HIGH);
}

void hjemmePointToBoard() {
  // turn off the output so the pins don't light up
  // while you're shifting bits:
  digitalWrite(RCLK_LATCH_PIN, LOW);
  shiftOut(SER_2_DATA_PIN, SRCLK_2_PIN, MSBFIRST, tal[digit_get(hjemme_point, LEFTMOST)]);
  shiftOut(SER_2_DATA_PIN, SRCLK_2_PIN, MSBFIRST, tal[digit_get(hjemme_point, RIGHTMOST)]);
  digitalWrite(RCLK_LATCH_PIN, HIGH);
}

void udePointToBoard() {
  // turn off the output so the pins don't light up
  // while you're shifting bits:
  digitalWrite(RCLK_LATCH_PIN, LOW);
  shiftOut(SER_3_DATA_PIN, SRCLK_3_PIN, MSBFIRST, tal[digit_get(ude_point, LEFTMOST)]);
  shiftOut(SER_3_DATA_PIN, SRCLK_3_PIN, MSBFIRST, tal[digit_get(ude_point, RIGHTMOST)]);
  digitalWrite(RCLK_LATCH_PIN, HIGH);
}
int digit_get(int number, boolean leftmost) { 
  Serial.print("\nNumber: ");
  Serial.print(number, DEC);
  int ret_val = 0;
  if (leftmost) {
    for (ret_val = 0; number >= 10; number -= 10) 
      {ret_val++;} // tæller antal dekrementeringer
  } else {
    while (number >= 10) 
      {number -= 10;} // giver rest efter dekrementeringer
    ret_val = number;
  }
  Serial.print("\nret_val: ");
  Serial.print(ret_val, DEC);  
  if (ret_val < 10 && ret_val >= 0) {
      return ret_val;  
    } else {
      return 10; // Error-code (shows E in display)
    }
}

void receiveEvent(int bytes) {
  dataBuff[1] = dataBuff[0];
  dataBuff[0] = Wire.read();    // read one character from the I2C
  freshData = true;
  
  switch (dataBuff[0]) {
    case 1  :
      Serial.print("PLUS POINT HJEMME - ");
      hjemme_point++;
      hjemmePointToBoard();
      Serial.print(hjemme_point, DEC);
      Serial.print("\n");
      break;

    case 2  :
      Serial.print("MINUS POINT HJEMME - ");
      if (hjemme_point > 0) {
        hjemme_point--;
      }
      hjemmePointToBoard();
      Serial.print(hjemme_point, DEC);
      Serial.print("\n");
      break;

    case 3  :
      Serial.print("MINUS POINT UDE - ");
      if (ude_point > 0) {
        ude_point--;
      }
      udePointToBoard();
      Serial.print(ude_point, DEC);
      Serial.print("\n");
      break;

    case 4  :
      Serial.print("PLUS POINT UDE - ");
      ude_point++;
      udePointToBoard();
      Serial.print(ude_point, DEC);
      Serial.print("\n");
      break;

    case 5  : // START / STOP TID {time_running}
      Serial.print("START/STOP TID - ");
      time_running != time_running; //skift mellem start/stop
      if (time_running) {
        Serial.print("TIDEN KOERER\n");
      } else {
        Serial.print("TIDEN KOERER IKKE\n");
      }
      break;

    case 6  : // START 45 MIN
      Serial.print("TID 45 MIN - TIDEN KOERER\n");
      display_time[0] = 4;
      display_time[1] = 5;
      time_running = true;
      break;

    case 7  : // PLUS 1 MINUT
      Serial.print("TID PLUS - ");
      display_time[1]++;
      ctrl_time_overflow();
      Serial.print(display_time[1] * 10 + display_time[1]);
      Serial.print("\n");
      break;

    case 8  : // MINUS 1 MINUT
      Serial.print("TID MINUS - ");
      display_time[1]--;
      ctrl_time_overflow();
      Serial.print(display_time[1] * 10 + display_time[1]);
      Serial.print("\n");
      break;


    default :
      Serial.print("FORKERT TASTETRYK!\n");
  }
}

void ctrl_time_overflow() { // korrigerer, når der lægges til og trækkes fra i tid, så overflow håndteres
  if (display_time[1] < 0) {
    if (display_time[0] == 0) {
      display_time[1] = 0;
    } else {
      display_time[1] = 9; // roll-'over'
      display_time[0]--;
    } // decrement due to roll-'over'
  }
  if (display_time[1] > 9) {
    display_time[1] = 0; // roll-over
    display_time[0]++; // increment due to roll-over
  }
  if (display_time[0] < 0) {
    display_time[0] = 0;
  }
  if (display_time[0] > 9) {
    display_time[0] = 0;
  }
}

void reset_board() {
  ude_point = 0;
  hjemme_point = 0;
  display_time[0] = 0;
  display_time[1] = 0;
  time_running = false;
}
