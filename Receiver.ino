#include <SPI.h>
#include <RH_RF95.h>
#include <ChaCha.h>  // Include the ChaCha library

// Constants
const int TX_POWER = 23; // Transmit power level
const int MAX_MESSAGE_LEN = 255; // Maximum length for a message
const byte KEY[32] = { /* 32 byte key */ }; // ChaCha20 key
const byte IV[8] = { /* 8 byte IV */ };  // ChaCha20 IV

// Device ID for this receiver
const int deviceID = 2;  // Set this to your device ID

// Create an instance of the LoRa driver
RH_RF95 rf95;
void setup() {
  pinMode(13, OUTPUT); // LED to indicate reception
  Serial.begin(9600);
  Serial.println("Receiver");

  while (!rf95.init()) {
    Serial.println("LoRa radio initialization failed");
    delay(1000);
  }

  rf95.setFrequency(915.0); // Set frequency band
  rf95.setTxPower(TX_POWER, false);
  rf95.setSignalBandwidth(500000);
  rf95.setSpreadingFactor(12);
}

void loop() {
  if (rf95.available()) {  // Check if a message is available
    uint8_t buf[MAX_MESSAGE_LEN];  // Buffer to hold the received message
    uint8_t len = sizeof(buf);  // Length of the buffer
    if (rf95.recv(buf, &len)) {  // Receive the message
      buf[len] = 0;  // Null-terminate the received message

      // Decrypt the message using ChaCha20
      ChaCha chacha;
      chacha.setKey(KEY, 32);
      chacha.setIV(IV, 8);
      chacha.decrypt((byte*)buf, buf, len);

      String decryptedMessage = (char*)buf;

      // Expected format: <sourceID>:<senderID>:<receiverID>:<message>
      int firstColon = decryptedMessage.indexOf(':');
      int secondColon = decryptedMessage.indexOf(':', firstColon + 1);
      int thirdColon = decryptedMessage.indexOf(':', secondColon + 1);

      if (firstColon != -1 && secondColon != -1 && thirdColon != -1) {
        String sourceID = decryptedMessage.substring(0, firstColon);
        String senderID = decryptedMessage.substring(firstColon + 1, secondColon);
        String receiverIDStr = decryptedMessage.substring(secondColon + 1, thirdColon);
        String messageContent = decryptedMessage.substring(thirdColon + 1);

        int receiverID = receiverIDStr.toInt();

        Serial.print("Source ID: ");
        Serial.println(sourceID);
        Serial.print("Sender ID: ");
        Serial.println(senderID);
        Serial.print("Receiver ID: ");
        Serial.println(receiverID);
        Serial.print("Message: ");
        Serial.println(messageContent);

        if (receiverID == deviceID) {
          Serial.println("Message for this device.");
        } else {
          // Update sender ID to this device's ID for rebroadcasting
          String newMessage = sourceID + ":" + String(deviceID) + ":" + receiverIDStr + ":" + messageContent;
          byte newBuf[MAX_MESSAGE_LEN];
          size_t newMsgLen = newMessage.length();
          newMessage.getBytes(newBuf, newMsgLen + 1);

          rf95.send(newBuf, newMsgLen); // Send the modified packet directly
          rf95.waitPacketSent();
          Serial.println("Message rebroadcasted with updated sender ID.");
        }
      } else {
        Serial.println("Message format error.");
      }
    }
  }
}
