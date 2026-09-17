#include <M5Unified.h>

// =====================================================
// JIN Solution
// M5Stack BASIC USB Serial Monitor
// =====================================================

// =====================================================
// Baud Rate
// =====================================================

const uint32_t baudList[] = {
  9600,
  19200,
  38400,
  57600,
  115200,
  230400,
  460800,
  921600
};

const int BAUD_COUNT = sizeof(baudList) / sizeof(baudList[0]);

// Initial baud rate = 115200
int baudIndex = 4;

// =====================================================
// Display
// =====================================================

const int HEADER_H = 45;
const int FOOTER_H = 28;

// Receive text size
const int TEXT_SIZE = 2;

// Line height
const int LINE_HEIGHT = 18;

// Data starts after timestamp
const int DATA_X = 68;

int cursorX = DATA_X;
int cursorY = HEADER_H + 3;

bool newSerialLine = true;

// =====================================================
// Timestamp (elapsed time from startup)
// HH:MM:SS
// =====================================================

void getTimestamp(char *buffer)
{
  unsigned long totalSeconds = millis() / 1000;

  unsigned long hours = (totalSeconds / 3600) % 100;
  unsigned long minutes = (totalSeconds / 60) % 60;
  unsigned long seconds = totalSeconds % 60;

  sprintf(
    buffer,
    "%02lu:%02lu:%02lu",
    hours,
    minutes,
    seconds
  );
}

// =====================================================
// Header
// =====================================================

void drawHeader()
{
  M5.Display.fillRect(
    0,
    0,
    M5.Display.width(),
    HEADER_H,
    BLACK
  );

  M5.Display.setTextSize(2);
  M5.Display.setTextColor(CYAN, BLACK);
  M5.Display.setCursor(4, 3);
  M5.Display.print("JIN Solution");

  M5.Display.setTextColor(YELLOW, BLACK);
  M5.Display.setCursor(205, 3);
  M5.Display.printf("%lu", baudList[baudIndex]);

  M5.Display.setTextSize(1);
  M5.Display.setTextColor(WHITE, BLACK);
  M5.Display.setCursor(4, 27);
  M5.Display.print("USB SERIAL MONITOR");

  M5.Display.setCursor(210, 27);
  M5.Display.print("8N1");

  M5.Display.drawLine(
    0,
    HEADER_H - 1,
    M5.Display.width(),
    HEADER_H - 1,
    DARKGREY
  );
}

// =====================================================
// Footer
// =====================================================

void drawFooter()
{
  int y = M5.Display.height() - FOOTER_H;

  M5.Display.fillRect(
    0,
    y,
    M5.Display.width(),
    FOOTER_H,
    DARKGREY
  );

  M5.Display.setTextSize(1);
  M5.Display.setTextColor(WHITE, DARKGREY);

  M5.Display.setCursor(25, y + 9);
  M5.Display.print("BAUD -");

  M5.Display.setCursor(140, y + 9);
  M5.Display.print("CLEAR");

  M5.Display.setCursor(250, y + 9);
  M5.Display.print("BAUD +");
}

// =====================================================
// Terminal Clear
// =====================================================

void clearTerminal()
{
  M5.Display.fillRect(
    0,
    HEADER_H,
    M5.Display.width(),
    M5.Display.height() - HEADER_H - FOOTER_H,
    BLACK
  );

  cursorX = DATA_X;
  cursorY = HEADER_H + 3;
  newSerialLine = true;
}

// =====================================================
// Next line
// =====================================================

void nextLine()
{
  cursorY += LINE_HEIGHT;
  cursorX = DATA_X;

  int bottom = M5.Display.height() - FOOTER_H - LINE_HEIGHT;

  if (cursorY > bottom) {
    clearTerminal();
  }
}

// =====================================================
// Draw timestamp
// =====================================================

void drawTimestamp()
{
  char timestamp[16];
  getTimestamp(timestamp);

  M5.Display.setTextSize(1);
  M5.Display.setTextColor(LIGHTGREY, BLACK);
  M5.Display.setCursor(3, cursorY + 4);

  M5.Display.print("[");
  M5.Display.print(timestamp);
  M5.Display.print("]");

  cursorX = DATA_X;
  M5.Display.setCursor(cursorX, cursorY);
  newSerialLine = false;
}

// =====================================================
// Display received character
// =====================================================

void printSerialChar(char c)
{
  if (c == '\r') {
    return;
  }

  if (c == '\n') {
    nextLine();
    newSerialLine = true;
    return;
  }

  if (newSerialLine) {
    drawTimestamp();
  }

  M5.Display.setTextSize(TEXT_SIZE);
  M5.Display.setTextColor(GREEN, BLACK);
  M5.Display.setTextWrap(false);
  M5.Display.setCursor(cursorX, cursorY);
  M5.Display.print(c);

  cursorX = M5.Display.getCursorX();

  if (cursorX > M5.Display.width() - 13) {
    nextLine();
    newSerialLine = false;
    cursorX = DATA_X;
    M5.Display.setCursor(cursorX, cursorY);
  }
}

// =====================================================
// System message
// =====================================================

void printSystemMessage(const char *message)
{
  char timestamp[16];
  getTimestamp(timestamp);

  M5.Display.setTextSize(1);
  M5.Display.setTextColor(LIGHTGREY, BLACK);
  M5.Display.setCursor(3, cursorY + 4);
  M5.Display.printf("[%s]", timestamp);

  M5.Display.setTextSize(2);
  M5.Display.setTextColor(YELLOW, BLACK);
  M5.Display.setCursor(DATA_X, cursorY);
  M5.Display.print(message);

  nextLine();
  newSerialLine = true;
}

// =====================================================
// Baud Change
// =====================================================

void changeBaud(int newIndex)
{
  if (newIndex < 0) {
    newIndex = BAUD_COUNT - 1;
  }

  if (newIndex >= BAUD_COUNT) {
    newIndex = 0;
  }

  baudIndex = newIndex;

  Serial.flush();
  Serial.end();
  delay(50);
  Serial.begin(baudList[baudIndex]);

  clearTerminal();
  drawHeader();
  drawFooter();

  char message[32];
  sprintf(message, "BAUD %lu", baudList[baudIndex]);
  printSystemMessage(message);
}

// =====================================================
// Setup
// =====================================================

void setup()
{
  auto cfg = M5.config();
  M5.begin(cfg);

  M5.Display.setRotation(1);
  M5.Display.fillScreen(BLACK);

  Serial.begin(baudList[baudIndex]);

  drawHeader();
  drawFooter();
  clearTerminal();

  printSystemMessage("READY");
}

// =====================================================
// Loop
// =====================================================

void loop()
{
  M5.update();

  // Button A: baud down
  if (M5.BtnA.wasPressed()) {
    changeBaud(baudIndex - 1);
  }

  // Button B: clear
  if (M5.BtnB.wasPressed()) {
    clearTerminal();
    printSystemMessage("CLEARED");
  }

  // Button C: baud up
  if (M5.BtnC.wasPressed()) {
    changeBaud(baudIndex + 1);
  }

  // USB Serial RX
  while (Serial.available() > 0) {
    char c = Serial.read();
    printSerialChar(c);
  }

  delay(1);
}
