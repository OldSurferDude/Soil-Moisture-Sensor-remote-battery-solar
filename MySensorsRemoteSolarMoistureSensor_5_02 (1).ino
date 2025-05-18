#define VER 5.02
#define SKETCH_NAME "RSMoistureSensor"
// Setting Node ID from EEPROM ... done by My Sensors
#include <EEPROM.h>
// get NODE ID so it can be presented before MySensors starts
unsigned short xMY_NODE_ID = EEPROM.read(0);

//#define DEV

/* 
ver 5.02
  5.00 does not register sensors 5.01 defines MY_DEBUG which turns on print
    this turns off all print statements
  Increased message wait to 500msec
Ver 5.00
    turn off Debug statemens by not defining MY_DEBUG
ver 4.05
  remove wns button because it wasn't used
  Store interval in NVRAM
  turn off Debug
ver 4.01
  The moist % calculation had a mismatch between the max and min numbers
  cleaned up debug message
Ver 4.00
  It appears that HA requires an acknowledgement whne sending messages
    added send message back when recived
  3.05 isn't what I had decided to do
  If the sample interval is received from HA,
    the sample interval will be changed
    And stored in NVRAM 
  A calibration flag, a number, is non-zero, 
    It is assumed that the device will be calibrated
      The power supply for the device will be a programmable supply
      For observing the raw moisture count
        The TX and GND of the Device will go to a terminal
        and/or
        The raw moisture count will be observed in HA
    The PROCESS will be:

      send a number greater than 4 and less than 255 to the calibration flag
        255 will perform a factory reset 
      Dry the sensor

      Set the power supply to max voltage
      wait for the moisture level to stabilize
      send a 1 to the calibration flag

      Set the power supply to min voltage
      wait for the moisture level to stabilize
      send a 2 to the calibration flag

      put the sensor into a glass of water so that the waterisup to the line on the sensor

      (the power supply is already at min voltage)
      wait for the moisture level to stabilize
      send a 3 to the calibration flag

      Set the power supply to min voltage
      wait for the moisture level to stabilize
      send a 4 to the calibration flag

      Send a 0 to the calibration flag


    The COD
    the device will enter calibration mode which is
      the interval period is set to 2 seconds (2000mSec)
      The raw moisture level will be reported.
    if the calibration flag is set to:
      0
        the interval period is set to the value in NVRAM
        the moisuture level will be adjusted per the calibration constants
      1
        The maxVoltage is set to the current voltage value and stored in NVRAM
        The DRY x maxVoltage is set to the current raw moisture value and store in NVRAM
        Then set the calibration flag to 0xFF
      2
        The minVoltage is set to the current voltage value and stored in NVRAM
        The DRY x minVoltage is set to the current raw moisture value and store in NVRAM
        Then set the calibration flag to 0xFF
      3
        The WET x minVoltage is set to the current raw moisture value and store in NVRAM
        Then set the calibration flag to 0xFF
      4
        The WET x maxVoltage is set to the current raw moisture value and store in NVRAM
        Then set the calibration flag to 0xFF
    
Ver 3.05
    get the constants from EEPROM
    left them as the default of the value in the EEPROM was 0xFF
    send them to HA
Ver 3.00
  need to add:
     get interval on boot
     get calibration constants on boot?

Ver 2.00
o capture time it takes to get and send data
  ~458mSec
  This will be subtracted for the total time
o note: the amount of time the Arduino computes it slept is ~509mSec

Ver 1.00
o copied from MySensorsMoistureHumidityTemperatureBattery
  and made some changes
o when the idle period is "sleep", use [in HA] 
  switch.turn_off... switch.smtNNN
  text.set_value ... text.intNNN ... data: value: text.intNNN
o when the idle period is "wait", use [in HA] 
  mqtt.publish ... data: topic: mysensors-in/NNN/10/1/0/2 payload: 1
  mqtt.publish ... data: topic: mysensors-in/177/0/0/0/47 payload: 200000 (period in time)
o The script below will change the interval during sleep


% moisture calculation
%moist=(B2-((D1-B1x)/(D1-D2)*(H1-H2)+H2))/((((D1-B1x)/(D1-D2))*(F1-F2)+F2)-((D1-B1x)/(D1-D2)*(H1-H2)+H2))
v is current voltage reading B1x
C is current count reading   B2
CALIBRATION CONSTANTS
Vmax	maximum voltage		D1
Vmin	minimum voltage		D2
Cxd		maximum count dry	F1
Cnd		minimum count dry	F2
Cxw		maximum count wet	H1
Cnw		minimum count wet	H2

IMPORTANT
Choosing where to store the calibration constants
1. device EEPROM
  non-trivial code has to be added to do the calibration
  the calibration must be done before assembly
2. Home assistant and pass to device on boot
  non-trivial code in HA to send the the calibration constants
3. Home assistant and Home assistant receives the raw count and voltage and does the calculation
  Difficulties will arise in coding the calculation
  No changes to current device code

NOTE: nonlinierities occur at voltages belo 3.4V  Thus Vmin will be 3.4v

Home Assistant (HA) sample period
  interval period
  offset and slope of temperature device
  wet value limit and dry value limit for soil moisture sensor
In the very first connection between the Arduino and Home Assistant (HA),
  these defaults must be sent from the Arduino to the controller.
The Arduino does not know at the time of boot if this boot is the very first connection,
  but if it is not, and the Arduino sends the defaults, the values in HA would be overwriten.
The Arduino could request data from HA, and would not receive a response
  but the lack of response may be due to HA being down, though it is already configured

Thus the technique will be to have a binary switch in HA that the user can control.
  when this switch is set true, the state will be sent to the Arduino.  The Arduino
  will then send the default values.  After the values are set, the Arduino will reset
  the switch.  Because there needs to be a delay between message sends in the Arduino,
  there will be a lag time between when the user sets the switch and the Arduino resets it.
  Currently, this is about 10 seconds.  The user seeing the switch reset very quickly 
    indicates that the Arduino did not receive the reset message!
    
A change from HA to one of the defaults can be performed programatically.  This yaml script
  will cause the interval time for the sensor at node 133 to change to 60 seconds
    alias: SendToArduino-int133
    sequence:
      - service: text.set_value
        target:
          entity_id: text.int133
        data:
          value: "60000"
    mode: single 

*/// IMPORTANT

/*
 *  ver 3.00
 *  Created calibration constants that are stored in NVRAM
 *  ver 2.00
 *    ?
 *  ver 1.00
 *    Changed name of program to RSMoistureSensor
 *    ver 3.00 was never compiled
 *  ver 3.00
 *    Just sending raw data.  This will make calibration a lot easier 
 *      because the calibration can be done when the sustem is completely built
 *      updataed the above description
 *    removed all reference to DHT
      Connecting moisture sensor analog port A0
        manufacturing convenience
 *  ver 2.02
 *    Now connects moisture sensor to analog port 0 for convenience of construction
 *    don't need EEPROM because MySensors reads the address location (EEPROM 0x00)
 *    If the address is 0xFF, the gateway supplies an arbitrary NODE_ID
 *    Otherwise the NODE_ID is given this value
 *  ver 2.01
 *    The variables are not registering with HA
 *    perhaps a longer wait between sending variables
 *    shorter wait for regular messages
 *  ver 2.00
 *    set compiler directive to indicate production
 *  ver 1.20
 *    added compiler directives to be able to do development without disturbing
 *      the production environment
 *    removed a number of debug statements
 *  ver 1.19
 *    1.18 seems to be working.  Some debug print statements could be removed.
 *  ver 1.18
 *    The wait for type wasn't working right it wait() has 1, 2 or 3 parameters,
 *      the last being message type meaning the other two have to be there.
 *      The first is the time to wait
 *      The second is a "cmd" which I arbitrarily put 2 (req)
 *      The third is type.
 *    I set it up so that if fetching sample period fails, it just bombs out and uses
 *      the default settings.  The assumptiion is that the defaults need to be created in
 *      Home Assistant.  Subsequently the user must trigger the reset function.
 *      This seems to work
 *    When there is an asynchronous receive, there must be a response to it or 
 *      else it will be continuosly be sent.  So a message is sent with that value.
 *  ver 1.17
 *    Try sending the request and then wait for a response for each variable
 *    This seemed to work
 *  ver 1.16
 *    Trying smartSleep to see if messages sent while asleep are buffered so that 
 *        the messages arrive when the device comes out of sleep.  Otherwise,
 *        messages can only be sent during the awake period.
 *    smartSleep didn't seem to work.  Message didn't get through and the a slew of
 *      message came through.
 *  ver 1.15
 *      1.14 seems to be written correctly, but Home Assistant isn't getting 
 *        the sensor inputs.  don't want to muck with something that may work
 *    There was a mistake in the checkForReceived function, both terms should have been false
 *      this routine only exists to print out that data has been received
 *    Added feature that when calibration is locked, the device sleeps for the interval period,
 *      but when unlocked, it waits for the interval period (saves battery when sleeping)
 *    When things seem to be going awry and it is desired to start from ground zero:
 *      1) stop the mysgw
 *      2) In Home Assistant, delete the instance of this device
 *          Settings-->Integrations-->mqtt/MySensors/x devices/device(BatMoistHumTemp)-->delete
 *          (DO NOT attempt to remove the device in the CodeServer section)
 *      3) Using the MQTT Explorer, remove all vestiges of the device.
 *      4) In Home Assistant, restart the engine
 *          Developer Tools-->[RESTART]  (wait for the restart to complete)
 *      5) Restart the mysgw (wait until it seems to have completely restarted)
 *      6) Start this sketch
 *      7) In Home Assistant go to the page of the instance of this device
 *          Settings-->Integrations-->mqtt/MySensors/x devices/device(BatMoistHumTemp)
 *      8) Eventually, the binary control "RSTnnn" will appear. Wait another minute or more
 *          then click on it.  This will set all the parameters to default.  Wait for many minutes
 *          All of the controls will appear and the binary control will set back to off
 *      9) Click on the binary control "ULKnnn"  This unlocks the calibration variables.  Wait at leas a minute
 *          This is done because making changes to the variables takes up a lot of time (aka battery)
 *      10) Enter the data in the desired variables.  Wait several minutes
 *      11) Click on the binary control "ULKnnn"  This locks the calibration variables.
 *  ver 1.14
 *    Incorporating request()
 *      since the node will be sleeping, no data will be coming in.
 *      to get any message, a request needs to be done
 *      also, the default sample period, offset and slope are initialized
 *        in this code but should be obtained from the controller (Home Assistant)
 *      removed a lot of debug print statements.
 *    Added calibration Constants to soil moisure sensor
 *  ver 1.13
 *    Fixed serious bug in receiving a change in sample period
 *    There is a problem in wireless communication that can be corrected
 *      by having a intermediat repeater node.
 *    1.12 is functionally correct, but does not work well
 *      data not sent by controller or takes a long time to be sent
 *  ver 1.12
 *   name change
 *   incorporated offset and slope to temperature sensors
 *  ver 2.00 was sendBatteryLevel
      1.00 was successful at sending the battery voltage as a sensor
        and it also sent the % battery level to the MQTT broker
        but Home assistant did not pick up the % battery level
      Removed sendBatteryLevel
      Add sending Humidity
      Add Sending Temperature
      Add Sending Soil Moisture
        
 *  ver 1.00
      This program tests the sendBatteryLevel function of the MySensors Library
      Assumptions:
        The MySensors Gateway is on nRF24 channel 121
        The gateway is to an MQTT broker
        Home Assistant is will receive the data from the MQTT broker
*/

// Enable and select radio type attached
#define MY_RADIO_RF24
#define MY_RF24_CS_PIN 9
#define MY_RF24_CE_PIN 10
#ifdef DEV
  // Enable debug prints to serial monitor
  #define MY_DEBUG
  #define MY_RF24_PA_LEVEL (RF24_PA_MIN)
  #define MY_RF24_CHANNEL 86
#else
  #ifdef MY_DEBUG
    #undef MY_DEBUG
  #endif
  #define MY_RF24_PA_LEVEL (RF24_PA_MAX)
  #define MY_RF24_CHANNEL 121
#endif 

//------------------------------------------------------------------------ infrastructure
//------------------------------------------------------------------------ MySensors
#include <MySensors.h>
#define MESSAGE_WAIT 500     // ms to wait between each send message
#define REGISTERING_VARIABLES_WAIT 4000  // ver 2.01

// Sample Interval
#define CHILD_ID_SAMPLE_INTERVAL 0
MyMessage msgSampleInterval(CHILD_ID_SAMPLE_INTERVAL, V_TEXT);
volatile unsigned long SamplePeriod = 20000; // ms to sleep between sensor measurements
unsigned long SamplePeriodSaved = SamplePeriod;
volatile bool SamplePeriodReceived = false;

// moisture sensor
#define CHILD_ID_MOISTURE 1
MyMessage msgMoisture(CHILD_ID_MOISTURE, V_LEVEL);
#define ANALOG_PIN_FOR_MEASURING_SOIL_MOISTURE A0
int moistureLevel = 0;

// battery sensor has two parts, voltage and percent charged0
#define CHILD_ID_BATTERY 2
MyMessage msgBattery(CHILD_ID_BATTERY, V_VOLTAGE);

// for sending calibration constants
#define CHILD_ID_CALIBRATION 3
MyMessage msgCAL(CHILD_ID_CALIBRATION, V_TEXT);
volatile int CalibrationFlag = 0;  // must be type int for case statement?
volatile bool CalibrationFlagReceived = false;

// constants in NVRAM
#define NVRAM_LOCATION_START 2
unsigned long IntervalCalibration = 2000;
// constants for determining moisture level
int Vbat_MIN = 3100; // milli Volts, also used in conjuction with sendBatteryLevel()
int Vbat_MAX = 4400; // milli Volts, also used in conjuction with sendBatteryLevel()
int CxD = 900; // count max dry +8
int CnD = 680; // count min dry +10
int CxW = 375; // count max wet +12
int CnW = 275; // count min wet +14


//------------------------------------------------------------------------ReadCalibrationConstants
void ReadCalibrationConstants(){
  uint16_t D1;
  uint8_t NVRAMlocation = NVRAM_LOCATION_START;  // NVRAM address of first stored variable.
  // Sample period
  unsigned long SPz = uint32_t(loadState(NVRAMlocation++))<<24 | uint32_t(loadState(NVRAMlocation++))<<16 | uint32_t(loadState(NVRAMlocation++))<<8 | uint32_t(loadState(NVRAMlocation++));
  if (SPz != 0xFFFFFFFF) SamplePeriod = SPz;  // +0
  // calibration constants
  D1 = loadState(NVRAMlocation++)<<8 | loadState(NVRAMlocation++); if (D1 != 0xFFFF) Vbat_MIN = D1;  // +4
  D1 = loadState(NVRAMlocation++)<<8 | loadState(NVRAMlocation++); if (D1 != 0xFFFF) Vbat_MAX = D1;  // +6
  D1 = loadState(NVRAMlocation++)<<8 | loadState(NVRAMlocation++); if (D1 != 0xFFFF) CxD = D1;       // +8
  D1 = loadState(NVRAMlocation++)<<8 | loadState(NVRAMlocation++); if (D1 != 0xFFFF) CnD = D1;       // +10
  D1 = loadState(NVRAMlocation++)<<8 | loadState(NVRAMlocation++); if (D1 != 0xFFFF) CxW = D1;       // +12
  D1 = loadState(NVRAMlocation++)<<8 | loadState(NVRAMlocation++); if (D1 != 0xFFFF) CnW = D1;       // +14

#ifdef DEV
  MY_SERIALDEVICE.println(F("Calibration Constants: "));

  MY_SERIALDEVICE.print(F("SamplePeriod: "));MY_SERIALDEVICE.println(SamplePeriod);

  MY_SERIALDEVICE.print(F("Vbat_MIN: "));MY_SERIALDEVICE.println(Vbat_MIN);
  MY_SERIALDEVICE.print(F("Vbat_MAX: "));MY_SERIALDEVICE.println(Vbat_MAX);

  MY_SERIALDEVICE.print(F("CxW: "));MY_SERIALDEVICE.println(CxW);
  MY_SERIALDEVICE.print(F("CnW: "));MY_SERIALDEVICE.println(CnW);
  MY_SERIALDEVICE.print(F("CxD: "));MY_SERIALDEVICE.println(CxD);
  MY_SERIALDEVICE.print(F("CnD: "));MY_SERIALDEVICE.println(CnD);
#endif

}

//------------------------------------------------------------------------percentMoist
int percentMoist(int Voltage, int Count){
  // dry point for the current voltage of the battery:map(Voltage, Vbat_MAX,Vbat_MIN, CxD,CnD)
  // saturated wet point for the current voltage of the battery: map(Voltage, Vbat_MAX,Vbat_MIN, CxW,CnW)
  // Convert the count to a percentaage between dry point and saturated wet point
  return map(Count, map(Voltage, Vbat_MAX,Vbat_MIN, CxD,CnD), map(Voltage, Vbat_MAX,Vbat_MIN, CxW,CnW), 0, 100);
}

//------------------------------------------------------------------------Calibrate
// this routine is only run if a calibration flag is received
void Calibrate(){
  #ifdef DEV
    MY_SERIALDEVICE.print(F("----------------Calibration "));
    MY_SERIALDEVICE.println(CalibrationFlag);
  #endif
  switch(CalibrationFlag) {
    /*
      */
    case 0:  // return to sensing (set sample period back to value in NVRAM )
      SamplePeriod = SamplePeriodSaved;  // this will be default if 
      #ifdef DEV
        MY_SERIALDEVICE.println(F("----------------Calibration ended "));
      #endif
      ReadCalibrationConstants();
      break;
    case 1:  // set CxD maxvolt-dry (+8) and Vbat_MAX (+6)
      saveState((NVRAM_LOCATION_START+8),(moistureLevel >> 8));  // store Most Significant Bits (MSB)
      saveState((NVRAM_LOCATION_START+9),(moistureLevel & 0x00FF));  // store Least Significant Bits (LSB)
      #ifdef DEV
        MY_SERIALDEVICE.print(F("----------------Calibration CxD "));MY_SERIALDEVICE.println(moistureLevel);
      #endif
      Vbat_MAX = readBatteryVoltage();
      saveState((NVRAM_LOCATION_START+6),(Vbat_MAX>>8));
      saveState((NVRAM_LOCATION_START+7),(Vbat_MAX & 0x00FF));
      #ifdef DEV
        MY_SERIALDEVICE.print(F("----------------Calibration Vbat_MAX "));MY_SERIALDEVICE.println(Vbat_MAX);
      #endif
      break;
    case 2:  // set CnD minvolt-dry (+10) and Vbat_MIN (+4)
      saveState((NVRAM_LOCATION_START+10),(moistureLevel >> 8));
      saveState((NVRAM_LOCATION_START+11),(moistureLevel & 0x00FF));
      #ifdef DEV
        MY_SERIALDEVICE.print(F("----------------Calibration CnD "));MY_SERIALDEVICE.println(moistureLevel);
      #endif
      Vbat_MIN = readBatteryVoltage();
      saveState((NVRAM_LOCATION_START+4),(Vbat_MIN>>8));
      saveState((NVRAM_LOCATION_START+5),(Vbat_MIN & 0x00FF));
      #ifdef DEV
        MY_SERIALDEVICE.print(F("----------------Calibration Vbat_MIN "));MY_SERIALDEVICE.println(Vbat_MIN);
      #endif
      break;
    case 3:  // set CnW minvolt-wet (+14)
      saveState((NVRAM_LOCATION_START+14),(moistureLevel >> 8));
      saveState((NVRAM_LOCATION_START+15),(moistureLevel & 0x00FF));
      #ifdef DEV
        MY_SERIALDEVICE.print(F("----------------Calibration CnW "));MY_SERIALDEVICE.println(moistureLevel);
      #endif
      break;
    case 4:  // set CxW maxvolt-wet (+12)
      saveState((NVRAM_LOCATION_START+12),(moistureLevel >> 8));
      saveState((NVRAM_LOCATION_START+13),(moistureLevel & 0x00FF));
      #ifdef DEV
        MY_SERIALDEVICE.print(F("----------------Calibration CxW "));MY_SERIALDEVICE.println(moistureLevel);
      #endif
      break;
    case 127: // set factory defaults
      for (uint8_t addr = NVRAM_LOCATION_START;addr < NVRAM_LOCATION_START+16;addr++) saveState(addr,0xFF);
      #ifdef DEV
        MY_SERIALDEVICE.println(F("----------------Calibration NVRAM reset "));
      #endif
      ReadCalibrationConstants();
      break;
    default: // set sample period to 2 seconds
      SamplePeriodSaved = SamplePeriod;
      SamplePeriod = 2000;
      #ifdef DEV
        MY_SERIALDEVICE.println(F("----------------Calibration started "));
      #endif
      break;
  }
}
//------------------------------------------------------------------------checkForDataReceived
void checkForDataReceived(){
  // check for data received and process
  // This is "in loop", post ISR processing of the data
  if (SamplePeriodReceived){
    SamplePeriodReceived = false;
    send(msgSampleInterval.set(SamplePeriod));
    wait(MESSAGE_WAIT);

    // save Sample Period in NVRAM
    saveState((NVRAM_LOCATION_START    ),(SamplePeriod >> 24) & 0x000000FF);
    saveState((NVRAM_LOCATION_START +1 ),(SamplePeriod >> 16) & 0x000000FF);
    saveState((NVRAM_LOCATION_START +2 ),(SamplePeriod >>  8) & 0x000000FF);
    saveState((NVRAM_LOCATION_START +3 ),(SamplePeriod      ) & 0x000000FF);
  }
  if (CalibrationFlagReceived) {
    CalibrationFlagReceived = false;
    send(msgCAL.set(CalibrationFlag));
    wait(MESSAGE_WAIT);
    Calibrate();
  }
}

//------------------------------------------------------------------------before
void before(){
  #ifdef DEV
  Serial.println(F("--------------------------- DEV ---------------------------"));
  #endif
  Serial.print(SKETCH_NAME);Serial.print(F("Version "));Serial.println(VER);
  Serial.print("Channel: ");Serial.print(MY_RF24_CHANNEL);Serial.print(" Node ID: ");Serial.println(xMY_NODE_ID);
}

//------------------------------------------------------------------------prepresentation
char varName[7]; // this makes the variable global
void prepresentation(){
  // create three character, zero padded, NodeID
  //  to be the suffix so that the interval, slope, and offset can be unique for each device
  char NodeIDchar[4];
  itoa(xMY_NODE_ID,NodeIDchar,10);
  if (xMY_NODE_ID<10){
    varName[5] = NodeIDchar[0];
    varName[4] = '0';
    varName[3] = '0';
  }
  else { 
    if (xMY_NODE_ID<100){
      varName[5] = NodeIDchar[1];
      varName[4] = NodeIDchar[0];
      varName[3] = '0';
    }
    else{
      varName[5] = NodeIDchar[2];
      varName[4] = NodeIDchar[1];
      varName[3] = NodeIDchar[0];
      
    }
  }
  varName[6]=0;  //ensure 0 as last character of "a string of characters terminated by the 0 character"
}

//------------------------------------------------------------------------presentation
void presentation()
{
  prepresentation();  // set up suffix to make incoming variables unique to this device

	// Send the sketch version information to the gateway and Controller
  char sVER[6];dtostrf(VER,9,2,sVER);
	sendSketchInfo(SKETCH_NAME, sVER);
  
  varName[0] ='i';varName[1] ='n';varName[2] ='t';
  present(CHILD_ID_SAMPLE_INTERVAL,S_INFO,varName); // sensor 0
  wait(REGISTERING_VARIABLES_WAIT);

  varName[0] ='m';varName[1] ='o';varName[2] ='i';
  present(CHILD_ID_MOISTURE, S_MOISTURE,varName);   // sensor 1
  wait(REGISTERING_VARIABLES_WAIT);

  varName[0] ='b';varName[1] ='a';varName[2] ='t';
  present(CHILD_ID_BATTERY,S_MULTIMETER,varName);   // sensor 2
  wait(REGISTERING_VARIABLES_WAIT);

  varName[0] ='C';varName[1] ='A';varName[2] ='L';
  present(CHILD_ID_CALIBRATION,S_INFO,varName);  // sensor 3
  wait(REGISTERING_VARIABLES_WAIT);

}
//------------------------------------------------------------------------readMoistureSensor
int readMoistureSensor(int Vbat){
  moistureLevel = analogRead(ANALOG_PIN_FOR_MEASURING_SOIL_MOISTURE);
  int pcmoist = percentMoist(Vbat,moistureLevel);
  
  #ifdef DEV
    MY_SERIALDEVICE.print(F("-------------------------------Moisture level: "));MY_SERIALDEVICE.print(pcmoist);
    MY_SERIALDEVICE.print(F("% raw: "));MY_SERIALDEVICE.println(moistureLevel);
  #endif

  // if the Sample period is 2s, then calibration mode is running and return the raw moisture level
  if (SamplePeriod == 2000) return moistureLevel; else return pcmoist; // 
}
//------------------------------------------------------------------------readBatteryVoltage
int readBatteryVoltage(){
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);

  wait(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both

  return int(1125300L / ( (high<<8) | low)); 
}
//------------------------------------------------------------------------readAndSendValues
void readAndSendValues(){
  char charVar[12];  
  
  // Battery
  int Vbat = readBatteryVoltage();
  float Vbatf = float(Vbat) / 1000.0;
  // Battery Level is supposed to be found in Home Assistant  But doesn't work
  int batLevel = map(Vbat, Vbat_MIN, Vbat_MAX, 0, 100);
  #ifdef DEV
    MY_SERIALDEVICE.print("---------------------------------- Battery: ");
    MY_SERIALDEVICE.print(Vbatf);
    MY_SERIALDEVICE.print(F("V charge: "));
    MY_SERIALDEVICE.print(batLevel);
    MY_SERIALDEVICE.println('%');
  #endif  

  // send voltage
  send(msgBattery.set(Vbatf,2));
  wait(MESSAGE_WAIT);

  // Battery Level is supposed to be found in Home Assistant  But doesn't work
  if (!sendBatteryLevel(batLevel)) {
    #ifdef DEV
      MY_SERIALDEVICE.println("---------------------------------- Battery send error ");
    #endif  
  }
  wait(MESSAGE_WAIT);

  int moistureCurrentValue = readMoistureSensor(Vbat);
  send(msgMoisture.set(moistureCurrentValue));
  wait(MESSAGE_WAIT);

}

//------------------------------------------------------------------------setup
void setup(){
  before();  //repeat printing header (save memory)
  
  // CONSTANTS
  ReadCalibrationConstants();
  // Sending a first message is required to let HA know that the variable exists

  send(msgSampleInterval.set(SamplePeriod));
  wait(MESSAGE_WAIT);
  
  // send as a negative to indicate this is what is in NVRAM
  send(msgCAL.set(CalibrationFlag));
  wait(MESSAGE_WAIT);

}

//------------------------------------------------------------------------loop
void loop(){
  checkForDataReceived();
  
  readAndSendValues();

  #ifdef DEV
    MY_SERIALDEVICE.print(F("---------------- "));
    MY_SERIALDEVICE.print(SamplePeriod);
    MY_SERIALDEVICE.println(F(" of sleep"));
  #endif
  smartSleep(SamplePeriod);
  
}

//------------------------------------------------------------------------receive
void receive(const MyMessage &message){
  // minimize time in interupt service routine 
  // Get message, flag, and leave ... no print statements
	if (message.getSender() == 0 ) {  // Only listen to messages from controller
    switch( message.getSensor() ) {
      case CHILD_ID_SAMPLE_INTERVAL:
        // MQTT topic: mysensors-in/177/0/0/0/47
        SamplePeriod = message.getLong();
        SamplePeriodSaved = SamplePeriod;
        SamplePeriodReceived = true;
        #ifdef ISRp
          MY_SERIALDEVICE.println(F(" Sample Interval"));
        #endif
        break;
      case CHILD_ID_CALIBRATION:
        // MQTT topic: mysensors-in/177/10/1/0/2
        CalibrationFlag = message.getInt();
        CalibrationFlagReceived = true;
        #ifdef ISRp
          MY_SERIALDEVICE.println(F(" Calibration"));
        #endif
        break;
      default:
        #ifdef ISRp
          MY_SERIALDEVICE.print(F("sender: "));MY_SERIALDEVICE.println(message.getSender());
          MY_SERIALDEVICE.print(F("isAck: "));MY_SERIALDEVICE.println(message.isAck());
          MY_SERIALDEVICE.print(F("Sensor: "));MY_SERIALDEVICE.println(message.getSensor());
          MY_SERIALDEVICE.print(F("Type: "));MY_SERIALDEVICE.println(message.getType()); 
          MY_SERIALDEVICE.print(F("Data: "));MY_SERIALDEVICE.println(message.data);
          MY_SERIALDEVICE.print(F("Long: "));MY_SERIALDEVICE.println(message.getLong());
          MY_SERIALDEVICE.print(F("Float: "));MY_SERIALDEVICE.println(message.getFloat());
        #endif
        break;
    }
  }
/*
*/
}

