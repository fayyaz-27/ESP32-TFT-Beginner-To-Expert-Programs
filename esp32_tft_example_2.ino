#include <WiFi.h>
#include <WebServer.h>
#include <TFT_eSPI.h>

const char* ssid = "SSID";
const char* password = "PASSWORD";

WebServer server(80);
TFT_eSPI tft = TFT_eSPI();

// RGB565 raw channels
uint8_t r = 31;   // 0-31
uint8_t g = 0;    // 0-63
uint8_t b = 0;    // 0-31

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
  return (r << 11) | (g << 5) | b;
}

void drawColor()
{
  tft.fillScreen(rgb565(r,g,b));
}

String html()
{
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body { font-family: sans-serif; text-align:center; }
.slider { width:90%; }
</style>
</head>
<body>

<h2>ESP32 RGB565 Control</h2>

R (0-31)<br>
<input type="range" min="0" max="31" value="0" class="slider" id="r" oninput="send()"><br>

G (0-63)<br>
<input type="range" min="0" max="63" value="0" class="slider" id="g" oninput="send()"><br>

B (0-31)<br>
<input type="range" min="0" max="31" value="0" class="slider" id="b" oninput="send()"><br>

<script>
function send(){
  let r=document.getElementById('r').value;
  let g=document.getElementById('g').value;
  let b=document.getElementById('b').value;
  fetch(`/set?r=${r}&g=${g}&b=${b}`);
}
</script>

</body>
</html>
)rawliteral";
}

void handleRoot()
{
  server.send(200, "text/html", html());
}

void handleSet()
{
  if(server.hasArg("r")) r = constrain(server.arg("r").toInt(),0,31);
  if(server.hasArg("g")) g = constrain(server.arg("g").toInt(),0,63);
  if(server.hasArg("b")) b = constrain(server.arg("b").toInt(),0,31);

  drawColor();

  server.send(200,"text/plain","OK");
}

void setup()
{
  Serial.begin(115200);

  tft.begin();
  tft.setRotation(3);
  drawColor();

  WiFi.begin(ssid,password);
  Serial.print("Connecting");

  while(WiFi.status()!=WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.begin();
}

void loop()
{
  server.handleClient();
}
