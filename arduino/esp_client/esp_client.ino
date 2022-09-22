#include "ESP8266WiFi.h"
#include <math.h>

char Spliter = '!';

// Temper
#define LED D6
#define BUZZER D5
const int thermistor = A0; 

int warn1 = 33;
int warn2 = 50;

float ParmB = 3950.0;// B 파라미터
float R100 = 3300.0; // 데이터시트상 R100 = 3.3k옴

// Client
const char* ssid     = "hapgang 2.4G";
const char* password = "12341234";
String clientId = "N3";
#define HOST_NAME   "192.168.0.11"
#define HOST_PORT   8092
bool checkConnect = false;
WiFiClient client;
bool messageChecked = false;
bool isWarn = false;
bool isEmergency = false;
bool isEmergencyIng = false;
String state = "nomal";
// Nomal
// Warn
// Emergency
// EmergencyIng

float preT = 0;
float recentT = 0;
bool isRise = false;

unsigned long recentMillis = millis();
unsigned long emergencyStartMillis = millis();
unsigned long emergencyDelay = 0;

void setup() {
  Serial.begin(115200);
  
  // Temper
  pinMode(LED, OUTPUT); 
  pinMode(BUZZER, OUTPUT); 

  // Client
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  delay(500);
  recentMillis = millis();
  
  recentT = temper();

  // 상승중 or 감소중
  if(recentT > preT){
    isRise = true;
  }
  else{
    isRise = false;
  }

  // 서버 접속 채크
  if(!client.connected()){
    if(!client.connect(HOST_NAME, HOST_PORT))
    {
      //서버 접속에 실패
      Serial.println("connection failed");
      return;
    }
    else{
    }
  }
  // 서버 접속 성공
  if(client.connected()){
    if(!checkConnect){
      Serial.println("connection");
    }
    checkConnect = true;
  }

  // 상태
  if(recentT>warn1){
    isWarn = true;
  }
  if(recentT>warn2 &&!isEmergency){
    isEmergency = true;
    messageChecked = false;
  }
  if(600000 > recentMillis - emergencyStartMillis && recentMillis - emergencyStartMillis > 10000 && !isEmergencyIng){
    isEmergencyIng = true;
    messageChecked = false;
  }
  
  if(recentT < (warn2 -10) && messageChecked && isEmergency){
    isEmergency  = false;
    isEmergencyIng = false;
    messageChecked = false;
    sendWarnning(recentT, "/emergencyClear");
  }
  if(recentT < (warn1 - 5) && messageChecked && isWarn){
    isWarn = false;
    messageChecked = false;
    sendWarnning(recentT, "/warnClear");
  }

  // send
  if(isWarn && !messageChecked && !isEmergency && !isEmergencyIng){
    messageChecked = sendWarnning(recentT, "/warn");
  }
  if(isEmergency && !messageChecked && !isEmergencyIng){
    messageChecked = sendWarnning(recentT, "/emergency");
  }
  if(isEmergencyIng && !messageChecked){
    messageChecked = sendWarnning(recentT, "/emergencyIng");
  }

  /// receive
  if(client.available()){
    String recevbline = client.readStringUntil('\n');
    Serial.println(recevbline);
    if(recevbline != "" || recevbline.length() != 0){
      int maxIndex = countChar(recevbline, Spliter);
      String recevList[maxIndex+1];
      stringSplit(recevbline, Spliter, recevList);
      if(recevList[1] == "/checkV"){
        String message = makeStringSocket(clientId, "/sendT", String(recentT, 2));
        char charList[message.length()+1];
        message.toCharArray(charList, message.length());
        Serial.println(charList);
        client.write(charList);
      }
      if(recevList[1] == "/checkA"){
        // do Alarm
      }
      if(recevList[1] == "/checkId"){
        String message = makeStringSocket(clientId, "/sendId", String(recentT, 2));
        char charList[message.length()+1];
        message.toCharArray(charList, message.length());
        Serial.println(charList);
        client.write(charList);
      }
      if(recevList[1] == "/clear"){
        clearBool();
      }
    }
  }

  // to check Time
  preT = recentT;
  if(recentT<=warn2){
    emergencyStartMillis = millis();
  }
  if(recentMillis < emergencyStartMillis){
    Serial.print("emergency Ing : ");
    emergencyDelay = 0;
    Serial.println(emergencyDelay);
  }
  else{
    Serial.print("emergency Ing : ");
    emergencyDelay = recentMillis - emergencyStartMillis;
    Serial.println(emergencyDelay);
  }

  if(recentT<=warn1){
    digitalWrite(LED_BUILTIN, HIGH);
  }
  else{
    digitalWrite(LED_BUILTIN, LOW);
  }
}


void clearBool(){
  emergencyStartMillis = millis();
  messageChecked = false;
  isEmergency = false;
  isEmergencyIng = false;
  isWarn = false;
}

float temper(){
  float voltage_value = analogRead(thermistor);
  float voltage = (3.3 * voltage_value) / 1023.0;

  float R = (3.3*10000.0)/voltage - 10000.0; // 10000은 10k 저항값
  float T = 0;
  if (R < 10){
    T = 550;
  }
  else{
    T = (ParmB/(log(R/R100)+(ParmB/(273.15+100.0))))-273.15;
  }
  
  Serial.print("volt: ");
  Serial.print(voltage);
  Serial.print(" C, ");
  Serial.print("Thermis_R: ");
  Serial.print(R);
  Serial.print(" ohm, ");
  Serial.print("Temp: ");
  Serial.print(T);
  Serial.println(" C");
  return T;
}

int countChar(String str, char chr){
  int cnt = 0;
  for(auto x : str){
    if(x == chr){
      cnt++;
    }
  }
  return cnt;
}

void stringSplit(String sData, char cSeparator, String* resultList){

  int indexCount = 0;
  
  /////// 스플릿 구현
  int nCount = 0;
  int nGetIndex = 0 ;
  //임시저장
  String sTemp = "";
  //원본 복사
  String sCopy = sData;
  while(true){
    //구분자 찾기
    nGetIndex = sCopy.indexOf(cSeparator);
    //리턴된 인덱스가 있나?
    if(-1 != nGetIndex){
      //데이터 넣고
      sTemp = sCopy.substring(0, nGetIndex);
      resultList[indexCount] = sTemp;
      indexCount++;
      //뺀 데이터 만큼 잘라낸다.
      sCopy = sCopy.substring(nGetIndex + 1);
    }
    else{
      //없으면 마무리 한다.
      resultList[indexCount] = sCopy;
      break;
    }
    //다음 문자로~
    ++nCount;
  }
}

String charToString(char* chr){
  return String(chr);
}

String makeStringSocket(String id, String order, String value){
  String strResult = id + Spliter + order + Spliter + value + "\r";
  return strResult;
}

bool sendWarnning(float T, String state){
  // state : /warn or /emergency or /emergencyIng
//  if(state != "/warn" || state != "/emergency" || state != "/emergencyIng"){
//    state = "/warn";
//  }

  // message EX : "N1!/warn!32.22\r"
  String message = makeStringSocket(clientId, state, String(T, 2));
  char charList[message.length()+1];
  message.toCharArray(charList, message.length());
  Serial.println(charList);
  client.write(charList);

  String recevbline = client.readStringUntil('\n');
  Serial.print("receved msg : ");
  Serial.println(recevbline);

  // message Split
  int maxIndex = countChar(recevbline, Spliter);
  String recevList[maxIndex+1];
  stringSplit(recevbline, Spliter, recevList);
  
  if(recevList[1] == "/received"){
    return true;
  }
  else{
    return false;
  }
}
