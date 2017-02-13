/* MASTER || RF || COM 10 */

#define BOARD_INTERRUPT_PIN 3
#define RF_MODULE_PIN 4
#define BAUD_RATE 9600
#define SLAVE_ADR 10 // Dette er adressen SLAVE [skal være ens i begge kode-stykker]

#include <Wire.h>

//** Variables, global **//

struct buttonStruct
{
  byte one;
  byte two;
  byte three;
};

typedef struct buttonStruct button_t;

byte bitVal = 0; //bitvalue på pin BOARD_INTERRUPT_PIN
int printCount = 8;
int pos;
byte readByte;
byte incomingByte;
int byteCounter = 21;
byte incData[255];
byte dataLocation;
boolean interrupted = false;
int loopCounter = 0;
button_t buttons[11]; // 11, så er 0 ubrugelig og [1] er lig knap 1.

boolean setup_mode = true;

byte processedData[4];

//** variabler til behandling af signal fra fjernbetjening **//

const int dataSize = 320;  //modtagersignalet på nederste knapper er de længste, på ca. 319 sekvenser.
unsigned int storedData[dataSize];  //Create an array to store the data
unsigned int dataCounter = 0;    //Variable to measure the length of the signal
unsigned int maxSignalLength = 65535;   //Set the maximum length of the signal

//** variabler til debugging **//

unsigned long my_time;
unsigned long timeDiff;

void setup() {
  // Initial setup, pins used & pin-mode
  pinMode(RF_MODULE_PIN, INPUT);
  pinMode(BOARD_INTERRUPT_PIN, INPUT); // som input, med intern pull-up resistor aktiveret
  pinMode(LED_BUILTIN, OUTPUT);

  // Connect the Arduino to the USB
  Serial.begin(BAUD_RATE);
  digitalWrite(LED_BUILTIN, HIGH);

  // Attach interrupt (giver nemmere opfattelse af hvor sendt data starter, men kan laves uden)
  // attachInterrupt(digitalPinToInterrupt(BOARD_INTERRUPT_PIN), signalIncomming, RISING);
  // possible intterupt RISING, CHANGE

  Wire.begin(); 
}

void signalIncomming() {
  //Read and store the signal into the storedData array
  for (int i = 0; i < dataSize; i = i + 2)
  {
    //Identify the  length of the HIGH signal---------------HIGH
    dataCounter = 0; //reset the counter
    while (digitalRead(RF_MODULE_PIN) && dataCounter < maxSignalLength) {
      dataCounter++;
    }
    storedData[i] = dataCounter;  //Store the length of the HIGH signal

    //Identify the length of the LOW signal---------------LOW
    dataCounter = 0; //reset the counter
    while (!digitalRead(RF_MODULE_PIN) && dataCounter < maxSignalLength) {
      dataCounter++;
    }
    storedData[i + 1] = dataCounter; //Store the length of the LOW signal
    if (dataCounter >= maxSignalLength) {
      break;
    }
  }

  storedData[0]++;  //Account for the first digitalRead>threshold = lost while listening for signal
  /*
    for (int i = 0; i < dataSize && storedData[i]; i++)
    {
    Serial.print(" ");
    Serial.print(storedData[i]);
    Serial.print("(");
    Serial.print(i);
    Serial.print(")");
    }
    Serial.print("\n"); */
  sequenceCleaner();
}

void sequenceCleaner() {
  int i, j, value;
  byte tal;
  byte resultData[512];
  int bitCounter = 1;
  int byteCounter = 1;
  for (i = 0; i < 23; i++)
  {
    if (storedData[i] < maxSignalLength)
    {
      value = ((!(i % 2)) ? 1 : 0);
      for (j = 0; j < 8; j++)
      {
        if ( ((37 + (j * 75)) < storedData[i] ) && ((112 + (j * 75)) > storedData[i]) )
        {
          //Serial.print(value);
          resultData[byteCounter] = (resultData[byteCounter] << 1) | HIGH;
          break;
        } else {
          //Serial.print(value);
          resultData[byteCounter] <<= 1;
        }
      }
      bitCounter = (bitCounter + 1) % 8;
      if (bitCounter == 0)
      {
        byteCounter++;
      }
      //Serial.print(":");
    }
  } /*
  for (i = 0; i < byteCounter ; i++)
  {
    Serial.print(" ");
    Serial.print(resultData[i], DEC);
  } */

  processedData[1] = resultData[1];
  processedData[2] = resultData[2];
  processedData[3] = resultData[3];

  /* Serial.print("\n"); */
}

void loop() {
  
  if (setup_mode) {
    for (int k = 1; k < 11; k++) {
      Serial.print("Tryk knap ");
      Serial.print(k, DEC);
      Serial.print(" ned: ");
      while (!digitalRead(RF_MODULE_PIN))
      { } // looking for data
      signalIncomming();

      buttons[k].one = processedData[1];
      buttons[k].two = processedData[2];
      buttons[k].three = processedData[3];

      Serial.print("\nknap ");
      Serial.print(k, DEC);
      Serial.print(" er: ");
      Serial.print(buttons[k].one, DEC);
      Serial.print(" ");
      Serial.print(buttons[k].two, DEC);
      Serial.print(" ");
      Serial.print(buttons[k].three, DEC);
      Serial.print("\n");

      delay(1000); // 1 sec. delay
    }

    setup_mode = false;

  } else {
    while (!digitalRead(RF_MODULE_PIN)) // stay and wait (!) - ingen interrupt
    { } // looking for data
    signalIncomming();

    for (int l = 1; l < 11; l++) {
      if ( (processedData[1] == buttons[l].one) && (processedData[2] == buttons[l].two) && (processedData[3] == buttons[l].three) )
      {
        Serial.print("\nDu trykkede ");
        Serial.print(l, DEC);

        sendI2C(l);
      }
    }

  }
}

void sendI2C(int dataToSend) {
  Wire.beginTransmission(SLAVE_ADR);
  Wire.write(dataToSend);             // sends the keypress to <LED CTRL> (the other arduino)
  Wire.endTransmission();             // stop transmitting
}








