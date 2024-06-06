#include <SPI.h>
#include <RH_RF95.h>
#include <ChaCha.h>  // Include the ChaCha library

// Constants
const int TX_POWER = 23; // Transmit power level
const int MAX_INPUT_SIZE = 64; // Adjust based on your input message size
const int MAX_MESSAGE_LEN = 255; // Maximum length for a message
const byte KEY[32] = { /* 32 byte key */ }; // ChaCha20 key
const byte IV[8] = { /* 8 byte IV */ };  // ChaCha20 IV

// Device ID 
const int deviceID = 1;  // Change this to your desired device ID

// Sequence number and current receiver ID
int sequenceNumber = 1;
String currentReceiverID = "";

// Create an instance of the LoRa driver
RH_RF95 rf95;

void setup() {
  pinMode(13, OUTPUT); // LED for indicating transmission
  Serial.begin(9600);
  Serial.println("Transmitter ready to receive messages");

  while (!rf95.init()) {
    Serial.println("LoRa radio initialization failed");
    delay(1000);
  }

  rf95.setFrequency(915.0); // Adjust to your region's frequency band
  rf95.setTxPower(TX_POWER, false);
  rf95.setSignalBandwidth(500000);
  rf95.setSpreadingFactor(12);
}

void loop() {
  // Check for incoming serial messages to send
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    // Include Source ID, and assume input format includes Receiver ID and message: "<receiverID>:<message>"
    int colonIndex = input.indexOf(':');
    if (colonIndex != -1) {
      String receiverID = input.substring(0, colonIndex);
      String messageContent = input.substring(colonIndex + 1);

      // Check if receiver ID has changed
      if (receiverID != currentReceiverID) {
        currentReceiverID = receiverID;
        sequenceNumber = 1; // Reset sequence number
      }

      // Construct message with Source ID (as both Source and Sender ID initially) and sequence number
      String messageWithIDs = String(deviceID) + ":" + String(deviceID) + ":" + receiverID + ":" + messageContent + ":" + String(sequenceNumber);

      // Encrypt the message using ChaCha20
      ChaCha chacha;
      chacha.setKey(KEY, 32);
      chacha.setIV(IV, 8);
      byte buf[MAX_INPUT_SIZE];
      size_t msgLen = messageWithIDs.length();
      messageWithIDs.getBytes(buf, msgLen + 1);
      chacha.encrypt(buf, buf, msgLen); // In-place encryption

      rf95.send(buf, msgLen);
      rf95.waitPacketSent();
      Serial.println("Message sent: " + messageContent + " with sequence number: " + String(sequenceNumber));

      // Increment the sequence number after sending the message
      sequenceNumber++;
    } else {
      Serial.println("Invalid message format");
    }
  }

  // Check for incoming LoRa messages
  if (rf95.available()) {
    uint8_t buf[MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf95.recv(buf, &len)) {
      buf[len] = 0; // Null-terminate the received message

      String receivedMessage = (char*)buf;

      // Expected format: <sourceID>:<senderID>:<receiverID>:<message>:<sequenceNumber>
      int firstColon = receivedMessage.indexOf(':');
      int secondColon = receivedMessage.indexOf(':', firstColon + 1);
      int thirdColon = receivedMessage.indexOf(':', secondColon + 1);
      int fourthColon = receivedMessage.indexOf(':', thirdColon + 1);

      if (firstColon != -1 && secondColon != -1 && thirdColon != -1 && fourthColon != -1) {
        String sourceID = receivedMessage.substring(0, firstColon);
        String senderID = receivedMessage.substring(firstColon + 1, secondColon);
        String receiverIDStr = receivedMessage.substring(secondColon + 1, thirdColon);
        String messageContent = receivedMessage.substring(thirdColon + 1, fourthColon);
        String sequenceNumberStr = receivedMessage.substring(fourthColon + 1);

        int sourceIDInt = sourceID.toInt();

        Serial.print("Source ID: ");
        Serial.println(sourceID);
        Serial.print("Sender ID: ");
        Serial.println(senderID);
        Serial.print("Receiver ID: ");
        Serial.println(receiverIDStr);
        Serial.print("Message: ");
        Serial.println(messageContent);
        Serial.print("Sequence Number: ");
        Serial.println(sequenceNumberStr);

        // Check if the source ID matches the device ID
        if (sourceIDInt == deviceID) {
          Serial.println("Received message is a duplicate of sent message.");
        } else {
          Serial.println("Received: " + receivedMessage);
        }
      } else {
        Serial.println("This Message is broadcast message.");
      }
    }
  }
}
