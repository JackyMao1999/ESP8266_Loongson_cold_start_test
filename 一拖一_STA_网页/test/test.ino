/**
 * @Description: 航天龙梦 1915冷启动测试 ESP8266程序 1拖1
 * @Author: JackyMao
 */
#include "postform.h"
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

/***********WIFI配置***********/
#define STASSID "htlm"
#define STAPSK  "htlm2020"

/**********状态机状态**********/
#define INIT            0   // ESP初始化
#define AC_ON	          1   // 继电器ON
#define POWER_ON        2   // PC ON
#define POWER_OFF_CHECK 3   // PC OFF
#define AC_OFF	        4   // 继电器OFF
#define TEST_ERR        5   // 测试失败
#define TEST_PASS       6   // 测试通过

/**
 * 3.3V -> 稳定电压
 * PWR -> 开机PWR接口
 * GND -> 接地
*/
#define RELAY           4    // 继电器控制
#define PWR             5    // 开机PWR接口
#define ISON            10   // 检测是否开机成功 返回1开机状态 返回0关机状态
#define LED             16   // 状态灯显示

/**
 * StartDelay_T -> AC通电延时时间(秒)
 * Start_T      -> AC通电持续时间(秒)
 * CloseDelay_T -> AC断电冷却时间(秒)
 */
int StartDelay_T = 5;
int Start_T      = 10;
int CloseDelay_T = 10;

const char* ssid = STASSID;
const char* password = STAPSK;


unsigned char STATE = -1;    // 测试状态变量

ESP8266WebServer server(80);


/**
 * @method STATE_CONTROL()
 * @description 状态控制函数
 */
void STATE_CONTROL(){
  char str[20];
  switch (STATE)
  {
    /*-----------等待StartDelay_T秒进入继电器使能环节-----------*/
    case INIT:{
      sprintf(str,"wait %d seconds to AC_ON",StartDelay_T);
      Serial.println(str);
      
      server.send(200, "text/plain",str);
      for (int i = 0; i < StartDelay_T; i++)
      {
        digitalWrite(LED, LOW); 
        delay(StartDelay_T*500/StartDelay_T);
        digitalWrite(LED, HIGH); 
        delay(StartDelay_T*500/StartDelay_T);          
      }
      STATE = AC_ON;
      break;
    }
    /*-----------继电器使能Start_T秒-----------*/
    case AC_ON:{
      Serial.println("Enable RELAY...");
      server.send(200, "text/plain", str);
      digitalWrite(LED,LOW);                          // AC上电灯亮
      digitalWrite(RELAY,HIGH);                       // 继电器上电   
      delay(1000);      
      Serial.println("Enabled RELAY successful");
      
      server.send(200, "text/plain", "Enabled RELAY successful");
      // delay(Start_T*1000);
      STATE = POWER_ON;
      break;
    }
    /*-----------电脑开机-----------*/
    case POWER_ON:{
      Serial.println("Start the computer...");
      // 第一次激活启动测试
      digitalWrite(PWR,LOW);
      delay(500);
      digitalWrite(PWR,HIGH);
      delay(100);
      // 如果检测到ISON还是0 再激活启动一次
      if(!digitalRead(ISON)){
        digitalWrite(PWR,LOW);
        delay(500);
        digitalWrite(PWR,HIGH);
        delay(100);
      }
      if(digitalRead(ISON)){
        Serial.println("Started the computer successful");
        server.send(200, "text/plain", "Started the computer successful");
        STATE = POWER_OFF_CHECK;
      }
      break;
    }
    /*-----------等待电脑关机信息-----------*/
    case POWER_OFF_CHECK:{
      if(digitalRead(ISON)){
        Serial.println("The computer is turning on now"); 
        server.send(200, "text/plain", "The computer is turning on now");
        delay(500);
      }
      else{
        Serial.println("The computer is turning off now");
        server.send(200, "text/plain", "The computer is turning off now");
        STATE = TEST_PASS;
      }
      break;  
    }
    /*-----------测试失败-----------*/
    case TEST_ERR:{
      Serial.println("TEST_ERR");
          
      server.send(200, "text/plain", "TEST_ERR");
      delay(1000);
      break;
    }
    /*-----------测试通过-----------*/
    case TEST_PASS:{
      STATE = AC_OFF;
      Serial.println("TEST_PASS");

      server.send(200, "text/plain", "TEST_PASS");
      break;
    }
    /*-----------AC断电-----------*/
    case AC_OFF:{
      Serial.println("Stop RELAY...");
      digitalWrite(LED,HIGH);
      digitalWrite(RELAY,LOW);
      Serial.println("Stoped RELAY successful");
      
      server.send(200, "text/plain", "Stoped RELAY successful");
      for (int i = 0; i < CloseDelay_T; i++)
      {
        delay(1000);
        sprintf(str,"wait %d seconds to INIT(ALL %d)",CloseDelay_T-i,CloseDelay_T);
        Serial.println(str);

        server.send(200, "text/plain",str);
      }
      STATE = INIT;
      break;
    }
  }
  
}
/**
 * @method WIFI_INIT()
 * @description 初始化WIFI
 */
void WIFI_INIT(){
  WiFi.mode(WIFI_STA);        // WIFI模式为STA
  WiFi.begin(ssid, password); // 连接WIFI

  // 等待WIFI连接成功
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // 添加域名
  if (MDNS.begin("ESP8266_1")) {
    Serial.println("MDNS responder started");
  }
}
/**
 * @method HANDLENOTFOUND()
 * @description 网页 没有找到
 */
void HANDLE_NOT_FOUND() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}
/**
 * @method handleForm()
 * @description 网页 表单测试
 */
void handleForm() {
  String s1 = server.arg("StartDelay_T");
  String s2 = server.arg("Start_T");
  String s3 = server.arg("CloseDelay_T");
  StartDelay_T = atoi(s1.c_str());
  Start_T = atoi(s2.c_str());
  CloseDelay_T = atoi(s3.c_str());
  String message = "StartDelay_T:" + s1 + "\n" + "Start_T:" + s2 + "\n" + "CloseDelay_T:" + s3;
  server.send(200, "text/plain", message); 
}

/**
 * @method handleRoot()
 * @description 网页 刷新数据
 */
void handleRoot(){
  server.send(200, "text/html", postForms);
}

/**
 * @method Set_PC()
 * @description 网页 控制设置电脑启动
 */
void Set_PC(){
  String state ="";
  String pcset = server.arg("pc_set");
  if(pcset == "1")
  {
    Serial.print("open"); 
    STATE = INIT;
    state = "ON";
  }
  else
  {
    Serial.print("close"); 
    STATE = AC_OFF;
    state = "OFF";
  }
  server.send(200, "text/plane", state);
  
}
/**
 * @method handleTime()
 * @description 网页 更新延时时间数据
 */
void handleTime(){
  char str[10];
  sprintf(str,"%d,%d,%d",StartDelay_T,Start_T,CloseDelay_T);
  server.send(200, "text/plain", str);
}
/**
 * @method File2read()
 * @description 网页 读文件HTML代码
 */

void setup () {
  pinMode(RELAY, OUTPUT);
  pinMode(PWR, OUTPUT);
  pinMode(ISON,INPUT);
  pinMode(LED, OUTPUT);
  
  Serial.begin(9600);         // 开启串口 9600
  WIFI_INIT();
  server.onNotFound(HANDLE_NOT_FOUND);
  server.begin();
  server.on("/",handleRoot);
  server.on("/uart",STATE_CONTROL);
  server.on("/PC_set",Set_PC);
  server.on("/ACtime",handleTime);
  server.on("/postform",handleForm);
  
  Serial.println("HTTP server started");
}


void loop() {
  //STATE_CONTROL(); 
  server.handleClient();
  MDNS.update();
  //MDNS.update();
}
