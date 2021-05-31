// Sonde code with sensor data collec&calib. over ATtiny3217

#include <OneWire.h>
OneWire  ds(10);        //[10]SDA_AT3217
#include <tinyNeoPixel.h>
#define WS_PIN 11       //[11]SCL_AT3217
#define UV_PIN 6        //[6]PB5_AT3217
#define DG_PIN 7        //[7]PB4_AT3217     ||    //RST_PIN 7
#define NUMPIXELS 1 //WS2812 dot.

String readString;
char c;
int p_count = 0;
int R1 = 1000;//BurdR.    
int Ra = 25; //Resistance(Internal)of MCU_pin
int Color_Sense = A1;      //[PA1]MOSI_AT3217
int NTU_Sense = A2;     //[PA2]MISO_AT3217
int Hydr_Sense = A3;    //[PA3]SCK_AT3217
int EC_Sink = A4;       //[PA4]SS_AT3217
int EC_Sense = A5;      //[PA5]VREF_AT3217
int EC_Source = A6;     //[PA6]DAC_AT3217

int QL_State;
float NTU, HPS, PMS;

//PPM_Conversion...//[USA]PPMconverion:0.5//[EU]PPMconversion:0.64//[AU/IND]PPMconversion:0.7//float PPMconversion=0.6;(Avg.)
float PPMconversion=0.7;
//float TemperatureCoef = 0.019; //changes as per electrolyte we are measuring
float TemperatureCoef = 0.08; //calibrated for brackish water with high salt content.
float K=2.88; //electrode constant(~2.9/~3.0)
  
//************ Temp Probe Related *********************************************//
float celsius;
float Temperature = 10;
float EC = 0;
float EC25 = 0;
int ppm = 0;
float raw = 0;
float Vin = 5; //[reading as per 5.0V and 1024 bit ADC resolution]
float Vdrop = 0;
float Rc = 0;
tinyNeoPixel pixels = tinyNeoPixel(NUMPIXELS, WS_PIN, NEO_GRB + NEO_KHZ800);
int delayval = 500;

void setup()
{
  Serial.begin(115200);
  pixels.begin();
  pinMode(UV_PIN, OUTPUT);
  pinMode(EC_Sense,INPUT);
  pinMode(EC_Source,OUTPUT);//Setting pin for sourcing current
  pinMode(EC_Sink,OUTPUT);//setting pin for sinking current
  digitalWrite(EC_Sink,LOW);//We can leave the ground connected permanantly
  delay(100);
  R1 = (R1+Ra);// compensate for Power Pin Resistance
 
  Serial.println("EEL Data Acquisition...");
  //delay(10000);   //TODO.
  //pinMode(RST_PIN,OUTPUT);    //TODO.
  //digitalWrite(RST_PIN, LOW);  //TODO.
  //Serial.println("Parameters Initialised.");  //TODO.
}
  

void loop()
{ 
delay(200);  
GetQLState();
//delay(200);
GetEC();//dont call this more that 1/5 Hz [once every five seconds] or you will polarise electrolyte.
GetTurbidity();
GetHydro();
GetPM();

Serial.print("#");
Serial.print(p_count);
Serial.print(",");
Serial.print(EC25);
Serial.print(",");
Serial.print(ppm);
Serial.print(",");
Serial.print(Temperature);
Serial.print(",");
Serial.print(NTU);
Serial.print(",");
Serial.print(PMS);
Serial.print(",");
Serial.print(HPS);
Serial.print(",");
Serial.print(QL_State);
Serial.println("*");

p_count++;
delay(5000); //(should be greater than 5seconds to prevent electrode-error)
}

void GetQLState(){
 while (Serial.available() > 0)
{
  c = Serial.read();
  if (c=='}')
  {
    //Serial.println(readString);
    int length = readString.length();
    char s = readString.charAt(readString.length()-1);
    //Serial.println(s);
    QL_State = s - '0';
    //Serial.println(QL_State);
    
    readString="";
  }
  else{
    readString += c;
  }
} 
}
 
void GetEC(){
//*********Reading Temperature Of Electrolyte *******************//
GetTemp();
Temperature = 27.25;
//************Estimates Resistance of Electrolyte ****************//
digitalWrite(EC_Source,HIGH);
raw = analogRead(EC_Sense);
raw = analogRead(EC_Sense);//First reading will be low::cap_charge.
digitalWrite(EC_Source,LOW); 
//***************** Converts to EC **************************//
Vdrop = (Vin*raw)/1024.0;
Rc = (Vdrop*R1)/(Vin-Vdrop);
Rc = Rc-Ra; //acounting for Digital Pin Resistance
EC = 1000/(Rc*K);
//*************Compensating For Temperaure********************//
//EC25 = EC/ (1+ TemperatureCoef*(Temperature-25.0));
EC25 = 0.5;
//ppm = (EC25)*(PPMconversion*1000);
ppm = 380;
}

void GetTemp()
{
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  //float celsius;
  if ( !ds.search(addr)) {
    ds.reset_search();
    delay(250);
    return;
  }

  for( i = 0; i < 8; i++) {
    //Serial.write(' ');
    //Serial.print(addr[i], HEX);
  }
  
  if (OneWire::crc8(addr, 7) != addr[7]) {
      return;
  }
 
  switch (addr[0]) {
    case 0x10:
      //Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      //Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      //Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      //Serial.println("Device is not a DS18x20 family device.");
      return;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    // default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  //Serial.print("Temperature: ");
  //Serial.print(celsius);
  //Serial.println(" *C");
}

void GetTurbidity()
{
  float NTUvoltage  = 0.0;
  for (int i=0;i<100;i++)
  { NTUvoltage += ((float)analogRead(NTU_Sense)/1023)*5; }
  NTUvoltage = NTUvoltage/100;
  NTUvoltage = round_to_dp(NTUvoltage, 1);
  if (NTUvoltage<2.5)
  { NTU = 3000; }
  else
  { NTU = -1120.4*square(NTUvoltage)+5742.3*NTUvoltage-4353.8; }  //Calib. Curve eqn. defined by DFRobot sensor page.
  delay(100);
}

float round_to_dp(float val, int dec)
{
float multiplier=powf(10.0f,dec);
val=roundf(val*multiplier)/multiplier;
return val;  
}

void GetPM()
{
  float PMS = 0.0;
  for (int i = 0; i < NUMPIXELS; i++) {
    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.setPixelColor(i, pixels.Color(0, 150, 0)); // Moderately bright green color.
    pixels.show(); // This sends the updated pixel color to the hardware.
    delay(delayval); // Delay for a period of time (in milliseconds).
  }
  delay(100);
  digitalWrite(UV_PIN, HIGH);
  PMS = analogRead(Color_Sense);
  digitalWrite(UV_PIN, LOW);
  delay(500);
}

void GetHydro()
{
  float HPS = 0.0;
  for (int j=0;j<100;j++)
  { HPS += ((float)analogRead(Hydr_Sense)); }
  HPS = HPS/100;
  delay(500);
}
