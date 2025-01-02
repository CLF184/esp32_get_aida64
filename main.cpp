#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <math.h>
#include <freertos/semphr.h> // Include the FreeRTOS semaphore header

const char *ssid = "werrrrttt";
const char *password = "13719623327aa";
const char *host = "192.168.31.5"; // 主机ip地址
const int port = 9999;             // 端口号
const int dac_pin25 = 25;
const int dac_pin26 = 26;
const double maxavoltage = 1;  // 电压表最大电压
const double smoothness = 0.75; // 平滑度
const int N = 2;               // 数据长度
char item1[] = "item1";        // 数据项1
char item2[] = "item2";       // 数据项2
const String getUrl = "/sse";
int16_t dataStart = 0;
int16_t dataEnd = 0;
String dataStr;

class Item
{
public:
  double item_1;
  double item_2;
};

WiFiClient client;
Item item[N] = {0, 0};
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
// SemaphoreHandle_t semaphore; // Declare the semaphore handle

int connect()
{
  if (!client.connect(host, port)) // 使用新的端口号变量
  {
    Serial.println("Connect host failed!");
    return true;
  }
  Serial.println("host Conected!");
  return false;
}

int toint(char *item, String line)
{
  dataStart = line.indexOf(item) + strlen(item);
  dataEnd = line.indexOf("%", dataStart);
  dataStr = line.substring(dataStart, dataEnd);
  return dataStr.toInt();
}

void getitem(void *parameter)
{
  String line = "";
  char endOfHeaders[] = "\n\n";
  Item temp;
  int i;
  while (true)
  {
    client.print(String("GET ") + getUrl + " HTTP/1.1\r\n" + "Content-Type=application/json;charset=utf-8\r\n" + "Host: " + host + "\r\n" + "User-Agent=ESP32\r\n" + "Connection: close\r\n\r\n");
    // Serial.println("Get send");
    // delay(5);

    if (!client.find(endOfHeaders))
    {
      // Serial.println("No response or invalid response!");
      client.stop(); // 关闭连接
      connect();     // 重新连接
      continue;
    }

    // Serial.println("Skip headers");

    line = client.readStringUntil('\n');

    // Serial.println("Content:");
    Serial.println(line);

    temp.item_1 = toint(item1, line);
    temp.item_2 = toint(item2, line);

    portENTER_CRITICAL(&mux);
    for(i = N - 1; i > 0; i--)
    {
      item[i] = item[i - 1];
    }
    item[0] = temp;
    portEXIT_CRITICAL(&mux);

    // xSemaphoreGive(semaphore); // V operation

    /*
    Serial.print(item1);
    Serial.print(":");
    Serial.println(item[0].item_1);
    Serial.print(item2);
    Serial.print(":");
    Serial.println(item[0].item_2);
    */
  }
}

void dac(void *parameter)
{
  Item output;
  Item sum;
  int i;
  while (true)
  {
    sum = {0, 0};
    // xSemaphoreTake(semaphore, portMAX_DELAY); // P operation
    portENTER_CRITICAL(&mux);
    for (i = 0; i < N; i++)
    {
      sum.item_1 += item[i].item_1;
      sum.item_2 += item[i].item_2;
    }
    portEXIT_CRITICAL(&mux);
    output = {sum.item_1 / N * (1 - smoothness) + output.item_1 * smoothness, sum.item_2 / N * (1 - smoothness) + output.item_2 * smoothness};
    dacWrite(dac_pin25, (int)output.item_1 * maxavoltage / 1.294117647058823529);
    dacWrite(dac_pin26, (int)output.item_2 * maxavoltage / 1.294117647058823529);
    Serial.print("output1:");
    Serial.println(output.item_1);
    Serial.print("output2:");
    Serial.println(output.item_2);

    delay(100);
  }
}

void wlan()
{
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup()
{
  Serial.begin(115200);
  // put your setup code here, to run once:
  // 初始化DAC
  pinMode(dac_pin25, OUTPUT);
  pinMode(dac_pin26, OUTPUT);
  dacWrite(dac_pin25, 0);
  dacWrite(dac_pin26, 0);
  wlan();

  while (connect());

  // semaphore = xSemaphoreCreateBinary(); // Create the semaphore
  // xSemaphoreGive(semaphore); // Initialize the semaphore with a value of 1

  xTaskCreate(getitem, "GetItemTask", 4096, NULL, 1, NULL);
  xTaskCreate(dac, "DacTask", 4096, NULL, 1, NULL);
}

void loop()
{
  // Nothing to do here
}
