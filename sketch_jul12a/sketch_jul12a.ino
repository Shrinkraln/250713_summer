//定义led引擎
#include <WiFi.h>
int led_pin = 12;
const char* ssid = "802";
const char* password = "18113936661";
unsigned long startAttemptTime = millis();
const unsigned long timeout = 60000;  // 60秒超时

void setup() {
  // put your setup code here, to run once:

  //点灯
  //设置引脚为输出模式
  pinMode(led_pin, OUTPUT);


  //连接WiFi
  //配置串口屏的通信波特率
  Serial.begin(115200);  //功能：初始化硬件串口（UART），设置通信波特率为115200bps
  //wroom-32等待初始化稳定
  delay(2000);  // while (!Serial) {       //wroom-32的Serial一直是true，不使用循环
                //   delay(100);           //s2/s3的Serial为false表示未初始化，延迟等待初始化或者稳定
                // }

  //开始连接
  Serial.println();
  Serial.println("******************************************************");
  Serial.print("Connecting to ");

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);  //链接wifi会自动持续重试
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
}

void loop() {
  // put your main code here, to run repeatedly:

  //点灯
  //点亮led灯，给引脚赋一个高电平的值，HIGH 1 LOW 0
  digitalWrite(led_pin, HIGH);
  //延时1s
  delay(1000);
  //熄灭
  digitalWrite(led_pin, LOW);
  //延时
  delay(1000);
}
