/*
  USB-SWLED BOX
  CPU        : Seeed Studio XIAO ESP32-C3
  Development: Arduino IDE

  Serial:
    Baud rate : 115200 bps
    Data bits : 8
    Parity    : None
    Stop bits : 1
    Flow Ctrl : None
    Terminator: CR + LF
    Encoding  : ASCII

  Commands:
    i1   -> switch status query
            i11 = switch ON
            i10 = switch OFF

    o11  -> green LED ON   / response: o1
    o10  -> green LED OFF  / response: o1
    o21  -> red LED ON     / response: o2
    o20  -> red LED OFF    / response: o2

  Errors:
    e1 = undefined command
    e2 = receive buffer overflow
    e3 = invalid CR/LF format

  Pin assignment:
    D1 = Green push switch input
    D4 = Green LED MOSFET control
    D5 = Red LED MOSFET control

  Notes:
    - Switch input uses INPUT_PULLUP.
    - Pressed = LOW.
    - Switch input is always valid regardless of LED state.
    - LEDs are OFF at startup.
    - LED state is retained during communication loss.
    - LED outputs assume HIGH-active MOSFET module.
*/

#include <Arduino.h>
#include <string.h>

const uint8_t PIN_SW_GREEN  = D1;
const uint8_t PIN_LED_GREEN = D4;
const uint8_t PIN_LED_RED   = D5;

const uint32_t SERIAL_BAUDRATE = 115200;
const uint32_t DEBOUNCE_TIME_MS = 30;

const uint8_t RX_BUFFER_SIZE = 32;
char rxBuffer[RX_BUFFER_SIZE];
uint8_t rxIndex = 0;
bool receivedCR = false;

bool switchState = false;
bool lastRawSwitchState = false;
uint32_t lastDebounceTime = 0;

void clearReceiveBuffer();
void sendResponse(const char* response);
void processCommand(const char* cmd);
void receiveSerial();
void updateSwitch();

void setup()
{
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);

  // LEDs OFF at startup/reset
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_RED, LOW);

  pinMode(PIN_SW_GREEN, INPUT_PULLUP);

  const bool initialState = (digitalRead(PIN_SW_GREEN) == LOW);
  switchState = initialState;
  lastRawSwitchState = initialState;
  lastDebounceTime = millis();

  Serial.begin(SERIAL_BAUDRATE);

  clearReceiveBuffer();
}

void loop()
{
  updateSwitch();
  receiveSerial();
}

void updateSwitch()
{
  const bool rawState = (digitalRead(PIN_SW_GREEN) == LOW);

  if (rawState != lastRawSwitchState)
  {
    lastRawSwitchState = rawState;
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) >= DEBOUNCE_TIME_MS)
  {
    switchState = rawState;
  }
}

void receiveSerial()
{
  while (Serial.available() > 0)
  {
    const char c = (char)Serial.read();

    if (c == '\r')
    {
      if (receivedCR)
      {
        sendResponse("e3");
        clearReceiveBuffer();
        continue;
      }

      receivedCR = true;
      continue;
    }

    if (c == '\n')
    {
      if (receivedCR)
      {
        rxBuffer[rxIndex] = '\0';

        if (rxIndex == 0)
        {
          sendResponse("e1");
        }
        else
        {
          processCommand(rxBuffer);
        }

        clearReceiveBuffer();
      }
      else
      {
        sendResponse("e3");
        clearReceiveBuffer();
      }

      continue;
    }

    if (receivedCR)
    {
      sendResponse("e3");
      clearReceiveBuffer();
      continue;
    }

    if (rxIndex >= (RX_BUFFER_SIZE - 1))
    {
      sendResponse("e2");
      clearReceiveBuffer();
      continue;
    }

    rxBuffer[rxIndex++] = c;
  }
}

void processCommand(const char* cmd)
{
  if (strcmp(cmd, "i1") == 0)
  {
    sendResponse(switchState ? "i11" : "i10");
    return;
  }

  if (strcmp(cmd, "o11") == 0)
  {
    digitalWrite(PIN_LED_GREEN, HIGH);
    sendResponse("o1");
    return;
  }

  if (strcmp(cmd, "o10") == 0)
  {
    digitalWrite(PIN_LED_GREEN, LOW);
    sendResponse("o1");
    return;
  }

  if (strcmp(cmd, "o21") == 0)
  {
    digitalWrite(PIN_LED_RED, HIGH);
    sendResponse("o2");
    return;
  }

  if (strcmp(cmd, "o20") == 0)
  {
    digitalWrite(PIN_LED_RED, LOW);
    sendResponse("o2");
    return;
  }

  sendResponse("e1");
}

void sendResponse(const char* response)
{
  Serial.print(response);
  Serial.print("\r\n");
}

void clearReceiveBuffer()
{
  memset(rxBuffer, 0, sizeof(rxBuffer));
  rxIndex = 0;
  receivedCR = false;
}
