#include <WiFi.h>
#include <WebServer.h>
#include <NetworkClient.h>
#include <ESPmDNS.h>
//#include <LittleFS.h>

//定义led引擎
int led_pin = 12;

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
                "     fetch(\"/status\")  // 向 ESP32 请求 LED 状态\n"
                "        .then(res => res.text())\n"//将返回对象转化为纯文本
                "        .then(text => {\n"
                "            document.getElementById(\"state\").innerText = text;\n"//找到网页中ID为"state"的HTML元素，并将其文本内容设置为text变量的值
                "        });\n"
                "    </script>\n"
                "</head>\n"
                "<body>\n"
                "  <h1>控制LED灯</h1>\n"
                "  <h2>当前状态：<span id=\"state\"></span></h2>\n"
                "  <button onclick=\"turnOn()\">打开</button>\n"//点击按钮时，调用turnOn()函数，向ESP32发送"/index/on"请求
                "  <button onclick=\"turnOff()\">关闭</button>\n"
                "  \n"
                "  <script>\n"
                "    function turnOn() {\n"
                "      fetch('/index/on')\n"
                "        .then(response => response.text())\n"
                "        .then(text => {\n"
                "          document.getElementById('state').innerText = 'ON';\n"
                "        });\n"
                "    }\n"
                "    \n"
                "    function turnOff() {\n"
                "      fetch('/index/off')\n"
                "        .then(response => response.text())\n"
                "        .then(text => {\n"
                "          document.getElementById('state').innerText = 'OFF';\n"
                "        });\n"
                "    }\n"
                "  </script>\n"
                "</body>\n"
                "</html>";

  server.send(200, "text/html", html);
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

void setup() {
  // put your setup code here, to run once:

  //点灯
  //设置引脚为输出模式
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);

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

  //server.serveStatic("/bootstrap.min.css", LittleFS, "/bootstrap.min.css");
  //启动ESO32网络服务
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // put your main code here, to run repeatedly:
  //handleClient函数实现真实的监听，server.on是注册监听对象以及触发的函数，在handleclient中才会检查监听事件对象是否发生
  server.handleClient();
  delay(2);
}
