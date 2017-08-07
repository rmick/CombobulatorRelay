#include <IRremote.h>

#define RICHIE_VERSION 1

#ifndef RICHIE_VERSION
    #include <LazerTagIr.h>
#endif



  #define WIFI_SERIAL Serial1       // Wi-Fi
//#define WIFI_SERIAL Serial        // USB

#ifdef RICHIE_VERSION
    #define     TYPE_LAZERTAG_BEACON    6000
    #define     TYPE_LAZERTAG_TAG       3000
#endif

int RECEIVE_PIN = 8;

#define SERIAL_BUFFER_SIZE 64
char serialBuffer[SERIAL_BUFFER_SIZE];
int serialBufferPosition = 0;
long unsigned TimeSinceLast;

#ifdef RICHIE_VERSION
    IRrecv lazerTagReceive(RECEIVE_PIN);
    IRsend lazerTagSend;
#else
    LazerTagIrReceive lazerTagReceive(RECEIVE_PIN);
    LazerTagIrSend lazerTagSend;
#endif

decode_results results;

void setup();
void loop();
void processSignature(decode_results *results);
void readSerial();
void processCommand(char command[]);

void setup()
{
  Serial.begin(115200);
  WIFI_SERIAL.begin(115200);
  lazerTagReceive.enableIRIn();
  lazerTagReceive.blink13(true);

  //These are required foe Arduino Mega only.
  pinMode(9, OUTPUT);
  digitalWrite(9, LOW);

  
  TimeSinceLast = millis();
  Serial.print(F("Here we go...xxx1.."));
  #ifdef RICHIE_VERSION
    Serial.print("New LTTO library");
  #else
    Serial.print("Riley's library");
  #endif
}

void loop()
{  
  if (lazerTagReceive.decode(&results))
  {
    processSignature(&results);
    lazerTagReceive.resume();
  }
  else
  {
    readSerial();
  }
}

void processSignature(decode_results *results)
{
  #ifdef RICHIE_VERSION
    if (results->address != TYPE_LAZERTAG_BEACON && results->address != TYPE_LAZERTAG_TAG)
  #else
    if (results->decode_type != TYPE_LAZERTAG_BEACON && results->decode_type != TYPE_LAZERTAG_TAG)
  #endif
  {
      Serial.print(results->address);
      Serial.print("-RAW ");
      for (int i = 0; i < results->rawlen; i++)
      {
        Serial.print(results->rawbuf[i] * USECPERTICK, DEC);
        if (i < (results->rawlen - 1)) Serial.print(" ");
      }
      Serial.println();
      return;
  }

  Serial.print("\nirRCV ");
  Serial.print(results->value, HEX);
  Serial.print(" ");
  Serial.print(results->bits, HEX);
  Serial.print(" ");
  #ifdef RICHIE_VERSION
    Serial.print(results->address == TYPE_LAZERTAG_BEACON ? 1 : 0);
  #else
    Serial.print(results->decode_type == TYPE_LAZERTAG_BEACON ? 1 : 0);
  #endif
  Serial.println();
  if( (results->bits == 9) && (results->value > 256) ) Serial.println("__________________\n");
  
  WIFI_SERIAL.print("RCV ");
  WIFI_SERIAL.print(results->value, HEX);
  WIFI_SERIAL.print(" ");
  WIFI_SERIAL.print(results->bits, HEX);
  WIFI_SERIAL.print(" ");
  WIFI_SERIAL.print(results->decode_type == TYPE_LAZERTAG_BEACON ? 1 : 0);
  WIFI_SERIAL.println();
}

void readSerial()
{
    while (WIFI_SERIAL.available())
    {
      //Serial.print(".");
      char previousChar;
      if (serialBufferPosition > 0)
      {
        previousChar = serialBuffer[serialBufferPosition - 1];
      }
      
      char thisChar = WIFI_SERIAL.read();
      serialBuffer[serialBufferPosition] = thisChar;
      
      if (previousChar == '\r' && thisChar == '\n')
      {
        serialBuffer[serialBufferPosition - 1] = '\0';
        processSerialLine(serialBuffer);
        serialBufferPosition = 0;
      }
      else
      {
        serialBufferPosition++;
        if (serialBufferPosition >= SERIAL_BUFFER_SIZE) serialBufferPosition = 0;
      }
    }
}

void processSerialLine(char line[])
{
  const char *delimiters = " ";
  
  char *token = strtok (line, delimiters);
  if (token == NULL || strcasecmp(token, "cmd") != 0)
  {
    Serial.print("ERROR Invalid command: ");
    Serial.println(token);
    return;
  }
  
  token = strtok (NULL, delimiters);
  if (token == NULL)
  {
    Serial.println("ERROR Missing command number.");
    return;    
  }
  int command = strtol(token, NULL, 16);

  switch (command)
  {
    case 0x10:
    {
      token = strtok (NULL, delimiters);
      if (token == NULL)
      {
        Serial.println("ERROR Missing data parameter.");
        return;    
      }
      short data = strtol(token, NULL, 16);
      
      token = strtok (NULL, delimiters);
      if (token == NULL)
      {
        Serial.println("ERROR Missing bit count parameter.");
        return;    
      }
      int bitCount = strtol(token, NULL, 16);

      token = strtok (NULL, delimiters);
      if (token == NULL)
      {
        Serial.println("ERROR Missing beacon parameter.");
        return;    
      }
      bool isBeacon = (strtol(token, NULL, 16) != 0);
      
      //lazerTagSend.sendLazerTag(data, bitCount, isBeacon);
      lazerTagSend.sendLTTO(data, bitCount, isBeacon);
    
    Serial.print(F("\nLZ Data sent =\t"));
    Serial.print(data);
    Serial.print(",");
    Serial.print(bitCount);
    Serial.print(",");
    Serial.print(isBeacon);
    if(data < 10) Serial.print("\t");
    Serial.print(" -\t");
    long unsigned CurrentTime = millis();
    Serial.print(CurrentTime-TimeSinceLast);
    Serial.print("mS");
    if (bitCount == 9 && data > 256) Serial.print("\n");
    TimeSinceLast = millis();

      lazerTagReceive.enableIRIn();
      lazerTagReceive.resume();
  
      break;
    }
    default:
      Serial.print("ERROR Invalid command number: ");
      Serial.println(command, HEX);
  }
}
