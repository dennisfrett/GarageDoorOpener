#include <RH_RF69.h>
#include "code.h"  // Defines char pattern[] = { "01010101..." }

// Pins
#define RFM69_NSS D2     // NSS / CS pin
#define RFM69_DIO2 D0    // In continuous mode RFM69 is controlled through this pin.
#define RFM69_IRQ D4     // Not connected, unused in sketch.
#define SHUTDOWN_PIN D1  // Connected reset pin of latch circuit.
#define SPI_CK D5        // For info only - not used in below Sketch
#define SPI_MISO D6      // For info only - not used in below Sketch
#define SPI_MOSI D7      // For info only - not used in below Sketch
#define RFM69_RST D8

#define RF69_FREQ 869.8465
#define CONFIG_PACKET_VARIABLE (RH_RF69_PACKETCONFIG1_PACKETFORMAT_VARIABLE | RH_RF69_PACKETCONFIG1_CRCAUTOCLEAROFF)
#define CONFIG_FSK_CONT_SYNC (RH_RF69_DATAMODUL_DATAMODE_CONT_WITH_SYNC | RH_RF69_DATAMODUL_MODULATIONTYPE_FSK | RH_RF69_DATAMODUL_MODULATIONSHAPING_FSK_NONE)
#define CONFIG_RegRxBw7_8 0x45  // BW=7.8kHz DCC=4%

//#define DEBUG

// Modem config to match original transmitter as close as possible.
const RH_RF69::ModemConfig FSK_CONT = { CONFIG_FSK_CONT_SYNC, 0x3E, 0x80, 0x01, 0x15, CONFIG_RegRxBw7_8, CONFIG_RegRxBw7_8, CONFIG_PACKET_VARIABLE };

RH_RF69 rf69(RFM69_NSS, RFM69_IRQ);

void ShutDown() {
#ifdef DEBUG
  Serial.println("Shutting down...");
#endif

  digitalWrite(SHUTDOWN_PIN, HIGH);

  while (true) {
    yield();
  }
}

void SendCode() {
  rf69.setModeTx();

  for (int i = 0; i < sizeof(pattern); i++) {
    if (pattern[i] == '0') {
      digitalWrite(RFM69_DIO2, LOW);
    } else {
      digitalWrite(RFM69_DIO2, HIGH);
    }

    delayMicroseconds(424);
  }

  rf69.setModeIdle();
}

void setup() {
  // Pull shutdown pin low immediately.
  pinMode(SHUTDOWN_PIN, OUTPUT);
  digitalWrite(SHUTDOWN_PIN, LOW);

#ifdef DEBUG
  Serial.begin(74880);
  delay(2000);
  Serial.println("Starting");
#endif

  pinMode(RFM69_DIO2, OUTPUT);
  pinMode(RFM69_RST, OUTPUT);

  // Reset radio.
  digitalWrite(RFM69_RST, HIGH);
  delay(50);
  digitalWrite(RFM69_RST, LOW);
  delay(100);

#ifdef DEBUG
  Serial.println("\nrf69 reset done OK");
#endif

  if (rf69.init()) {
#ifdef DEBUG
    Serial.println("rf69 radio init OK");
#endif
  } else {
#ifdef DEBUG
    Serial.println("rf69 radio init failed - hanging...");
#endif
    ShutDown();
  }

#ifdef DEBUG
  Serial.print("rf69.setFrequency=");
  Serial.print(RF69_FREQ, 4);
  Serial.println(" MHz");
#endif

  rf69.setFrequency(RF69_FREQ);
  rf69.setTxPower(-5, false); // TODO: Play with this value to see what works best.

  rf69.setModemRegisters(&FSK_CONT);

#ifdef DEBUG
  Serial.println("rf69.setModemConfig=done");

  byte bMSB = rf69.spiRead(RH_RF69_REG_03_BITRATEMSB), bLSB = rf69.spiRead(RH_RF69_REG_04_BITRATELSB);

  Serial.print("BitRate=0x");
  Serial.print(bMSB, HEX);
  Serial.write(' ');
  Serial.print(bLSB, HEX);

  Serial.print(" = ");
  Serial.print(32000000 / ((bMSB * 256) + bLSB));
  Serial.println("bps");

  Serial.print("OP-Mode=0x");
  Serial.println(rf69.spiRead(RH_RF69_REG_01_OPMODE), HEX);

  Serial.print("DataMode=0x");
  Serial.println(rf69.spiRead(RH_RF69_REG_02_DATAMODUL), HEX);

#endif

  rf69.setModeTx();
}


void loop() {
  for (int i = 0; i < 20; i++) {
    SendCode();
    delay(700);
  }

  ShutDown();
}