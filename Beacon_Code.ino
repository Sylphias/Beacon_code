#include <XBee.h>
#include <SoftwareSerial.h>
 
 
// XBee's DOUT (TX) is connected to pin 8 (Arduino's Software RX)
// XBee's DIN (RX) is connected to pin 9 (Arduino's Software TX)

// Establishing Message Structure
// Edit This code to change what is sent in payload
// Message Payload can only contain 256 Bytes, First few of which are reserved for key information
// 1) Message Type (1,2,3) 1 - broadcast 2 - play help 3 - redirect but dont play
// 2) Hop Number (In hexadecimal)
// 4) Message ID
// Look at CRC Encoding
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
        //Convert Unsigned Char to Integers
        int message_type = (char)rx16.getData(0)-'0';
        int hop_number = (char)rx16.getData(1)-'0';
        int message_id = (char)rx16.getData(2)-'0';
        Serial.println(message_type);
        // Handling various types of messages 
        switch(message_type){
          case 1:
            format_message_payload(1,hop_number,message_id);
          break;
          case 2:
            //todo
          break;
          case 3:
            //todo
          break;
          default:
            //todo
          break;
        }

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

void format_message_payload(int message_type, int hop_number, int message_id)
{
  String padded_hop_number;
  hop_number++;
  if(hop_number < 10)
  {
    padded_hop_number = "0"+hop_number;
  }
  message_type = 2;
  message_id = 4;
  Serial.print("Padded:");
  Serial.println(hop_number);
  String composed_message = String(message_type) + String(hop_number) + String(message_id) + "TestPad";
  char payload[32];
  Serial.println(composed_message); 
  composed_message.toCharArray(payload, composed_message.length()+1);
  Serial.println(payload);
  uint8_t payload_bytes[32] = {};
  memcpy((uint8_t*)payload_bytes,(char*)payload, 32);
  Tx16Request tx = Tx16Request(0x0000, payload_bytes, sizeof(payload_bytes));
  xbee.send(tx);

}

