#define Ver 1.0

#include <EEPROM.h>


// ------------------------------------------------------------------------------------clearSerialBuffer
void clearSerialBuffer(){
  while(Serial.available()){
    Serial.read();
    delay(50);
  }
}
// ------------------------------------------------------------------------------------header
void header(){
  uint8_t EEpromInt = EEPROM.read(0);
  Serial.print(F("Current value at address (0) is "));
  Serial.println(EEpromInt);
  clearSerialBuffer();
  Serial.print(F("Enter a an unsigned integer >0 and <254: "));
}

// ------------------------------------------------------------------------------------setup
void setup() {
  Serial.begin(115200); Serial.print(F("\n\rAddress As uint8_t ver ")); Serial.println(Ver);
  clearSerialBuffer();
  header();
}
// ------------------------------------------------------------------------------------loop
void loop() {
  
  if (Serial.available()){
    long inInt = Serial.parseInt();
    if (inInt <= 0 || inInt >=254){
      Serial.print(F(" I don't like '"));
      Serial.print(inInt);
      Serial.println(F("'"));
    }
    else {
      EEPROM.write(0,uint8_t(inInt));
    }
    header();
  }
}
