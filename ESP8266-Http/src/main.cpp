/**
 * @Description: 航天龙梦 1915冷启动测试 ESP8266程序 1拖1 http客户端
 * @Date:2021/5/27
 * @Last:2021/5/31
 * @Author: JackyMao
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
// #include <ESP8266mDNS.h>
#include <Ticker.h>
#include <FS.h>

/***********WIFI配置***********/
#define STASSID "htlm"                  // 这里设置wifi账号
#define STAPSK  "htlm2020"              // 这里设置wifi密码

/**********状态机状态**********/
#define INIT            0               // ESP初始化
#define AC_ON	        1               // 继电器ON
#define POWER_ON        2               // PC ON
#define POWER_OFF_CHECK 3               // PC OFF
#define AC_OFF	        4               // 继电器OFF
#define TEST_ERR        5               // 测试失败
#define TEST_PASS       6               // 测试通过

/**
 * 3.3V -> 稳定电压
 * PWR -> 开机PWR接口
 * GND -> 接地
*/
#define RELAY           4               // 继电器控制
#define PWR             5               // 开机PWR接口
#define ISON            12              // 检测是否开机成功 返回1开机状态 返回0关机状态
#define LED             16              // 状态灯显示

/*---------- 测试数据参数 ----------*/
unsigned int Times_counts = 50;         // 测试次数
unsigned int StartDelay_T = 10;         // AC通电延时时间(秒)
unsigned int Start_T      = 1000;       // AC通电持续时间(秒)
unsigned int CloseDelay_T = 10;         // AC断电冷却时间(秒)
unsigned int TestTimesNow = 0;          // 当前测试次数
bool isupdate = false;                  // 是否需要升级固件
String update_url ="";                  // OTA升级文件地址


const char* ssid = STASSID;             // 账号
const char* password = STAPSK;          // 密码

const char* host_ip = "172.16.5.11";    // 需要连接的服务器IP
const unsigned int httpport = 8088;     // 需要连接的服务器端口

unsigned char STATE = 0;                // 测试状态变量

/*---------- Flash数据读取 ----------*/
String msdnfile_path = "/Cold_test/MSDNname.txt";  // MSDN名字数据存放位置
String cfgfile_path = "/Cold_test/config.txt"; // 测试数据参数存放位置
String MSDNname = "";                   // msdn域名

Ticker flipper;                         // 定时器
WiFiServer server(5045);                // 建立服务器用于接收web的表单信息

/*---------- 函数申明 ----------*/
void update_started();
void update_finished();
void update_progress(int cur, int total);
void update_error(int err);
void FSWriteFile(String filepath,String Data);
void FSReadFile(String filepath);
void flip_callback();
void response2Json(int type,String json);
void wifiClientRequest(int testTimes,int status,int ac1,int ac2,int ac3,int presetTimes);
void wifiServerResponse();
void coldStateControl();
void wifiInit();
void ESP_ProUpdate();

/**
 * @method update_started()
 * @description httpupdate 无线升级程序启动
 */
void update_started() {
  Serial.println("CALLBACK:  HTTP update process started");
}

/**
 * @method update_finished()
 * @description httpupdate 无线升级程序结束
 */
void update_finished() {
  Serial.println("CALLBACK:  HTTP update process finished");
}

/**
 * @method update_progress()
 * @description httpupdate 无线升级程序正在执行
 */
void update_progress(int cur, int total) {
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}

/**
 * @method update_error()
 * @description httpupdate 无线升级程序报错
 */
void update_error(int err) {
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
}

/**
 * @method FSWriteFile()
 * @description FLAS写底层文件
 */
void FSWriteFile(String filepath,String Data){
  Serial.println("****************写操作开始******************");
  
  File writeFile = SPIFFS.open(filepath,"w");//建立File对象，并表示将要进行“写”操作
  writeFile.println(Data);//写
  writeFile.close();//写操作完成后关闭文件
  
  Serial.println("SPIFFS Write Finished");
  Serial.println("****************写操作完成******************\n");
}


/**
 * @method FSReadFile()
 * @description FLASH读底层文件
 */
void FSReadFile(String filepath,int flag){
  Serial.println("****************读操作开始******************");
  String cfgstr = "";
  if(SPIFFS.exists(filepath)){
    Serial.print(filepath);
    Serial.println(" Found");
    File readFile = SPIFFS.open(filepath,"r");
    if (flag)
    {
        for(int i = 0; i<readFile.size()-2;i++)
            MSDNname += String((char)readFile.read());
        Serial.println(MSDNname);
    }
    else
    {
        for(int i = 0; i<readFile.size()-2;i++)
            cfgstr += String((char)readFile.read());
        Serial.println(cfgstr);
        response2Json(2,cfgstr);
    }   
  }else{
    Serial.print(filepath);
    Serial.println(" Not Found");
  }
  Serial.println("****************读操作完成******************\n");
}

/**
 * @method flip_callback()
 * @description 定时器回调函数
 */
void flip_callback(){
  if(digitalRead(ISON)){
    Serial.println("Test failed,times out!!"); 
    STATE = TEST_ERR;
  } 
  flipper.detach();
}

/**
 * @method response2Json()
 * @description 接收的数据转成JSON格式并解析处理
 * @type 0->wifiClientRequest  1->wifiServerResponse
 */
void response2Json(int type,String json){
    if (type==0){
        // JSON_OBJECT_SIZE(数据数量)
        // + xxx 复制数据时需要额外的空间
        const size_t capacity = JSON_OBJECT_SIZE(9) + 150;
        DynamicJsonDocument doc(capacity);

        // 反序列化数组
        deserializeJson(doc,json);

        // 解析后的数据
        int codeInt = doc["code"].as<int>();
        String msgStr = doc["msg"].as<String>();

        Serial.println(codeInt);
        Serial.println(msgStr);
    }
    else if(type == 1){
        // JSON_OBJECT_SIZE(数据数量)
        // + xxx 复制数据时需要额外的空间
        const size_t capacity = JSON_OBJECT_SIZE(8) + 160;
        DynamicJsonDocument doc(capacity);

        // 反序列化数组
        deserializeJson(doc,json);

        // 解析后的数据
        isupdate = doc["isUpdate"].as<bool>();
        if (isupdate){
            update_url = doc["fileUrl"].as<String>();
            
            Serial.print("ready to update ");
            Serial.println(update_url);
        }
        else{
            StartDelay_T = doc["ac1"].as<int>();
            Start_T = doc["ac2"].as<int>();
            CloseDelay_T = doc["ac3"].as<int>();
            Times_counts = doc["presetTimes"].as<int>();
            
            Serial.println(StartDelay_T);
            Serial.println(Start_T);
            Serial.println(CloseDelay_T);
            Serial.println(Times_counts);
            FSWriteFile(cfgfile_path,"{\"ac1\":"+String(StartDelay_T)+",\"ac2\":"+String(Start_T)+",\"ac3\":"+String(CloseDelay_T)+",\"presetTimes\":"+String(Times_counts));
        }
    }else if(type==2){
        const size_t capacity = JSON_OBJECT_SIZE(4) + 80;
        DynamicJsonDocument doc(capacity);

        // 反序列化数组
        deserializeJson(doc,json);

        // 解析后的数据
        StartDelay_T = doc["ac1"].as<int>();
        Start_T = doc["ac2"].as<int>();
        CloseDelay_T = doc["ac3"].as<int>();
        Times_counts = doc["presetTimes"].as<int>();
        Serial.println(123123123);
        Serial.println(StartDelay_T);
        Serial.println(Start_T);
        Serial.println(CloseDelay_T);
        Serial.println(Times_counts);
    }
}

/**
 * @method wifiClientRequest()
 * @description WIFI客户端处理
 */
void wifiClientRequest(int testTimes,int status,int ac1,int ac2,int ac3,int presetTimes){      
    WiFiClient client;              // TCP 客户端
    char urlstr[100];
    sprintf(urlstr,"&testTimes=%d&status=%d&ac1=%d&ac2=%d&ac3=%d&presetTimes=%d",testTimes,status,ac1,ac2,ac3,presetTimes);
    String url = "/esp/info/sendLog?msdnName=" + MSDNname +String(urlstr);
    // 建立字符串，用于HTTP请求
    String httpRequest = String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host_ip + "\r\n" + "Connection: close\r\n" + "\r\n";
                            
    Serial.print("Connecting to "); 
    Serial.println(host_ip); 
    
    if (client.connect(host_ip, httpport)) {        //如果连接失败则串口输出信息告知用户然后返回loop
        Serial.println("Http connected Success");
        client.print(httpRequest);                  // 向服务器发送HTTP请求
        Serial.println("Sending request: ");        // 通过串口输出HTTP请求信息内容以便查阅
        Serial.println(httpRequest);        
        
        // 服务器接收返回的数据
        Serial.println("Http server Response:");        
        while (client.connected() || client.available()){ 
            if (client.available()){
                // 取其精华，去其糟粕,就留json数据
                for (int i = 0; i < 6; i++)
                    client.readStringUntil('\r');
                client.readStringUntil('\n');
                String line = client.readString();  // 读取数据
                Serial.println(line);
                response2Json(0,line);              // Json处理 
                
            }
        }
    } else{
        Serial.println("Http connected failed");
    }
    client.stop();                         
}

/**
 * @method wifiServerResponse()
 * @description WiFi服务端数据处理
 */
void wifiServerResponse(){
    Serial.println("---------------------------");
    // 检测是否有客户端连接
    if (server.hasClient()){
        WiFiClient client = server.available();
        // 如果没有连接，退出
        if (!client)
            return;
        Serial.println("new client");
        // 设置超时时间为1秒
        client.setTimeout(1000);

        //打印TCP server状态码
        Serial.print("Server status:");
        Serial.println(server.status());

        //读取客户端发起的TCP请求
        String req = client.readString();
        Serial.println("request: ");
        Serial.println(req);
        response2Json(1,req);
        
        //读取剩余的内容,用于清除缓存
        while (client.available()) {
            client.read();
        }
        client.print("{\"code\":0,\"msg\":\"success\"}"); // 回复数据 code不等于0  或者msg不为success  代表传输失败
        wifiClientRequest(TestTimesNow,0,StartDelay_T,Start_T,CloseDelay_T,Times_counts);
    }
}

/**
 * @method coldStateControl()
 * @description 冷启动状态控制函数
 */
void coldStateControl(){
  char uart_str[20];                      // 串口数据
  switch (STATE)
  {
    /*-----------等待StartDelay_T秒进入继电器使能环节-----------*/
    case INIT:{
      TestTimesNow+=1;
      if (TestTimesNow <= Times_counts)
        wifiClientRequest(TestTimesNow,0,StartDelay_T,Start_T,CloseDelay_T,Times_counts);
      else
        break;
      
      sprintf(uart_str,"wait %d seconds to AC_ON",StartDelay_T);
     
      Serial.println(uart_str);
      
      
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
      digitalWrite(LED,LOW);                          // AC上电灯亮
      digitalWrite(RELAY,HIGH);                       // 继电器上电   
      delay(1000);      
      Serial.println("Enabled RELAY successful");
    //   wifiClientRequest(MSDNname,TestTimesNow,0,StartDelay_T,Start_T,CloseDelay_T,Times_counts);

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
        // wifiClientRequest(MSDNname,TestTimesNow,0,StartDelay_T,Start_T,CloseDelay_T,Times_counts);
        STATE = POWER_OFF_CHECK;
        flipper.attach(Start_T,flip_callback);
      }
      break;
    }
    /*-----------等待电脑关机信息-----------*/
    case POWER_OFF_CHECK:{
      if(digitalRead(ISON)){
        Serial.println("The computer is turning on now"); 
        delay(500);
      }
      else{
        Serial.println("The computer is turning off now");
        STATE = TEST_PASS;
      }
    //   wifiClientRequest(MSDNname,TestTimesNow,0,StartDelay_T,Start_T,CloseDelay_T,Times_counts);
      break;  
    }
    /*-----------测试失败-----------*/
    case TEST_ERR:{
      Serial.println("TEST_ERR");
      wifiClientRequest(TestTimesNow,-1,StartDelay_T,Start_T,CloseDelay_T,Times_counts);
      break;
    }
    /*-----------测试通过-----------*/
    case TEST_PASS:{
      STATE = AC_OFF;
      Serial.println("TEST_PASS");
      flipper.detach();
      wifiClientRequest(TestTimesNow,1,StartDelay_T,Start_T,CloseDelay_T,Times_counts);
      break;
    }
    /*-----------AC断电-----------*/
    case AC_OFF:{
      Serial.println("Stop RELAY...");
      digitalWrite(LED,HIGH);
      digitalWrite(RELAY,LOW);
      Serial.println("Stoped RELAY successful");
    //   wifiClientRequest(MSDNname,TestTimesNow,0,StartDelay_T,Start_T,CloseDelay_T,Times_counts);
      for (int i = 0; i < CloseDelay_T; i++)
      {
        delay(1000);
        sprintf(uart_str,"wait %d seconds to INIT(ALL %d)",CloseDelay_T-i,CloseDelay_T);
        Serial.println(uart_str);
      }
      STATE = INIT;
      break;
    }
  }
  
}

/**
 * @method wifiInit()
 * @description 初始化WIFI
 */
void wifiInit(){
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
    // if (MDNS.begin(MSDNname)) {
    //     Serial.println("MDNS responder started");
    // }
}

/**
 * @method ESP_ProUpdate()
 * @description 实现OTA更新固件程序
 */
void ESP_ProUpdate(){
    if (isupdate)
    {

        ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);

        // Add optional callback notifiers
        ESPhttpUpdate.onStart(update_started);
        ESPhttpUpdate.onEnd(update_finished);
        ESPhttpUpdate.onProgress(update_progress);
        ESPhttpUpdate.onError(update_error);

        t_httpUpdate_return ret = ESPhttpUpdate.update(update_url);

        switch (ret) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
            break;

        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("HTTP_UPDATE_NO_UPDATES");
            break;

        case HTTP_UPDATE_OK:
            Serial.println("HTTP_UPDATE_OK");
            break;
        }
    }
    
     
}

void setup () {
    pinMode(RELAY, OUTPUT);
    pinMode(PWR, OUTPUT);
    pinMode(ISON,INPUT);
    pinMode(LED, OUTPUT);
    //启动SPIFFS
    if(SPIFFS.begin()){
        Serial.println("SPIFFS Started");
    }else{
        Serial.println("SPIFFS Failed to Start");
        return;
    }
    Serial.begin(9600);                 // 开启串口 9600
    FSReadFile(msdnfile_path,1);          // 读msdn名
    FSReadFile(cfgfile_path,0);           // 读参数配置
    wifiInit();                         // 初始化WiFi
    server.begin();
}

void loop() {
    if(TestTimesNow-1 == Times_counts)
    {
        TestTimesNow = 0;
        wifiServerResponse();
        digitalWrite(LED, LOW); 
        delay(100);
        digitalWrite(LED, HIGH); 
        delay(500);
    }
    else
    {
        coldStateControl();
        wifiServerResponse();
        ESP_ProUpdate();
        Serial.println("---------------------------");
        Serial.println(TestTimesNow);
        Serial.println(Times_counts);
        if (STATE == TEST_ERR) {
            coldStateControl();
            while (1) {
                digitalWrite(LED, LOW); 
                delay(100);
                digitalWrite(LED, HIGH); 
                delay(500);
            }
        } 
    } 
}