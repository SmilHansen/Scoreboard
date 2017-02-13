/* MASTER || RF || COM 10 */
#define BOARD_INTERRUPT_PIN 3
#define RF_MODULE_PIN 4
#define BAUD_RATE 9600 // bruges til debugging, kommunikationshastighed over USB, skal fjernes i endelige udgave ! <------------------------------------------------------------###
#define SLAVE_ADR 10 // Dette er adressen SLAVE [skal være ens i begge kode-stykker]

#include <Wire.h>

//** Variables, global **//

// opretter et struct
struct buttonStruct
{
  byte one;
  byte two;
  byte three;
};
// laver en datatype af structet
typedef struct buttonStruct button_t;

button_t buttons[11]; // 11, så er 0 ubrugelig og [1] er lig knap 1.

// denne skal fjernes i den endelige udgave, når tastetryk har fået en statisk værdi (evt. en macro '#define key1 00101011010 eller lignende) <---------------------------------###
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
  Serial.begin(BAUD_RATE); // Skal fjernes i den endelige udgave! <-------------------------------------------------------------------------------------------------------------###
  digitalWrite(LED_BUILTIN, HIGH);

  // Attach interrupt (giver nemmere en opfattelse af hvor sendt data starter, men kan laves uden)
  // -- NB det er interrupts der skal bruges hvis man vil have det hele til at køre på én arduino!
  // attachInterrupt(digitalPinToInterrupt(BOARD_INTERRUPT_PIN), signalIncomming, RISING);
  // possible intterupt RISING, CHANGE

  Wire.begin();
}

void signalIncomming() {
  //Read and store the signal into the storedData array
  for (int i = 0; i < dataSize; i = i + 2)
  {
    //Identificer længden af signal der modtages    ---------------HIGH
    dataCounter = 0; //reset the counter
    while (digitalRead(RF_MODULE_PIN) && dataCounter < maxSignalLength) {
      dataCounter++;
    }
    storedData[i] = dataCounter;  //Gem længden af HIGH-signal (talt i clock-cycles)

    //Identificer længden af signal der modtages    ---------------LOW
    dataCounter = 0; //reset the counter
    while (!digitalRead(RF_MODULE_PIN) && dataCounter < maxSignalLength) {
      dataCounter++;
    }
    storedData[i + 1] = dataCounter; //Gem længden af LOW-signal (talt i clock-cycles)
    if (dataCounter >= maxSignalLength) {
      break;
    }
  }
  // NB. Der bør indføres noget der kan identificere det modtagne signal, der er aboslut ingen støjfiltrering pt.
  storedData[0]++;  //Tilføj første signal (der igangsatte loopet), da digitalRead>threshold = blev tabt mens der lyttedes efter signal

  // NB. Bliver arrayet nogensinde 'renset'?

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
      //value = ((!(i % 2)) ? 1 : 0); //er 'i' lige --> value=1; er 'i' ulige --> value=0
      for (j = 0; j < 8; j++)
      {
        // Med given læsehastighed, læses der ca. 75 gange i sekundet, så en bit er ca. 75,
        // derfor skelner vi ved 75/2 =~ 37 og 75+(75/2) =~ 112 osv., om det er en ny bit
        if ( ((37 + (j * 75)) < storedData[i] ) && ((112 + (j * 75)) > storedData[i]) )
        {
          resultData[byteCounter] = (resultData[byteCounter] << 1) | HIGH;
          break; // hopper ud af for-loop, da vi har fundet en bit
        } else {
          resultData[byteCounter] <<= 1;
          break; // hopper ud af for-loop, da vi har fundet en bit [NB. dette er en ændring af originalen] <--------------------------------------------------------------------###
        }
      }
      bitCounter = (bitCounter + 1) % 8;
      if (bitCounter == 0)
      {
        byteCounter++;
      }
    }
  }

  processedData[1] = resultData[1];
  processedData[2] = resultData[2];
  processedData[3] = resultData[3];
  // kan udvides til at returnere en pointer til et array eller struct-datatype... men globale variabler er så nemme!
}

void loop() {
  // Dette skal ændres så der i stedet er inkodet faktiske knappetryk fra fjernbetjeningen <------------------------------------------------------------------------------------###
  if (setup_mode) {
    for (int k = 1; k < 11; k++) {
      Serial.print("Tryk knap ");
      Serial.print(k, DEC);
      Serial.print(" ned: ");
      while (!digitalRead(RF_MODULE_PIN))
      {
        delay(1);  // looking for data
      }
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

    for (int p = 1; p < 11; p++) {
      if ( (processedData[1] == buttons[p].one) && (processedData[2] == buttons[p].two) && (processedData[3] == buttons[p].three) )
      {
        Serial.print("\nDu trykkede ");
        Serial.print(p, DEC);

        sendI2C(p); // sender info om hvilken knap der er blevet trykket på, til den anden arduino!
      }
    }

  }
}

void sendI2C(int dataToSend) {
  Wire.beginTransmission(SLAVE_ADR);  // Sætter modtager (NB. modtageradressen defineres i SLAVE-koden, disse skal stemme overens)
  Wire.write(dataToSend);             // Sender knappetrykket til <LED CTRL> (den anden arduino, vha. I2C)
  Wire.endTransmission();             // Afslutter kommunikationen
}
