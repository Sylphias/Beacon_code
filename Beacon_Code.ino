#include <XBee.h>
#include <SoftwareSerial.h>
//#include <string.h>
//#include <LiquidCrystal.h>
 
//LiquidCrystal lcd(12,11,5,4,3,2);
 
// XBee's DOUT (TX) is connected to pin 8 (Arduino's Software RX)
// XBee's DIN (RX) is connected to pin 9 (Arduino's Software TX)
SoftwareSerial serial1(0, 1); // RX, TX

XBee xbee=XBee();
XBeeResponse response = XBeeResponse();
Rx16Response rx16 = Rx16Response();
Rx64Response rx64 = Rx64Response();

uint8_t option = 0;
uint8_t data = 0;
uint8_t rssi = 0;
 
void setup() 
{
  Serial.begin(9600);
  serial1.begin(9600);
  xbee.setSerial(Serial);
}
 
void loop() 
{
  String test_msg = "this is a string message";
  char payload[64] ;
  test_msg.toCharArray(payload, test_msg.length());
//  payload = format_message_payload(1,1,1);
    uint8_t payload_bytes[64] = {};
//  for(int char_index = 0 ; char_index < sizeof(payload); char_index++){
//    payload_bytes[char_index] = payload[char_index];
//  }
  memcpy((uint8_t*)payload_bytes,(char*)payload, 64);
//  Serial.println((char)payload_bytes[1]);
  Tx16Request tx = Tx16Request(0x0000, payload_bytes, sizeof(payload_bytes));
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


        Serial.println(rssi);
        String ab ;
        for(int x = 0 ; x< rx16.getDataLength()+1; x++){
          char y = rx16.getData(x);
           ab = ab + y;
        }
        Serial.println(ab);

        xbee.send(tx);
      } 
      else 
      {
        Serial.println("64");
        xbee.getResponse().getRx64Response(rx64);
        rssi = rx64.getRssi();
        Serial.println(rssi);
      }
    }
  }
}
// Establishing Message Structure
// Edit This code to change what is sent in payload
// Message Payload can only contain 256 Bytes, First few of which are reserved for key information
// 1) Message Type
// 2) Hop Number
// 3) Message ID

//char format_message_payload(char message_type, char hop_number, char message_id)
//{
//  char payload[256] ={message_type, hop_number , message_id};
//  char test_string[256] = 'Help!, Help me! I am trapped!!!';
//  int i = 3;
//  for (int x = 0; x < sizeof(test_string); x++){
//    Serial.print(test_string[x]);
//    payload[i] = test_string[x];
//    i = i+1;
//  }
//  return payload;
//}











