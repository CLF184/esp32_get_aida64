#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <math.h>

const char *ssid = "xxxx";//wifi名称
const char *password = "xxxx";//wifi密码
const char *host = "xxxx";//主机ip地址
const int port = xxxx;  // 新增的端口号变量
const int dac_pin25 = 25;
const int dac_pin26 = 26;
const int maxavoltage = 1;//电压表最大电压
const float amplitude = 0.2;//变化幅度
const double step = 0.0129411764705882;//8bit-dac每1bit的电压

float item_1 = 0;
float item_2 = 0;
int output1_old = 0;
int output2_old = 0;

WiFiClient client;

int lerp(float item, float old, float amplitude, float v) {
  if(abs(item-old*step*100)>10){
    if((item-old*step*100)>0){
      return old+(amplitude*v)/step;
    }
    else{
      return old-(amplitude*v)/step;
    }
  }
  else {
    return (item*v/100)/step;
  }
}

void getitem() {
  String getUrl = "/sse";
  client.print(String("GET ") + getUrl + " HTTP/1.1\r\n" + "Content-Type=application/json;charset=utf-8\r\n" + "Host: " + host + "\r\n" + "User-Agent=ESP32\r\n" + "Connection: close\r\n\r\n");
  Serial.println("Get send");
  delay(10);
  char endOfHeaders[] = "\n\n";
  bool ok = client.find(endOfHeaders);
  if (!ok) {
    Serial.println("No response or invalid response!");
    client.stop();  // 关闭连接
    connect();      // 重新连接
    return;
  }
  Serial.println("Skip headers");

  String line = "";

  line += client.readStringUntil('\n');

  Serial.println("Content:");
  Serial.println(line);

  int16_t dataStart = 0;
  int16_t dataEnd = 0;
  String dataStr;

  char item1[] = "item1";
  dataStart = line.indexOf(item1) + strlen(item1);
  dataEnd = line.indexOf("%", dataStart);
  dataStr = line.substring(dataStart, dataEnd);
  item_1 = dataStr.toInt();

  char item2[] = "item2";
  dataStart = line.indexOf(item2) + strlen(item2);
  dataEnd = line.indexOf("%", dataStart);
  dataStr = line.substring(dataStart, dataEnd);
  item_2 = dataStr.toInt();

  Serial.print("item1:");
  Serial.println(item_1);
  Serial.print("item2:");
  Serial.println(item_2);
}

void dac() {
  int i=0;
  int output1 = lerp(item_1, output1_old, amplitude, maxavoltage);
  int output2 = lerp(item_2, output2_old, amplitude, maxavoltage);
  if (i == 100) {
    if (output1 == output1_old && output2 == output2_old) {
      output1 = 0;
      output2 = 0;
    }
    i = 0;
  }
  Serial.println(output1);
  Serial.println(output2);
  dacWrite(dac_pin25, output1);
  dacWrite(dac_pin26, output2);
  i++;
  output1_old = output1;
  output2_old = output2;
}

void connect() {
  if (!client.connect(host, port))  // 使用新的端口号变量
  {
    Serial.println("Connect host failed!");
    return;
  }
  Serial.println("host Conected!");
}
void wlan() {
  Serial.begin(115200);

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("");
  Serial.println("WiFi Conected!");
}

void setup() {
  // put your setup code here, to run once:
  // 初始化DAC
  pinMode(dac_pin25, OUTPUT);
  pinMode(dac_pin26, OUTPUT);
  dacWrite(dac_pin25, 0);
  dacWrite(dac_pin26, 0);
  wlan();
  connect();
}

void loop() {
  getitem();
  dac();
}
