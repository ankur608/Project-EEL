/*Cloud visualization of sensor parameters:: Project_EEL.
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
/////////////////////////////////Add.Start
String readString,Count, EC, TDS, Temp, NTU, PMS, HPS, QLState;
int arr1, arr2, arr3, arr4, arr5, arr6, arr7, arr8;
char c;
int TDSI, FlowRate;
float ECF,TempF,NTUF,PMSF,HPSF;
////////////////////////////////Add.End
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

TinyGPSPlus gps;
Adafruit_BMP280 bmp;
//////////////////////////.Add

#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

// WiFi AP SSID
#define WIFI_SSID "SSID"
// WiFi password
#define WIFI_PASSWORD "Password"
// InfluxDB v2 server url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
#define INFLUXDB_URL "InfluxDB hostcloud url"
// InfluxDB v2 server or cloud API authentication token (Use: InfluxDB UI -> Data -> Tokens -> <select token>)
#define INFLUXDB_TOKEN "Specify your Bucket Token"
// InfluxDB v2 organization id (Use: InfluxDB UI -> User -> About -> Common Ids )
#define INFLUXDB_ORG "specify user id"
// InfluxDB v2 bucket name (Use: InfluxDB UI ->  Data -> Buckets)
#define INFLUXDB_BUCKET "specify your Bucket"

// Set timezone string according to https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
// Examples:
//  Pacific Time: "PST8PDT"
//  Eastern: "EST5EDT"
//  Japanesse: "JST-9"
//  Central Europe: "CET-1CEST,M3.5.0,M10.5.0/3"
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);


void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();
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

  // Accurate time is necessary for certificate validation and writing in batches
  // For the fastest time sync find NTP servers in your area: https://www.pool.ntp.org/zone/
  // Syncing progress and the time will be printed to Serial.
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}

float SVal, St, Rt, Ra, lat, lon = 0.0;  
float ECidb, Tempidb, NTUidb, PMSidb, HPSidb = 0.0;  
int TDSidb, QLStateidb;

void loop() {  
while (tGPS.available() > 0)
    if (gps.encode(tGPS.read()))
      gpsInfo(); 
      delay(500); 
      GetSerial();     
SenseBase();
St = sensors.getTempCByIndex(0);
Rt = bmp.readTemperature();
Ra = bmp.readAltitude(1013.25);
lat = gps.location.lat();
lon = gps.location.lng();

ECidb=ECF; Tempidb=TempF; NTUidb=NTUF; PMSidb=PMSF; HPSidb=HPSF; TDSidb=TDSI; QLStateidb=FlowRate; 

writeData(SVal);
delay(10000);
}

//Sensor data injection into InfluxDB2.0
void writeData(float SVal)
{  
    Point dPoint("BASE");   
    //Parameters Obtained via Base Unit. 
    dPoint.addField("Surface_Temp", St); 
    dPoint.addField("Ambient Temp", Rt);
    dPoint.addField("Altitude", Ra);    
    dPoint.addField("lat", lat);
    dPoint.addField("lon", lon);
    //Parameters Obtained via Sonde 
    dPoint.addField("Conductivity", ECidb);
    dPoint.addField("Sonde Temp.", Tempidb);    
    dPoint.addField("Turbidity", NTUidb);
    dPoint.addField("Colorimet.", PMSidb);   
    dPoint.addField("Hydroph. Resp", HPSidb);        
    dPoint.addField("Dissolved Solids", TDSidb);
    dPoint.addField("FlowRate State", QLStateidb);   
      
    dPoint.addTag("device", "QuickFeather(H32)");  
    dPoint.addTag("sensor", "RiverMont.");
       
    Serial.print("Surface Temperature: ");  
    Serial.print(St);  
    Serial.print("\tAmbient Temperature: ");  
    Serial.print(Rt); 
    Serial.print("\tRelative Altitude: ");  
    Serial.print(Ra); 
    Serial.print("\tLat: ");  
    Serial.print(lat); 
    Serial.print("\tLong: ");  
    Serial.println(lon);

    Serial.print("Conductivity: ");  
    Serial.print(ECidb);  
    Serial.print("\tSonde Acq.Temperature: ");  
    Serial.print(Tempidb); 
    Serial.print("\tTurbidity: ");  
    Serial.print(NTUidb); 
    Serial.print("\tColorimet.: ");  
    Serial.print(PMSidb); 
    Serial.print("\tHydroph. Resp: ");  
    Serial.print(HPSidb);
    Serial.print("\tDissolved Solids: ");  
    Serial.print(TDSidb); 
    Serial.print("\tFlowRate State: ");  
    Serial.println(QLStateidb);

    
    // Write point
    if (!client.writePoint(dPoint)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  } 
}

void SenseBase()
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
}

void gpsInfo()
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
