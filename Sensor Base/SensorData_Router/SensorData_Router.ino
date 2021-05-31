//Code for BASE unit that collects data from Sonde and sends to gateway over LoRa.

/*Demo Code for Project_EEL.
 *GPS--Huzzah32(H32) 
 *DS18B20--H32
 *BMP280--H32
 *UART(H32)RX.--L.C.--TX.UART(ATtiny3217)
 *UART(QF).TX--L.C.--RX.UART(ATtiny3217)
 *ATtiny32.ADC--E.C./PPM,H.P.,TUB,P.M...debug
 */
//Test data added in Mk3 and Mk2.  Original Code Mk1.
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <Adafruit_BMP280.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <SPI.h>
#include <SPI.h>
#include <LoRa.h>

#define ONE_WIRE_BUS 33     //Tested.
#define ss 4
#define rst 21
#define dio0 33
HardwareSerial tGPS(2);
#define gpsRXPIN 14
#define gpsTXPIN 32

HardwareSerial SensorBoard(1);
#define sbRXPIN 15
#define sbTXPIN 12

String readString,Count, EC, TDS, Temp, NTU, PMS, HPS, QLState;
int arr1, arr2, arr3, arr4, arr5, arr6, arr7, arr8;
char c;
int TDSI, FlowRate;
float ECF,TempF,NTUF,PMSF,HPSF;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

TinyGPSPlus gps;
Adafruit_BMP280 bmp;

void setup()
{
  Serial.begin(115200);
  sensors.begin();
  LoRa.setPins(ss, rst, dio0);
  LoRa.begin(867E6);
  bmp.begin();
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
                  pinMode(LED_BUILTIN, OUTPUT);
                  tGPS.begin(9600, SERIAL_8N1, gpsRXPIN, gpsTXPIN);
                  SensorBoard.begin(115200, SERIAL_8N1, sbRXPIN, sbTXPIN);
}

void loop()
{ 
  delay(100); //Added.
  digitalWrite(LED_BUILTIN, LOW);
  Serial.print("Surface Probe Temperature...");
  sensors.requestTemperatures();
  Serial.println(sensors.getTempCByIndex(0));
  Serial.print(F("Rt: "));
  Serial.print(bmp.readTemperature());
  Serial.println(" degC");
  Serial.print(F("Ra: "));
  Serial.print(bmp.readAltitude(1013.25)); /* To be adjusted as per ground elevation profile*/
  Serial.println(" mtr.");
  while (tGPS.available() > 0)
    if (gps.encode(tGPS.read()))
      displayInfo();  
  Serial.println();
  //delay(2000);  
  delay(5000);  
  /////////////////////Add.Start
  GetSerial();
  delay(500);
  /////////////////////Add.End
  
  LoRa.beginPacket();
  LoRa.println("Project_EEL ");
  LoRa.print("Base: ");
  LoRa.print("[D/T:");LoRa.print(gps.date.day());LoRa.print("/");LoRa.print(gps.date.month());LoRa.print("/");LoRa.print(gps.date.year());LoRa.print("  ");
  LoRa.print(gps.time.hour());LoRa.print(":");LoRa.print(gps.time.minute());LoRa.print(":");LoRa.print(gps.time.second());LoRa.print("],[Pos:");
  LoRa.print(gps.location.lat(), 4);LoRa.print("  ");LoRa.print(gps.location.lng(), 4);
  LoRa.print("],[Ts:");LoRa.print(sensors.getTempCByIndex(0));LoRa.print("*C],[Rt:");LoRa.print(bmp.readTemperature());LoRa.print("*C],[Ra:");
  LoRa.print(bmp.readAltitude(1013.25));LoRa.print("m]");
  LoRa.endPacket();
  delay(500);
  LoRa.beginPacket();
  LoRa.print("SONDE: ");LoRa.print("[EC:");LoRa.print(ECF);LoRa.print("],[TDS:");LoRa.print(TDSI);LoRa.print("],[Temp:");LoRa.print(TempF);
  LoRa.print("],[NTU:");LoRa.print(NTUF);LoRa.print("],[PMS:");LoRa.print(PMSF);LoRa.print("],[HPS:");LoRa.print(HPSF);LoRa.print("],[F.R.Sate:");LoRa.print(FlowRate);LoRa.print("]");
  LoRa.endPacket();
  digitalWrite(LED_BUILTIN, HIGH);
  delay(2000);
}

void displayInfo()
{
  Serial.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 4);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 4);
  }
  else
  {
    Serial.print(F("N/A"));
  }

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else
  {
    Serial.print(F("N/A"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
  }
  else
  {
    Serial.print(F("N/A"));
  }
  
  Serial.println();
}


void GetSerial()
{
  while (SensorBoard.available() > 0)
  {
  c =SensorBoard.read();
  if (c=='*')
  {
    //Serial.println();
    Serial.println(readString);
    arr1=readString.indexOf(',');
    Count=readString.substring(0, arr1);    
    arr2=readString.indexOf(',', arr1+1);
    EC=readString.substring(arr1+1, arr2);    
    arr3=readString.indexOf(',', arr2+1);
    TDS=readString.substring(arr2+1, arr3);    
    arr4=readString.indexOf(',', arr3+1);
    Temp=readString.substring(arr3+1, arr4);
    arr5=readString.indexOf(',', arr4+1);
    NTU=readString.substring(arr4+1, arr5);
    arr6=readString.indexOf(',', arr5+1);
    PMS=readString.substring(arr5+1, arr6);   
    arr7=readString.indexOf(',', arr6+1);
    HPS=readString.substring(arr6+1, arr7);
    arr8=readString.indexOf(',', arr7+1);
    QLState=readString.substring(arr7+1);
/* // Print String for De-bugging.
    //Serial.print("Count: ");
    Serial.println(Count);
    Serial.print("EC: ");
    Serial.println(EC);
    Serial.print("TDS: ");
    Serial.println(TDS);
    Serial.print("Temp: ");
    Serial.println(Temp);
    Serial.print("NTU: ");
    Serial.println(NTU);
    Serial.print("PMS: ");
    Serial.println(PMS);
    Serial.print("HPS: ");
    Serial.println(HPS);
    Serial.print("FlowRate: ");
    Serial.println(QLState);
    Serial.println();
*/
//////Int&FloatConversion..
ECF = EC.toFloat();
TDSI = TDS.toInt();
TempF = Temp.toFloat();
NTUF = NTU.toFloat();
PMSF = PMS.toFloat();
HPSF = HPS.toFloat();
FlowRate = QLState.toInt(); // TODO.
//////PrintValues.
Serial.print("EC: ");
Serial.println(ECF);
Serial.print("TDS: ");
Serial.println(TDSI);
Serial.print("Temp: ");
Serial.println(TempF);

Serial.print("NTU: ");
Serial.println(NTUF);
Serial.print("PMS: ");
Serial.println(PMSF);
Serial.print("HPS: ");
Serial.println(HPSF);

Serial.print("Flow Rate State: ");
Serial.println(FlowRate);


//Flush individual string data.
    readString="";
    Count="";
    EC="";
    TDS="";
    Temp="";
    NTU="";
    PMS="";
    HPS="";    
    QLState="";
  }
  else{
    readString += c;
  }
}
}
