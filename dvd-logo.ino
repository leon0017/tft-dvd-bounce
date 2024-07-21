#include <PNGdec.h>

// Include image array
#include "dvd-logo.h"

PNG png; // PNG decoder instance

#define MAX_IMAGE_WIDTH 240 // Sets rendering line buffer lengths, adjust for your images

// Include the TFT library - see https://github.com/Bodmer/TFT_eSPI for library information
#include "SPI.h"
#include <TFT_eSPI.h>              // Hardware-specific library
TFT_eSPI tft = TFT_eSPI();         // Invoke custom library

#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 320
#define SPEED_POT_PIN 13
#define SCREEN_BACKLIGHT_PIN 2

uint16_t *imageBuffer = NULL;
uint32_t imageBufferSize = 0;
uint16_t *imageBufferEnd = NULL;
uint16_t imageWidth = 0;
uint16_t imageHeight = 0;

int16_t locX = 0;
int16_t locY = 0;

int16_t maxX = 0;
int16_t maxY = 0;

void setup()
{
  Serial.begin(115200);
  Serial.println("\n\n Using the PNGdec library");

  // Initialise the TFT
  tft.begin();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(3);

  loadImage();

  pinMode(SPEED_POT_PIN, INPUT);
  pinMode(SCREEN_BACKLIGHT_PIN, OUTPUT);

  digitalWrite(SCREEN_BACKLIGHT_PIN, HIGH);

  Serial.println("\r\nInitialisation done.");
}

void loadImage(void) {
  int rc = png.openFLASH((uint8_t *)dvd_logo, sizeof(dvd_logo), pngCache);

  if (rc == PNG_SUCCESS) {
    Serial.println("Successfully opened png file");
    imageWidth = png.getWidth();
    imageHeight = png.getHeight();
    maxX = SCREEN_WIDTH - imageWidth;
    maxY = SCREEN_HEIGHT - imageHeight;
    imageBufferSize = imageWidth * imageHeight * 2;
    imageBuffer = (uint16_t *)malloc(imageBufferSize);
    imageBufferEnd = (uint16_t *)((uintptr_t)imageBuffer + imageBufferSize);

    if (imageBuffer) {
      uint32_t dt = millis();
      rc = png.decode(NULL, 0);
      Serial.print(millis() - dt);
      Serial.println("ms: to load PNG");
    } else {
      Serial.println("Failed to allocate memory for image");
    }
  } else {
    Serial.println("Failed to load image!");
  }
}

void drawImage(int16_t x, int16_t y) {
  if (imageBuffer) {
    tft.startWrite();
    for (int loopHeight = 0; loopHeight < imageHeight; loopHeight++) {
      tft.pushImage(x, y + loopHeight, imageWidth, 1, (uint16_t *)(imageBuffer + loopHeight * imageWidth));
    }
    tft.endWrite();
  }
}

void setImageColor(uint16_t newColor) {
  uint32_t dt = millis();

  uint16_t *ptr = imageBuffer;

  while (ptr < imageBufferEnd) {
    if (*ptr != 0) {
      *ptr = newColor;
    }
    ptr++;
  }

  Serial.printf("[dt: %lums] setImageColor took %lums\n", dt, (millis() - dt));
}

int16_t max(int16_t a, int16_t b) {
  return a > b ? a : b;
}

int16_t min(int16_t a, int16_t b) {
  return a < b ? a : b;
}

void moveImage(int16_t x, int16_t y) {
  if (!imageBuffer) {
    return;
  }

  tft.startWrite();

  int16_t newLocX = locX + x;
  int16_t newLocY = locY + y;
  
  // Keep new loc in screen
  newLocX = max(0, min(newLocX, maxX));
  newLocY = max(0, min(newLocY, maxY));
  
  // Calculate rectangles to clear
  int16_t clearLeft = min(locX, newLocX);
  int16_t clearTop = min(locY, newLocY);
  int16_t clearRight = max(locX + imageWidth, newLocX + imageWidth);
  int16_t clearBottom = max(locY + imageHeight, newLocY + imageHeight);
  
  // Clear areas that are not overlapping
  if (newLocY > locY) {
    tft.fillRect(clearLeft, clearTop, clearRight - clearLeft, newLocY - locY, TFT_BLACK);
  } else if (newLocY < locY) {
    tft.fillRect(clearLeft, newLocY + imageHeight, clearRight - clearLeft, locY - newLocY, TFT_BLACK);
  }
  
  if (newLocX > locX) {
    tft.fillRect(clearLeft, clearTop, newLocX - locX, clearBottom - clearTop, TFT_BLACK);
  } else if (newLocX < locX) {
    tft.fillRect(newLocX + imageWidth, clearTop, locX - newLocX, clearBottom - clearTop, TFT_BLACK);
  }
  
  // Update location
  locX = newLocX;
  locY = newLocY;
  for (int loopHeight = 0; loopHeight < imageHeight; loopHeight++) { // Draw image
    tft.pushImage(locX, locY + loopHeight, imageWidth, 1, (uint16_t *)(imageBuffer + loopHeight * imageWidth));
  }

  tft.endWrite();
}

int16_t dirX = 1;
int16_t dirY = 1;
int16_t speed = 5;

void loop() {
  delay(5);

  speed = analogRead(SPEED_POT_PIN) / 150;

  moveImage(dirX * speed, dirY * speed);

  if (locX + imageWidth >= SCREEN_WIDTH || locX <= 0) {
      dirX = -dirX;  // Reverse X direction
      setImageColor(random(0, 0x10000));
      
      // Adjust position to prevent getting stuck
      if (locX <= 0) {
          locX = 0;
      } else if (locX + imageWidth >= SCREEN_WIDTH) {
          locX = SCREEN_WIDTH - imageWidth;
      }
  }

  if (locY + imageHeight >= SCREEN_HEIGHT || locY <= 0) {
      dirY = -dirY;  // Reverse Y direction
      setImageColor(random(0, 0x10000));
      
      // Adjust position to prevent getting stuck
      if (locY <= 0) {
          locY = 0;
      } else if (locY + imageHeight >= SCREEN_HEIGHT) {
          locY = SCREEN_HEIGHT - imageHeight;
      }
  }
}

void pngCache(PNGDRAW *pDraw) {
  if (imageBuffer) {
    png.getLineAsRGB565(pDraw, (uint16_t *)(imageBuffer + pDraw->y * imageWidth), PNG_RGB565_BIG_ENDIAN, 0xffffffff);
  }
}
