//ST-LRWAN based LoRa Receiver[Project EEL]:: connected to RPi0W.

#include "LoRaRadio.h"

void setup( void )
{
    Serial.begin(115200); 
    while (!Serial) { }
    LoRaRadio.begin(867000000);
    LoRaRadio.setFrequency(867000000);
    LoRaRadio.setTxPower(14);
    LoRaRadio.setBandwidth(LoRaRadio.BW_125);
    LoRaRadio.setSpreadingFactor(LoRaRadio.SF_7);
    LoRaRadio.setCodingRate(LoRaRadio.CR_4_5);
    LoRaRadio.setLnaBoost(true);
    LoRaRadio.receive(5000);
}

void loop( void )
{
int packetSize = LoRaRadio.parsePacket();
if (packetSize) {
    while (LoRaRadio.available()) {
    Serial.print((char)LoRaRadio.read());
    }
    Serial.print("(RSSI: ");
            Serial.print(LoRaRadio.packetRssi());
            Serial.print(", SNR: ");
            Serial.print(LoRaRadio.packetSnr());
            Serial.println(")");                                  
    }
}
