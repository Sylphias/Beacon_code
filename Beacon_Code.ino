#include <XBee.h>
#include <SoftwareSerial.h>
#include <SerialLCD.h>
#include <LiquidCrystal.h> 
#include "rgb_lcd.h"
#include <Wire.h>
// XBee's DOUT (TX) is connected to pin 1 (Arduino's Software RX)
// XBee's DIN (RX) is connected to pin 9 (Arduino's Software TX)
/*
  This file is for the beacon algorithm and how it handles packets coming in and out of the system


  Establishing Message Structure
  Edit This code to change what is sent in payload
  Message Payload can only contain 256 Bytes, First few of which are reserved for key information
  M) Message Type (1,2,3) 1 - broadcast 2 - play help 3 - redirect but dont play
  H) Hop Number (In hexadecimal)
  I) Message ID 
  R) Receiver Chains (Ie addresses of the chips the message has passed through)
  E) End Packet
  Look at CRC Encoding
*/

//Xbee SHits
SoftwareSerial serial1(0, 1); // RX, TX


// Initialize Global Variables
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
int this_beacon_ID = 0;

rgb_lcd lcd;

const int colorR = 255;
const int colorG = 255;
const int colorB = 0;


// initialize the library
SerialLCD slcd(11,12);//this is a must, assign soft serial pins
void setup() 
{
  Serial.begin(9600);
  serial1.begin(9600);
  xbee.setSerial(Serial);
  for (int x = 11; x<13; x++){
    pinMode(x,INPUT);
  }
  lcd.begin(16,2);
  lcd.setRGB(colorR, colorG, colorB);
  lcd.print("Press Button To ");
  lcd.setCursor(0,1);
  lcd.print("Send Messages");
}



 
void loop() 
{
  // Initialize Local Variables
  int message_type = 0;
  int hop_number = 0;
  int message_id = 1;
  char beacon_chain[30];
  int lastIndex = 0;
  xbee.readPacket(100);
  if (xbee.getResponse().isAvailable())
  {
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

        // This method splits up the packet data based off a comma delimiter, not optimised but time constraints....
        packet_input = packet_char;

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
        message_type = packet_sec[0].toInt();
        hop_number = packet_sec[1].toInt();
        message_id = packet_sec[2].toInt();
    
        packet_sec[3].toCharArray(beacon_chain, packet_sec[3].length());  
        // Clear out string and counters to get ready for the next incoming string
        if(message_type == 2){
          lcd.clear();
          lcd.print("Alert Beacon:");
          lcd.print(beacon_chain[0]);
          lcd.setCursor(0,1);
          lcd.print("Beacons Away:");
          lcd.print(hop_number);
          Serial.println(message_type);
          Serial.println(hop_number);
          Serial.println(message_id);
          Serial.println(beacon_chain[0]);
        }
        counter = 0;
        lastIndex = 0;
      } 
    }
  }


  if(digitalRead(11) == HIGH){
    format_message_payload(1,0,message_id);
    lcd.clear();
    lcd.print("Help Message Sent!");
    lcd.setCursor(0,1);
    lcd.print("Scanning...");
    message_id +=1;
    delay(200);
  }
  if(digitalRead(12) == HIGH){
    format_message_payload(3,0,message_id);
    lcd.clear();
    lcd.print("Broadcast Message Sent");
    message_id +=1;
    delay(200);
  }
  if(digitalRead(13) == HIGH){
    format_message_payload(0,0,message_id);
    message_id +=1;
    delay(200);
  }
}

//Prepares the outgoing message to be sent
void format_message_payload(int message_type, int hop_number, int message_id)
{
  String beacon_chain = "";
  String padded_hop_number;
  hop_number++;
  if(hop_number < 10)
  {
    padded_hop_number = "0"+hop_number;
  } 
  beacon_chain += this_beacon_ID;
  String composed_message = String(message_type)+","+ String(hop_number)+","+ String(message_id)+","+ beacon_chain +",";
  Serial.println(composed_message);
  char payload[32];
  composed_message.toCharArray(payload, composed_message.length()+1);
  Serial.println(payload);
  uint8_t payload_bytes[32] = {};
  memcpy((uint8_t*)payload_bytes,(char*)payload, 32);
  Tx16Request tx = Tx16Request(0x0000, payload_bytes, sizeof(payload_bytes));
  xbee.send(tx);

}

