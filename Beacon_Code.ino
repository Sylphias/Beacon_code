#include <XBee.h>
#include <SoftwareSerial.h>
//#include <LiquidCrystal.h>
 
//LiquidCrystal lcd(12,11,5,4,3,2);
 
// XBee's DOUT (TX) is connected to pin 8 (Arduino's Software RX)
// XBee's DIN (RX) is connected to pin 9 (Arduino's Software TX)
SoftwareSerial serial1(0, 1); // RX, TX
 
XBee xbee=XBee();
XBeeResponse response = XBeeResponse();
Rx16Response rx16 = Rx16Response();
Rx64Response rx64 = Rx64Response();
uint8_t payload[10] ; 
Tx16Request tx = Tx16Request(0x0000, payload, sizeof(payload));
uint8_t option = 0;
uint8_t data = 0;
uint8_t rssi = 0;
 
void setup() 
{
  Serial.begin(9600);
  serial1.begin(9600);
  xbee.setSerial(Serial);
//  lcd.begin(16,2);
//  lcd.clear();
}
 
void loop() 
{
  xbee.readPacket(100);
  if (xbee.getResponse().isAvailable())
  {
    Serial.println("Getting Signal Strength: ");
    if(xbee.getResponse().getApiId() == RX_64_RESPONSE || xbee.getResponse().getApiId() == RX_16_RESPONSE)
    { 
      if (xbee.getResponse().getApiId() == RX_16_RESPONSE) 
      {
        xbee.getResponse().getRx16Response(rx16);
        rssi = rx16.getRssi();

//        lcd.clear();
//        lcd.print(rssi);
        Serial.println(rssi);
        String ab ;
        for(int x = 0 ; x< rx16.getDataLength()+1; x++){
          char y = rx16.getData(x);
           ab = ab + y;
        }
        Serial.println(ab);
//       Serial.println(rx16.getFrameData());

        xbee.send(tx);
        //Serial.print('sent');
      } 
      else 
      {
        Serial.println("64");
        xbee.getResponse().getRx64Response(rx64);
        rssi = rx64.getRssi();
//        lcd.clear();
//        lcd.print(rssi);
        Serial.println(rssi);
//        xbee.send(rssi);
      }
    }
  }
}
