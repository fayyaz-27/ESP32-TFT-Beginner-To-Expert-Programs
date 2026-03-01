#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <TJpg_Decoder.h>
#include <TFT_eSPI.h>

const char* ssid="SSID";
const char* password="PASSWORD";

WebServer server(80);
TFT_eSPI tft=TFT_eSPI();

File uploadFile;
int thresholdVal=120;

// ===== segmentation =====
inline uint8_t getGray(uint16_t c){
  uint8_t r=((c>>11)&0x1F)*255/31;
  uint8_t g=((c>>5)&0x3F)*255/63;
  uint8_t b=(c&0x1F)*255/31;
  return (r*3+g*6+b)/10;
}

bool tft_output(int16_t x,int16_t y,uint16_t w,uint16_t h,uint16_t* bmp)
{
  auto gray=[&](uint16_t c){
    uint8_t r=((c>>11)&0x1F)*255/31;
    uint8_t g=((c>>5)&0x3F)*255/63;
    uint8_t b=(c&0x1F)*255/31;
    return (r*3+g*6+b)/10;
  };

  const int EDGE_THR = 40;

  for(int yy=0; yy<h; yy++)
  {
    for(int xx=0; xx<w; xx++)
    {
      int i = yy*w + xx;

      // skip borders → prevents block seams
      if(xx==0 || yy==0 || xx==w-1 || yy==h-1)
      {
        bmp[i] = 0x0000;
        continue;
      }

      int gx = gray(bmp[i]) - gray(bmp[i+1]);
      int gy = gray(bmp[i]) - gray(bmp[i+w]);

      int mag = abs(gx)+abs(gy);

      bmp[i] = (mag > EDGE_THR) ? 0xFFFF : 0x0000;
    }
  }

  tft.pushImage(x,y,w,h,bmp);
  return 1;
}

// ===== draw image =====
void drawImage()
{
  if(SPIFFS.exists("/img.jpg"))
  {
    TJpgDec.drawFsJpg(0,0,"/img.jpg");
  }
}

// ===== upload handler =====
void handleUpload()
{
  HTTPUpload& upload=server.upload();

  if(upload.status==UPLOAD_FILE_START)
  {
    uploadFile=SPIFFS.open("/img.jpg","w");
  }
  else if(upload.status==UPLOAD_FILE_WRITE)
  {
    if(uploadFile) uploadFile.write(upload.buf,upload.currentSize);
  }
  else if(upload.status==UPLOAD_FILE_END)
  {
    if(uploadFile) uploadFile.close();
    drawImage();
  }
}

// ===== html =====
String html()
{
return R"rawliteral(
<html>
<body>
<h3>Upload JPG</h3>

<form method='POST' action='/upload' enctype='multipart/form-data'>
<input type='file' name='data'>
<input type='submit'>
</form>

</body>
</html>
)rawliteral";
}

void setup()
{
  Serial.begin(115200);

  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  SPIFFS.begin(true);

  TJpgDec.setCallback(tft_output);

  WiFi.begin(ssid,password);
  while(WiFi.status()!=WL_CONNECTED) delay(500);

  Serial.println(WiFi.localIP());

  server.on("/",[](){server.send(200,"text/html",html());});

  server.on("/upload",HTTP_POST,
    [](){server.send(200,"text/plain","OK");},
    handleUpload);

  server.on("/thr",[](){
    thresholdVal=server.arg("val").toInt();
    drawImage();
    server.send(200,"text/plain","OK");
  });

  server.begin();
}

void loop()
{
  server.handleClient();
}
