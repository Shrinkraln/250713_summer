#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
//#include <LittleFS.h>
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <DHT.h>
#include <Adafruit_SSD1306.h>
#include <PubSubClient.h>

//定义led引擎
#define led_pin 12
// SPI引脚定义
#define OLED_MOSI 23   // 数据线
#define OLED_CLK 18    // 时钟线
#define OLED_DC 17     // 数据/命令选择
#define OLED_CS 5      // 片选(如不使用可设为-1
#define OLED_RESET 16  // 复位printf("printf("");");
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define DHT_DATA 4
#define DHTTYPE DHT11

#define MQTT_SERVER "192.168.222.14"
#define MQTT_PORT 1883

SPIClass mySPI = SPIClass(VSPI);  //或者说HSPI不冲突都一样
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &mySPI, OLED_DC, OLED_RESET, OLED_CS);
DHT dht(DHT_DATA, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);


float temperature = 0;
float humidity = 0;
String condition;
const char* ssid = "802";
const char* password = "18113936661";
unsigned long startAttemptTime = millis();
const unsigned long timeout = 60000;  // 60秒超时

//创建对象，监听ESP32的80端口
WebServer server(80);

//用于访问服务器根目录时，所以状态码是成功请求
//慢闪一次
void handleRoot() {
  //状态码（相当于有特殊含义的数字，表示请求成功）+内容类型：纯文本+内容
  //server.on(200,text/plain,"Hello from ESP32!");
  //显示网页，监听对象+工具类型+文件位置
  //server.serveStatic("/index", LittleFS, "/index.html");
  //r只读模式打开
  // File file = LittleFS.open("/index.html", "r");
  // server.streamFile(file, "text/html");
  // file.close();  //文件操作之后必须关闭
  String html = "<!DOCTYPE html>\n"
                "<html lang=\"en\">\n"
                "<head>\n"
                "    <meta charset=\"UTF-8\">\n"
                "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                "    <title>ESP32网页控制LED</title>\n"
                "    <script>\n"
                "     function fetchStatus (){fetch(\"/status\")  // 向 ESP32 请求 LED 状态\n"  //fetch就是访问，会激发server.on
                "        .then(res => res.text())\n"                                            //将返回对象转化为纯文本
                "        .then(text => {\n"
                "            document.getElementById(\"status\").innerText = text;\n"  //找到网页中ID为"status"的HTML元素，并将其文本内容设置为text变量的值
                "        });}\n"
                "     function fetchData(){"
                "        fetch(\"/data\")"
                "         .then(res =>res.json())\n"
                "         .then(data =>{\n"
                "             document.getElementById(\"temp\").textContent = data.temperature;\n"
                "             document.getElementById(\"humidity\").textContent = data.humidity;\n"
                "         });}\n"
                "       fetchStatus();"
                "       fetchData();"
                "       setInterval(fetchStatus,1000);"
                "       setInterval (fetchData,3000);"
                "    </script>\n"
                "</head>\n"
                "<body>\n"
                "  <h1>控制LED灯</h1>\n"
                "  <h2>当前状态：<span id=\"status\"></span></h2>\n"
                "  <button onclick=\"turnOn()\">打开</button>\n"  //点击按钮时，调用turnOn()函数，向ESP32发送"/index/on"请求
                "  <button onclick=\"turnOff()\">关闭</button>\n"
                "  \n"
                "  <script>\n"
                "    function turnOn() {\n"
                "      fetch('/index/on')\n"
                // "        .then(response => response.text())\n"
                // "        .then(text => {\n"
                // "          document.getElementById('status').innerText = 'ON';\n"
                // "        });\n"
                "    }\n"
                "    \n"
                "    function turnOff() {\n"
                "      fetch('/index/off')\n"
                // "        .then(response => response.text())\n"
                // "        .then(text => {\n"
                // "          document.getElementById('status').innerText = 'OFF';\n"
                //"        });\n"
                "    }\n"
                "  </script>\n"
                " <h2>当前温度<span id=\"temp\"></span></h2>"
                " <h2>当前湿度<span id=\"humidity\"></span></h2>"
                "</body>";
  server.send(200, "text/html", html);
}

void setupWiFi() {
  //连接WiFi
  //配置串口屏的通信波特率
  Serial.begin(115200);  //功能：初始化硬件串口（UART），设置通信波特率为115200bps
  //wroom-32等待初始化稳定
  delay(5000);  // while (!Serial) {       //wroom-32的Serial一直是true，不使用循环
                //   delay(100);           //s2/s3的Serial为false表示未初始化，延迟等待初始化或者稳定
                // }

  //开始连接
  Serial.println();
  Serial.println("******************************************************");
  Serial.print("Connecting to ");

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);                 //客户端模式
  WiFi.begin(ssid, password);          //链接wifi会自动持续重试
  WiFi.persistent(false);              //避免重复发送请求
  WiFi.setAutoReconnect(true);         //断连后自动重新连接
  WiFi.setTxPower(WIFI_POWER_8_5dBm);  //加强WiFi连接强度
  //是否链接上，加上连接超时机制
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect.");
  } else {
    Serial.println("Successfully connected!");
    Serial.println(WiFi.localIP());
  }

  //启动LittleFS管理
  // if (!LittleFS.begin()) {
  //   Serial.println("An error occurred while mounting LittleFS");
  //   return;
  // }
  //设置ESPmDNS服务器
  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }
}

void setupServeron() {
  //监听根路径，访问时调用函数
  //server.on类型注册监听不会立即执行监听事件发生的函数，但是必须在ser.begin之前，真正的监听是loop函数里的handleclient
  //直接在index里http请求获取LED状态即可
  server.on("/", handleRoot);  //是函数指针；server.on("/", handleRoot());是调用函数，执行，返回返回值，
  // //监听执行临时函数
  // server.on("/inline", []() {
  //   server.send(200, "text/plain", "this works as well");
  // });                            //是示例里的代码，无实际作用
  //访问未定义路径时执行
  server.onNotFound(handleNotFound);
  //
  server.on("/status", []() {
    server.send(200, "text/plain", digitalRead(led_pin) == HIGH ? "ON" : "OFF");
  });
  server.on("/index/on", HTTP_GET, []() {
    digitalWrite(led_pin, HIGH);
    server.send(200, "text/plain", "LED is ON");
  });

  server.on("/index/off", HTTP_GET, []() {
    digitalWrite(led_pin, LOW);
    server.send(200, "text/plain", "LED is OFF");
  });

  // 添加温湿度数据端点
  server.on("/data", HTTP_GET, []() {
    server.send(200, "application/json", condition);
  });

  //server.serveStatic("/bootstrap.min.css", LittleFS, "/bootstrap.min.css");
}
//访问未定义路径时进行
//快闪两次
void handleNotFound() {
  String message = "File Not Found\n\n";  //使用string而不是char*，一是因为后续拼接，二是自动内存管理，char*多用于小规模确定字符串
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();  //返回请求携带的参数总量
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";  //打印第i个参数名和第i个参数值
  }
  server.send(404, "text/plain", message);
  for (int i = 0; i < 2; i++) {
    digitalWrite(led_pin, HIGH);
    delay(500);
    digitalWrite(led_pin, LOW);
  }
}
//读取DHT11数据并存储
bool readSensorData() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("读取失败");
    return false;
  } else {
      condition = "{\"temperature\":" + String(temperature) + ",\"humidity\":" + String(humidity) + "}";
    return true;
  }
}
//setup里面初始化屏幕
void initialOLED() {
  // OLED初始化
  // 初始化OLED
  pinMode(OLED_RESET, OUTPUT);
  digitalWrite(OLED_RESET, LOW);
  delay(10);
  digitalWrite(OLED_RESET, HIGH);
  delay(10);
  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println("OLED初始化失败");
    while (1)
      ;
  } else {
    Serial.println("OLED初始化成功");
  }
  // SPI初始化
  mySPI.begin(OLED_CLK, -1, OLED_MOSI, OLED_CS);  //MISO用不上
  //mySPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));  // 设置为1MHz
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Hello World!");
  display.ssd1306_command(255);  // 0-255
  display.display();
}
//更新OLED屏幕数据
void updateOLED() {
  display.clearDisplay();  //清空缓冲区
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Temp: ");
  display.print(temperature);
  display.println(" 'C");
  display.print("Humidity: ");
  display.print(humidity);
  display.println("%");
  display.ssd1306_command(255);  // 0-255
  display.display();             //缓冲区上传到oled
}
//输出AHT11数据到串口监视器，排除DHT11故障
void updateSerial() {
  Serial.print("温度: ");
  Serial.print(temperature);
  Serial.print(" ℃，湿度: ");
  Serial.print(humidity);
  Serial.println(" %");
}

void subscribe(char* topic, byte* message, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) {
    msg += (char)message[i];
  }
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(msg);

  if (msg.indexOf("\"led\":\"on\"") > 0) {
    digitalWrite(LEDPIN, HIGH);
  } else if (msg.indexOf("\"led\":\"off\"") > 0) {
    digitalWrite(LEDPIN, LOW);
  }
}

void reconnectToMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe("esp32/control");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}
void setup() {
  // put your setup code here, to run once:

  //点灯
  //设置引脚为输出模式
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);

  setupWiFi();
  setupServeron();

  //启动ESO32网络服务
  server.begin();
  Serial.println("HTTP server started");

  //DHT初始化
  dht.begin();
  Serial.println("DHT初始化成功");

  initialOLED();

  //连接MQTT服务器
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(subscribe);
}

void loop() {
  // put your main code here, to run repeatedly:
  //handleClient函数实现真实的监听，server.on是注册监听对象以及触发的函数，在handleclient中才会检查监听事件对象是否发生
  server.handleClient();
  delay(1000);
  //读取数据
  readSensorData();
  //读取失败时跳出loop结束
  if (!readSensorData()) {
    return;
  }
  updateSerial();
  updateOLED();
  delay(1000);
  if (!client.connected()) {
    reconnect();
  } else {
    client.loop();
    client.publish("esp32/condition", (char*) condition.c_str());
  }
}
