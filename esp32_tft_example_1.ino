#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

// RGB565 raw channel values
uint8_t r = 31;   // 0-31
uint8_t g = 0;    // 0-63
uint8_t b = 0;    // 0-31

// convert 5-6-5 to uint16 color
uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
  return (r << 11) | (g << 5) | b;
}

void drawColor()
{
  uint16_t color = rgb565(r, g, b);
  tft.fillScreen(color);

  Serial.print("R:");
  Serial.print(r);
  Serial.print(" G:");
  Serial.print(g);
  Serial.print(" B:");
  Serial.println(b);
}

void setup()
{
  Serial.begin(115200);

  tft.begin();
  tft.setRotation(3);

  drawColor();

  Serial.println("Send: R G B   (example: 10 40 5)");
}


void loop()
{
  if (Serial.available())
  {
    String line = Serial.readStringUntil('\n');
    line.trim();

    int nr, ng, nb;

    if (sscanf(line.c_str(), "%d %d %d", &nr, &ng, &nb) == 3)
    {
      r = constrain(nr, 0, 31);
      g = constrain(ng, 0, 63);
      b = constrain(nb, 0, 31);

      drawColor();
    }
  }
}
