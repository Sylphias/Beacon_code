#include <XBee.h>
#include <SoftwareSerial.h>

// XBee's DOUT (TX) is connected to pin 1 (Arduino's Software RX)
// XBee's DIN (RX) is connected to pin 9 (Arduino's Software TX)
/*
  This file is for the beacon algorithm and how it handles packets coming in and out of the system


  Establishing Message Structure
  Edit This code to change what is sent in payload
  Message Payload can only contain 256 Bytes, First few of which are reserved for key information
  M) Message Type (1,2,3) 1 - broadcast 2 - play help 3 - redirect but dont play
  H) Hop Number (In hexadecimal)
  I) Message ID (originah)
  R) Receiver Chains (Ie addresses of the chips the message has passed through)
  E) End Packet
  Look at CRC Encoding
*/
SoftwareSerial serial1(0, 1); // RX, TX

XBee xbee=XBee();
XBeeResponse response = XBeeResponse();
Rx16Response rx16 = Rx16Response();
Rx64Response rx64 = Rx64Response();
const int beacon_id = 1 ;
uint8_t option = 0;
uint8_t data = 0;
uint8_t rssi = 0;

int old_message_id = 0;
int original_beacon_ID = 0;

//Beacon mode tells us if we're listening for microphone(1), or broadcasting back(2)
int beacon_mode = 0;

void setup() 
{
  Serial.begin(9600);
  serial1.begin(9600);
  xbee.setSerial(Serial);
  pinMode(13,INPUT);
  pinMode(12,OUTPUT);
  
}
 
void loop() 
{
  int message_type = 0;
  int hop_number = 0;
  int message_id = 0;
  char beacon_chain[30];
  int lastIndex = 0;


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
        String packet_sec[4];
        char packet_char[rx16.getDataLength()];
        String packet_input;
        int counter = 0;
        // This loop converts the data in to a string for processing
        for(int packet_counter = 0 ; packet_counter < rx16.getDataLength(); packet_counter++)
        {
          packet_char[packet_counter] = (char)rx16.getData(packet_counter);
        };

        // Arduino has no regex... have to do it the dirty way
        // Apparently, there is a function in C called sscanf
        // Get this working first, then optimise. (remove the uint ->str ->char array)

        packet_input = packet_char;
        Serial.println(packet_char);
        Serial.println(packet_input);

        for (int i = 0; i < packet_input.length(); i++) {
              // Loop through each character and check if it's a comma
              if (packet_input.substring(i, i+1) == ",") {
                // Grab the piece from the last index up to the current position and store it
                packet_sec[counter] = packet_input.substring(lastIndex, i);
                // Update the last position and add 1, so it starts from the next character
                lastIndex = i + 1;
                // Increase the position in the array that we store into
                counter++;
              }

              // If we're at the end of the string (no more commas to stop us)
              if (i == packet_input.length() - 1) {
                // Grab the last part of the string from the lastIndex to the end
                packet_sec[counter] = packet_input.substring(lastIndex, i);
              }
            }
            

        // Clear out string and counters to get ready for the next incoming string
        counter = 0;
        lastIndex = 0;
        for(int n = 0; n < 4; n++)
        {
        Serial.println(packet_sec[n]);
        }

        // Handling various types of messages 
        // Check if you re-received the previous message
        if(old_message_id != message_id && original_beacon_ID != beacon_chain[1]){
          switch(message_type){
            case 1:
              Serial.println('Test');
              beacon_mode = 1;
              if(digitalRead(13) == HIGH && beacon_mode == 1)
                {
                  digitalWrite(12, HIGH);
                  format_message_payload(2,0,message_id, beacon_chain);
                }
                else
                {
                  digitalWrite(12,LOW);
              }
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

//Prepares the outgoing message to be sent
void format_message_payload(int message_type, int hop_number, int message_id,String beacon_chain)
{
  String padded_hop_number;
  hop_number++;
  if(hop_number < 10)
  {
    padded_hop_number = "0"+hop_number;
  } 
  beacon_chain += beacon_id;
  String composed_message = String(message_type) + String(hop_number) + String(message_id) + beacon_chain;
  char payload[32];
  composed_message.toCharArray(payload, composed_message.length()+1);
  Serial.println(payload);
  uint8_t payload_bytes[32] = {};
  memcpy((uint8_t*)payload_bytes,(char*)payload, 32);
  Tx16Request tx = Tx16Request(0x0000, payload_bytes, sizeof(payload_bytes));
  xbee.send(tx);

}















