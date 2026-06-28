/*
 * Pred_Ai — Guardian Surveillance Detector (ESP32-S3)
 * Passive camera/tracker detector. Keeps all original modules:
 *   watchdog, SD logging, GPS, XOR command crypto, command history, scramble,
 *   and the original serial command words.
 * Adds: WiFi+BLE scanning, vendor/camera + AirTag/tracker tagging, GPS+time
 *   stamping, CSV logging, "following you" detection, LED/optional TFT alerts.
 *
 * NOTE: performActiveSpamCycle() (the active WiFi/BLE transmit flood) is
 * intentionally NOT included. Everything else from the original is here.
 */

#include <WiFi.h>
#include "esp_wifi.h"
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <esp_random.h>
#include <esp_task_wdt.h>
#include <SD.h>
#include <SPI.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <math.h>

// ---- Optional 2.0" TFT (set to 1 after installing Adafruit GFX + ILI9341
//      libraries and setting the pins below to your LAFVIN shield) ----
#define ENABLE_TFT 0
#if ENABLE_TFT
  #include <Adafruit_GFX.h>
  #include <Adafruit_ILI9341.h>   // swap for Adafruit_ST7789 if screen is blank
  #define TFT_CS  10
  #define TFT_DC   9
  #define TFT_RST  8
  Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
#endif

#define SD_CS_PIN 5
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17
#define ALERT_LED_PIN 2
#define ENABLE_BUZZER 0
#define BUZZER_PIN 4

#define LOG_PATH   "/sightings.csv"
#define DB_PATH    "/devices.dat"
#define DB_MAGIC   0x50414933UL
#define BLE_SCAN_SECS 4
#define SCAN_PERIOD 15000UL
#define MAX_DEVICES 150
#define MOVE_METERS 75.0
#define APPROACH_DB 12
#define APPROACH_MIN_HITS 3

TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

// ---- original globals (kept) ----
bool activeMode = false;          // "shield/activate" word (no transmit in this build)
bool scanningEnabled = true;      // detection on/off
bool alertsEnabled = true;
unsigned long lastActionTime = 0;
unsigned long lastLogTime = 0;
unsigned long lastScan = 0;
int currentChannel = 1;
String currentPattern = "Crowd";

double realLat = 0.0, realLon = 0.0;
double reportedLat = 0.0, reportedLon = 0.0;

String commandHistory[10];
int historyIndex = 0;
const uint8_t KEY = 0xA5;          // your XOR key (kept)

File logFile;
bool sdReady = false;
bool gpsFix = false;
double curLat = 0.0, curLon = 0.0;
BLEScan* pBLEScan = nullptr;

// ---------------- Vendor OUI table (camera/IoT/tracker) ----------------
struct OuiEntry { uint8_t oui[3]; const char* name; uint8_t risk; };
const OuiEntry OUI_TABLE[] = {
  {{0x00,0x40,0x8C},"Axis (camera)",2}, {{0xAC,0xCC,0x8E},"Axis (camera)",2},
  {{0x00,0x12,0x12},"Hikvision",2},     {{0x44,0x19,0xB6},"Hikvision",2},
  {{0xBC,0xAD,0x28},"Hikvision",2},     {{0x00,0x18,0xAE},"Dahua",2},
  {{0x3C,0xEF,0x8C},"Dahua",2},         {{0x2C,0xAA,0x8E},"Wyze",2},
  {{0x7C,0x78,0xB2},"Wyze",2},          {{0x18,0xB4,0x30},"Nest/Google",2},
  {{0x64,0x16,0x66},"Nest/Google",2},   {{0x00,0x7E,0x56},"Ring/Amazon",2},
  {{0x34,0xD2,0x70},"Ring/Amazon",2},   {{0x10,0x5A,0x17},"Tuya cam/IoT",2},
  {{0x50,0x02,0x91},"Tuya cam/IoT",2},  {{0x68,0x57,0x2D},"Tuya cam/IoT",2},
  {{0xD4,0xA6,0x51},"Reolink",2},       {{0xEC,0x71,0xDB},"Reolink",2},
};
const int OUI_COUNT = sizeof(OUI_TABLE)/sizeof(OUI_TABLE[0]);

struct Sighting {
  uint8_t mac[6]; bool isBle; int8_t firstRssi; int8_t bestRssi; uint16_t hits;
  double firstLat, firstLon, lastLat, lastLon;
  bool mobileFlag; bool approachFlag; bool vendorFlag; bool alerted; char label[24];
};
Sighting devices[MAX_DEVICES];
int deviceCount = 0;

// ---------------- helpers ----------------
double haversineMeters(double a1,double o1,double a2,double o2){
  const double R=6371000.0; double dA=radians(a2-a1), dO=radians(o2-o1);
  double a=sin(dA/2)*sin(dA/2)+cos(radians(a1))*cos(radians(a2))*sin(dO/2)*sin(dO/2);
  return R*2*atan2(sqrt(a),sqrt(1-a));
}
String macToStr(const uint8_t* m){char b[18];sprintf(b,"%02X:%02X:%02X:%02X:%02X:%02X",m[0],m[1],m[2],m[3],m[4],m[5]);return String(b);}
String getTimestamp(){
  if(gps.date.isValid()&&gps.time.isValid()){char b[32];
    sprintf(b,"%04d-%02d-%02dT%02d:%02d:%02dZ",gps.date.year(),gps.date.month(),gps.date.day(),gps.time.hour(),gps.time.minute(),gps.time.second());
    return String(b);}
  return String(millis()/1000)+"s";
}
bool isRandomMac(const uint8_t* m){return (m[0]&0x02)!=0;}
int lookupOui(const uint8_t* m){
  if(isRandomMac(m))return -1;
  for(int i=0;i<OUI_COUNT;i++)
    if(m[0]==OUI_TABLE[i].oui[0]&&m[1]==OUI_TABLE[i].oui[1]&&m[2]==OUI_TABLE[i].oui[2])return i;
  return -1;
}

// ---------------- your XOR crypto + history (kept) ----------------
String encryptCommand(String cmd){String e=cmd;for(unsigned i=0;i<e.length();i++)e[i]=e[i]^KEY;return e;}
String decryptCommand(String e){String d=e;for(unsigned i=0;i<d.length();i++)d[i]=d[i]^KEY;return d;}
bool isRepeatCommand(String c){for(int i=0;i<10;i++)if(commandHistory[i]==c)return true;return false;}
void addToHistory(String c){commandHistory[historyIndex]=c;historyIndex=(historyIndex+1)%10;}

// ---------------- your scramble + status log (kept) ----------------
void scrambleGPS(){
  if(gps.location.isValid()){double n=(random(-150,150)/100000.0)*1.5;reportedLat=realLat+n;reportedLon=realLon+n;}
}
void logStatusToSD(){
  scrambleGPS();
  if(!sdReady)return;
  logFile=SD.open("/guardian.log",FILE_APPEND);
  if(logFile){
    String ts=getTimestamp();
    logFile.printf("[%s] Scan:%s Alerts:%s Ch:%d Pattern:%s Real:%.6f,%.6f Reported:%.6f,%.6f\n",
      ts.c_str(),scanningEnabled?"YES":"NO",alertsEnabled?"YES":"NO",currentChannel,currentPattern.c_str(),
      realLat,realLon,reportedLat,reportedLon);
    logFile.close();
  }
}

// ---------------- persistence ----------------
void saveTable(){
  if(!sdReady)return;
  File f=SD.open(DB_PATH,FILE_WRITE); if(!f)return;
  uint32_t magic=DB_MAGIC; f.write((const uint8_t*)&magic,sizeof(magic));
  f.write((const uint8_t*)&deviceCount,sizeof(deviceCount));
  if(deviceCount>0) f.write((const uint8_t*)devices,sizeof(Sighting)*deviceCount);
  f.close();
}
void loadTable(){
  if(!sdReady||!SD.exists(DB_PATH))return;
  File f=SD.open(DB_PATH,FILE_READ); if(!f)return;
  uint32_t magic=0; int count=0;
  if(f.read((uint8_t*)&magic,sizeof(magic))==sizeof(magic)&&magic==DB_MAGIC&&
     f.read((uint8_t*)&count,sizeof(count))==sizeof(count)&&count>=0&&count<=MAX_DEVICES){
    int want=sizeof(Sighting)*count;
    if(f.read((uint8_t*)devices,want)==want)deviceCount=count;
  }
  f.close();
  Serial.printf("Loaded %d remembered devices.\n",deviceCount);
}

// ---------------- TFT ----------------
#if ENABLE_TFT
bool isFollowing(const Sighting* s){return s->mobileFlag||s->approachFlag;}
void tftBanner(const char* t,uint16_t c){tft.fillRect(0,0,tft.width(),22,c);tft.setTextColor(ILI9341_BLACK,c);tft.setTextSize(2);tft.setCursor(4,4);tft.print(t);}
void drawScreen(){
  int fl=0; for(int i=0;i<deviceCount;i++) if(devices[i].vendorFlag||isFollowing(&devices[i]))fl++;
  tft.fillScreen(ILI9341_BLACK);
  char h[40]; snprintf(h,sizeof(h),"Pred_Ai  %d flagged",fl);
  tftBanner(h, fl?ILI9341_RED:ILI9341_DARKGREEN);
  tft.setTextSize(1); int y=28;
  for(int i=0;i<deviceCount&&y<tft.height()-12;i++){
    Sighting* s=&devices[i]; bool f=isFollowing(s);
    if(!(s->vendorFlag||f))continue;
    tft.setTextColor(f?ILI9341_RED:ILI9341_YELLOW,ILI9341_BLACK);
    tft.setCursor(2,y); tft.printf("%-6s %-16s %ddB",f?"FOLLOW":"VENDOR",s->label,s->bestRssi); y+=12;
  }
  tft.setTextColor(ILI9341_WHITE,ILI9341_BLACK); tft.setCursor(2,tft.height()-10);
  tft.printf("seen:%d GPS:%s SD:%s",deviceCount,gpsFix?"Y":"N",sdReady?"Y":"N");
}
void tftAlertFlash(){tft.fillScreen(ILI9341_RED);delay(150);}
#endif

// ---------------- alerts ----------------
void localAlert(){
  if(!alertsEnabled)return;
  for(int i=0;i<6;i++){digitalWrite(ALERT_LED_PIN,HIGH);
#if ENABLE_BUZZER
    tone(BUZZER_PIN,2200,80);
#endif
    delay(90);digitalWrite(ALERT_LED_PIN,LOW);delay(90);}
}
void raiseAlert(const Sighting* s,const char* reason){
  String msg=String("ALERT: ")+reason+" "+(s->isBle?"BLE ":"WIFI ")+macToStr(s->mac)+
    " ["+s->label+"] rssi="+s->bestRssi+" @ "+String(curLat,6)+","+String(curLon,6)+" "+getTimestamp();
  Serial.print("*** ");Serial.println(msg);
#if ENABLE_TFT
  if(alertsEnabled)tftAlertFlash();
#endif
  localAlert();
}

// ---------------- record + log ----------------
void logSighting(Sighting* s,int rssi){
  String flag="";
  if(s->vendorFlag)flag+="VENDOR ";
  if(s->mobileFlag)flag+="FOLLOWING ";
  if(s->approachFlag)flag+="APPROACHING ";
  if(flag=="")flag="-";
  String line=getTimestamp()+","+String(curLat,6)+","+String(curLon,6)+","+
    (s->isBle?"BLE":"WIFI")+","+macToStr(s->mac)+","+String(s->label)+","+
    String(rssi)+","+String(s->hits)+","+flag;
  Serial.println(line);
  if(sdReady){File f=SD.open(LOG_PATH,FILE_APPEND);if(f){f.println(line);f.close();}}
}
void recordDevice(const uint8_t* mac,bool isBle,int rssi,const char* label,bool vendorHit){
  Sighting* s=nullptr;
  for(int i=0;i<deviceCount;i++) if(devices[i].isBle==isBle&&memcmp(devices[i].mac,mac,6)==0){s=&devices[i];break;}
  if(!s){
    if(deviceCount>=MAX_DEVICES)return;
    s=&devices[deviceCount++]; memcpy(s->mac,mac,6); s->isBle=isBle;
    s->firstRssi=rssi; s->bestRssi=rssi; s->hits=0; s->firstLat=curLat; s->firstLon=curLon;
    s->mobileFlag=false; s->approachFlag=false; s->vendorFlag=vendorHit; s->alerted=false;
    strncpy(s->label,label,sizeof(s->label)-1); s->label[sizeof(s->label)-1]='\0';
  }
  s->hits++; if(rssi>s->bestRssi)s->bestRssi=rssi; s->lastLat=curLat; s->lastLon=curLon;
  if(vendorHit)s->vendorFlag=true;
  if(gpsFix&&(s->firstLat!=0.0||s->firstLon!=0.0)){
    if(haversineMeters(s->firstLat,s->firstLon,curLat,curLon)>MOVE_METERS)s->mobileFlag=true;
  }
  if(s->hits>=APPROACH_MIN_HITS&&(rssi-s->firstRssi)>=APPROACH_DB)s->approachFlag=true;
  logSighting(s,rssi);
  bool follow=s->mobileFlag||s->approachFlag;
  if((s->vendorFlag||follow)&&!s->alerted){raiseAlert(s,follow?"FOLLOWING device":"known vendor device");s->alerted=true;}
}

// ---------------- scans ----------------
void scanWifi(){
  int n=WiFi.scanNetworks(false,true);
  for(int i=0;i<n;i++){
    uint8_t* b=WiFi.BSSID(i); if(!b)continue;
    int oui=lookupOui(b); bool v=(oui>=0&&OUI_TABLE[oui].risk>=1);
    String ssid=WiFi.SSID(i); char label[24];
    if(oui>=0){strncpy(label,OUI_TABLE[oui].name,sizeof(label)-1);label[sizeof(label)-1]='\0';}
    else if(ssid.length()>0)snprintf(label,sizeof(label),"wifi:%s",ssid.c_str());
    else {strncpy(label,"wifi:<hidden>",sizeof(label)-1);label[sizeof(label)-1]='\0';}
    recordDevice(b,false,WiFi.RSSI(i),label,v);
  }
  WiFi.scanDelete();
}
void performPassiveScan(){   // original function name kept; now does real detection
  Serial.println("[PASSIVE] Scanning...");
  BLEScanResults* r=pBLEScan->start(BLE_SCAN_SECS,false);
  int n=r->getCount();
  for(int i=0;i<n;i++){
    BLEAdvertisedDevice d=r->getDevice(i);
    const uint8_t* native=*d.getAddress().getNative();
    char label[24]="ble"; bool v=false;
    if(d.haveManufacturerData()){
      String md=d.getManufacturerData();
      if(md.length()>=2){
        uint16_t co=(uint8_t)md[0]|((uint8_t)md[1]<<8);
        if(co==0x004C){
          if(md.length()>=4&&(uint8_t)md[2]==0x12){strncpy(label,"AirTag/FindMy?",sizeof(label)-1);v=true;}
          else strncpy(label,"Apple BLE",sizeof(label)-1);
        } else if(co==0x0075)strncpy(label,"Samsung BLE",sizeof(label)-1);
        label[sizeof(label)-1]='\0';
      }
    }
    if(d.haveServiceUUID()){
      String u=d.getServiceUUID().toString().c_str();
      if(u.indexOf("feed")>=0||u.indexOf("feec")>=0){strncpy(label,"Tile tracker",sizeof(label)-1);label[sizeof(label)-1]='\0';v=true;}
      else if(u.indexOf("fd5a")>=0){strncpy(label,"Samsung SmartTag",sizeof(label)-1);label[sizeof(label)-1]='\0';v=true;}
    }
    if(!v&&d.haveName()&&d.getName().length()>0)snprintf(label,sizeof(label),"ble:%s",d.getName().c_str());
    recordDevice(native,true,d.getRSSI(),label,v);
  }
  pBLEScan->clearResults();
}

// ---------------- review commands ----------------
void printDevice(const Sighting* s){
  bool f=s->vendorFlag||s->mobileFlag||s->approachFlag;
  Serial.printf("  %s %s rssi=%d hits=%u %s%s%s%s [%s]\n",s->isBle?"BLE ":"WIFI",macToStr(s->mac).c_str(),
    s->bestRssi,s->hits,s->vendorFlag?"VENDOR ":"",s->mobileFlag?"FOLLOWING ":"",s->approachFlag?"APPROACHING ":"",f?"":"-",s->label);
}
bool handleReview(String c){
  c.trim(); c.toLowerCase();
  if(c=="help"||c=="?"){Serial.println("list | flagged | gps | save | clear | testalert | (your encrypted cmds also work)");return true;}
  if(c=="list"){Serial.printf("== %d devices ==\n",deviceCount);for(int i=0;i<deviceCount;i++)printDevice(&devices[i]);return true;}
  if(c=="flagged"){int n=0;for(int i=0;i<deviceCount;i++)if(devices[i].vendorFlag||devices[i].mobileFlag||devices[i].approachFlag){printDevice(&devices[i]);n++;}Serial.printf("(%d flagged)\n",n);return true;}
  if(c=="gps"){Serial.printf("GPS fix=%s lat=%.6f lon=%.6f sats=%d\n",gpsFix?"yes":"no",curLat,curLon,gps.satellites.isValid()?gps.satellites.value():0);return true;}
  if(c=="save"){saveTable();Serial.println("Saved.");return true;}
  if(c=="clear"){deviceCount=0;saveTable();Serial.println("Cleared.");return true;}
  if(c=="testalert"){Serial.println("Test alert...");bool a=alertsEnabled;alertsEnabled=true;localAlert();alertsEnabled=a;return true;}
  return false;
}

// ---------------- your command parser (kept, XOR + history) ----------------
void executeCommand(String rawCmd){
  String cmd=decryptCommand(rawCmd);
  cmd.trim(); String original=cmd; cmd.toLowerCase();
  if(isRepeatCommand(cmd)){Serial.println("Repeat command ignored for safety");return;}
  addToHistory(cmd);
  Serial.printf("Command: %s\n",original.c_str());
  if(cmd.indexOf("spam on")!=-1||cmd.indexOf("shield")!=-1||cmd.indexOf("crowd")!=-1||cmd.indexOf("activate")!=-1){
    activeMode=true; scanningEnabled=true; alertsEnabled=true;
    Serial.println("SHIELD/ACTIVE: detection+alerts ON (active transmit omitted in this build)");
  } else if(cmd.indexOf("scan on")!=-1||cmd.indexOf("passive")!=-1||cmd.indexOf("listen")!=-1){
    scanningEnabled=true; Serial.println("PASSIVE DETECTION ON");
  } else if(cmd.indexOf("off")!=-1||cmd.indexOf("stop")!=-1){
    scanningEnabled=false; activeMode=false; Serial.println("DETECTION PAUSED");
  } else if(cmd.indexOf("status")!=-1){
    logStatusToSD(); Serial.println("Status logged");
  } else {
    Serial.println("Unknown - general order");
  }
}

void serialEvent(){
  String line=Serial.readStringUntil('\n');
  String probe=line; probe.trim();
  if(handleReview(probe))return;     // plaintext review commands
  executeCommand(line);              // else your XOR-encrypted command path
}

// ---------------- setup / loop ----------------
void setup(){
  Serial.begin(115200); delay(200);
  randomSeed(esp_random());
  esp_task_wdt_init(20,true); esp_task_wdt_add(NULL);

  pinMode(ALERT_LED_PIN,OUTPUT); digitalWrite(ALERT_LED_PIN,LOW);

#if ENABLE_TFT
  tft.begin(); tft.setRotation(1); tft.fillScreen(ILI9341_BLACK);
  tftBanner("Pred_Ai booting...",ILI9341_DARKGREEN);
#endif

  sdReady=SD.begin(SD_CS_PIN);
  if(sdReady){
    Serial.println("SD OK");
    logFile=SD.open("/guardian.log",FILE_APPEND);
    if(logFile){logFile.println("=== Log Started ===");logFile.close();}
    File f=SD.open(LOG_PATH,FILE_APPEND);
    if(f){if(f.size()==0)f.println("timestamp,lat,lon,type,mac,label,rssi,hits,flags");f.close();}
    loadTable();
  } else Serial.println("SD: not found (serial only)");

  gpsSerial.begin(9600,SERIAL_8N1,GPS_RX_PIN,GPS_TX_PIN);

  WiFi.mode(WIFI_STA); WiFi.disconnect();   // station = scan/listen only
  BLEDevice::init("");
  pBLEScan=BLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100); pBLEScan->setWindow(99);

  Serial.println("Guardian Secure Unit Ready (detector). Type 'help'.");
}

void loop(){
  esp_task_wdt_reset();
  while(gpsSerial.available()>0)gps.encode(gpsSerial.read());
  if(gps.location.isUpdated()&&gps.location.isValid()){
    realLat=gps.location.lat(); realLon=gps.location.lng();
    curLat=realLat; curLon=realLon; gpsFix=true;
  }
  unsigned long now=millis();
  if(scanningEnabled&&now-lastScan>SCAN_PERIOD){
    lastScan=now;
    scanWifi(); performPassiveScan(); saveTable();
#if ENABLE_TFT
    drawScreen();
#endif
  }
  if(now-lastLogTime>8000){logStatusToSD();lastLogTime=now;}
  delay(5);
}
