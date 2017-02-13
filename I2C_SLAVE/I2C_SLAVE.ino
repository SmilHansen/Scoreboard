// SLAVECODE - LED CTRL || COM 4

#define SLAVE_ADR 10 // Adressen der bliver sat på dette modul - denne skal være ens i begge kode-stykker
#define RCLK_LATCH_PIN 4
#define SRCLK_1_PIN 5
#define SRCLK_2_PIN 6
#define SRCLK_3_PIN 7
#define SER_1_DATA_PIN 8
#define SER_2_DATA_PIN 9
#define SER_3_DATA_PIN 10
#define TIME_DELAY 499 // hvorfor 499? Det tager lidt tid at lave udregningerne, så et delay på ½ sekund (500 ms) er for meget!
#define LEFTMOST true
#define RIGHTMOST false
#define COLON_BIT B10000000 // Den bit der repræsenterer kolon på tids-modulet

#include <Wire.h> // til I2C protokollen

int dataBuff[2];
boolean freshData = false;
unsigned long my_time, my_time_diff;
boolean toggel = false;

int ude_point = 0;
int hjemme_point = 0;
// display_time :: [0] er venstre ciffer, altså tierne. [1] er højre ciffer, altså enerne.
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

void setup() {
  //Definér pins til output
  pinMode(RCLK_LATCH_PIN, OUTPUT);

  pinMode(SER_1_DATA_PIN, OUTPUT);
  pinMode(SER_2_DATA_PIN, OUTPUT);
  pinMode(SER_3_DATA_PIN, OUTPUT);

  pinMode(SRCLK_1_PIN, OUTPUT);
  pinMode(SRCLK_2_PIN, OUTPUT);
  pinMode(SRCLK_3_PIN, OUTPUT);

  Serial.begin(9600);
  Serial.println("Begyndt:\n");
  pinMode (LED_BUILTIN, OUTPUT); //LED_BUILTIN == 13

  // Start I2C Bussen som Slave med adressen angivet SLAVE_ADR
  Wire.begin(SLAVE_ADR);

  lWaitMillis = millis() + TIME_DELAY;  // initial setup

  // Forbind et funktionskald til data-input. -- Denne linje skal være sidste, da den kan interrupte (er en slags ISR [Interrupt Service Routine])
  Wire.onReceive(receiveEvent);
}

void loop() {
  if (freshData) {
    timeOutputToBoard(dataBuff[0], dataBuff[1], toggel);
    freshData = false;
    toggel = false;
  }

  if ( (long)( millis() - lWaitMillis ) >= 0)
  {
    // millis is now later than my 'next' time
    lWaitMillis += TIME_DELAY - (millis() - lWaitMillis);  // carries when arithmetic causes owerflow!

    toggel = !toggel;

    // Er en special-udgave af nested for-loop, indre del med d2, ydre med d1! - selve løkken indeholder reelt hele "loop"-funktionen
    // NB. der er ingen fordel ved at forsøge at pakke det ind i en forløkke, da for-løkken ville producere SAMME ASM-kode!
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
    timeOutputToBoard(d2, d1, toggel);
  }
  else
  {
    // Dette er stykket inden indre loop er færdig (!) - hvis det skal bruges til noget
  }
}

// This method sends bits to the shift register
void timeOutputToBoard(int showNumber1, int showNumber2, boolean toggelBlinker) { 
  // Sluk for output mens der 'shiftes', så LED'erne ikke blinker
  digitalWrite(RCLK_LATCH_PIN, LOW);

  // 'shift' bitsne ud
  if (toggelBlinker) {
    // forreste ciffer først (venstre)
    // reelt er det kun det ene modul der har kolon, men da det andet modul ikke har noget på den pin
    // kan de to moduler uhindret byttes om, hvis COLOR_BIT shiftes ud på begge moduler!
    shiftOut(SER_1_DATA_PIN, SRCLK_1_PIN, MSBFIRST, tal[showNumber2] | COLON_BIT ); 
    shiftOut(SER_1_DATA_PIN, SRCLK_1_PIN, MSBFIRST, tal[showNumber1] | COLON_BIT ); 
  } else {
    shiftOut(SER_1_DATA_PIN, SRCLK_1_PIN, MSBFIRST, tal[showNumber2] );
    shiftOut(SER_1_DATA_PIN, SRCLK_1_PIN, MSBFIRST, tal[showNumber1] );
  }
  digitalWrite(RCLK_LATCH_PIN, HIGH);
}

void hjemmePointToBoard() {
  // Sluk for output mens der 'shiftes', så LED'erne ikke blinker
  digitalWrite(RCLK_LATCH_PIN, LOW);
  // forreste ciffer først (venstre)
  shiftOut(SER_2_DATA_PIN, SRCLK_2_PIN, MSBFIRST, tal[digit_get(hjemme_point, LEFTMOST)]);
  shiftOut(SER_2_DATA_PIN, SRCLK_2_PIN, MSBFIRST, tal[digit_get(hjemme_point, RIGHTMOST)]);
  digitalWrite(RCLK_LATCH_PIN, HIGH);
}

void udePointToBoard() {  
  // Sluk for output mens der 'shiftes', så LED'erne ikke blinker
  digitalWrite(RCLK_LATCH_PIN, LOW);
  // forreste ciffer først (venstre)
  shiftOut(SER_3_DATA_PIN, SRCLK_3_PIN, MSBFIRST, tal[digit_get(ude_point, LEFTMOST)]);
  shiftOut(SER_3_DATA_PIN, SRCLK_3_PIN, MSBFIRST, tal[digit_get(ude_point, RIGHTMOST)]);
  digitalWrite(RCLK_LATCH_PIN, HIGH);
}

int digit_get(int number, boolean leftmost) {
  Serial.print("\nNumber: ");
  Serial.print(number, DEC);
  int ret_val = 0;
  if (leftmost) { 
    //Dette kunne gøres mere elegant med modulo-operator, men det er testet og virker!
    //returnerer 0x, 1x, 2x ... 8x, 9x (altså 'tierne')
    for (ret_val = 0; number >= 10; number -= 10)
    {
      ret_val++; // tæller antal dekrementeringer
    }
  } else { // Hvis ikke 'leftmost', så antages 'rightmost', f.eks. x0, x1, x2 ... x8, x9
    while (number >= 10)
    {
      number -= 10; // giver rest efter dekrementeringer
    }
    ret_val = number;
  }
  
  if (ret_val < 10 && ret_val >= 0) {
    return ret_val;
  } else {
    return 10; // 10 giver error-code (Viser E for Error i 'display')
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
      Serial.print(display_time[0] * 10 + display_time[1]);
      Serial.print("\n");
      break;

    case 8  : // MINUS 1 MINUT
      Serial.print("TID MINUS - ");
      display_time[1]--;
      ctrl_time_overflow();
      Serial.print(display_time[0] * 10 + display_time[1]);
      Serial.print("\n");
      break;

    default :
      Serial.print("FORKERT TASTETRYK!\n");
      break;
  }
}

void ctrl_time_overflow() { 
  // korrigerer, når der lægges til og trækkes fra i tid, så "overflow" håndteres
  // således at flg. gælder:
  // [0][0] - 1 giver 0
  // [9][9] + 1 giver 0
  // [x][9] + 1 giver [x+1][0]
  // [x][0] - 1 giver [x-1][9]
  // funktion er testet og giver ønskede resultater, begge veje!
  if (display_time[1] < 0) {
    if (display_time[0] == 0) {
      display_time[1] = 0;
    } else {
      display_time[1] = 9; // roll-'over' ~ altså i negativ retning
      display_time[0]--;
    } // dekrementér pga. roll-'over' ~ altså i negativ retning
  }
  if (display_time[1] > 9) {
    display_time[1] = 0; // "roll-over"
    display_time[0]++; // inkrementér pga. "roll-over"
  }
  if (display_time[0] < 0) {
    display_time[0] = 0;
  }
  if (display_time[0] > 9) { 
    display_time[0] = 0; // "roll-over"
  }
}

void reset_board() { // denne funktion nulstiller alle display-instillinger
  ude_point = 0;
  hjemme_point = 0;
  display_time[0] = 0;
  display_time[1] = 0;
  time_running = false;
}
