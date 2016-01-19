//
// RFID presence sensor
//
// Arduino Pins:
//  2: NRF24 IRQ
//  3: Gate of PNP MOSFET (turns LEDs on/off)
//  5: MFRC522 RST
//  6: MFRC522 SS
//  9: NRF24 CE
// 10: NRF24 SS (CSN)
// 11: MFRC522, NRF24 MOSI
// 12: MFRC522, NRF24 MISO
// 13: MFRC522, NRF24 SCK
//
#include <SPI.h>
#include <MFRC522.h>
#include <MySensor.h>

#define RST_PIN         5
#define SS_PIN          6
#define LEDS_PIN        3

#define CHILD_ID        1
#define CARD_PRESENT    1
#define CARD_AWAY       0
#define UPDATE_TIMER    120000

MFRC522 mfrc522(SS_PIN, RST_PIN);

MySensor gw;
MyMessage msg(CHILD_ID, V_STATUS);

unsigned int cardState = CARD_AWAY;
unsigned long nextUpdate = 0;


void setup() {
    // setup MFRC522 first, otherwise NRF24 will fail
    SPI.begin();
    mfrc522.PCD_Init();

    // MySensors will setup Serial...
    gw.begin();
    gw.present(CHILD_ID, S_BINARY);

    // setup LEDs
    pinMode(LEDS_PIN, OUTPUT);
    digitalWrite(LEDS_PIN, HIGH);

    mfrc522.PCD_DumpVersionToSerial();  // for debugging only
}

void loop() {
    if (!isCardPresent()) {
        setCardState(CARD_AWAY);
        return;
    }

    // selects one of the cards
    if (!mfrc522.PICC_ReadCardSerial()) {
        return;
    }

    // halt so the same card responds next time
    mfrc522.PICC_HaltA();
    setCardState(CARD_PRESENT);
}

void setCardState(int state) {
    unsigned int prevState = cardState;
    cardState = state;

    // notify the GW if the state changed or update timer has elapsed
    if ((cardState != prevState) || ((long)(millis() - nextUpdate) >= 0)) {
        int errCnt = 0;
        while (!gw.send(msg.set(state)) && errCnt < 2) {
            errCnt++;
        }
        nextUpdate = millis() + UPDATE_TIMER;
    }

    // flash LEDs quickly when the card is first detected away
    if (cardState == CARD_AWAY && prevState == CARD_PRESENT) {
        for (int i = 0; i < 2; ++i) {
            digitalWrite(LEDS_PIN, LOW);
            delay(200);
            digitalWrite(LEDS_PIN, HIGH);
            delay(200);
        }
    }
    // pulse LEDs if this is a new card
    else if (cardState == CARD_PRESENT && prevState == CARD_AWAY) {
        fadeLED(235, 0, 10);    // fade in
        delay(1000);
        fadeLED(0, 255, 10);    // fade out
        digitalWrite(LEDS_PIN, HIGH);    // turn all the way off
    }
}

boolean isCardPresent() {
    byte bufferATQA[2];
    byte bufferSize = sizeof(bufferATQA);
    MFRC522::StatusCode result = mfrc522.PICC_WakeupA(bufferATQA, &bufferSize);
    return (result == MFRC522::STATUS_OK ||
            result == MFRC522::STATUS_COLLISION);
}

void fadeLED(int from, int to, int ms) {
    for (int i = from; i != to; ) {
        analogWrite(LEDS_PIN, i);
        delay(ms);
        i += (to > from ? 1 : -1);
    }
}
