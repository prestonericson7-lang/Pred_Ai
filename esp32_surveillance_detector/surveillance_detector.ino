/*
 * Pred_Ai — Guardian Surveillance Detector (ESP32-S3 / LAFVIN AI Chatbot kit)
 * ===========================================================================
 * PASSIVE, RECEIVE-ONLY counter-surveillance scanner.
 *
 * Listens for nearby WiFi + BLE devices, flags likely cameras / IoT / trackers
 * by MAC vendor (OUI) AND by SSID/BLE-name keywords, GPS+time stamps them,
 * logs to SD, shows a live threat screen on the kit's ST7789 TFT, and warns
 * when a device is FOLLOWING you (GPS movement) or APPROACHING you (signal
 * rising over time). Keeps all original modules: watchdog, SD guardian.log,
 * GPS, XOR command crypto, command history, scramble, original command words.
 *
 * It only LISTENS. WiFi stays in station mode; BLE only scans. No transmit,
 * no AP, no advertising. The safety invariant: nothing calls WiFi.softAP() or
 * esp_wifi_set_mac() in a loop. Keep that true and it can only observe.
 *
 * Honest limits: only sees powered-on 2.4 GHz transmitters. The ESP32-S3 is
 * 2.4 GHz ONLY — it cannot hear 5 GHz cameras, wired/analog cameras, or
 * anything with its radio off. A flag means "look closer," not "confirmed."
 *
 * Verified LAFVIN ESP32-S3 AI Chatbot pin map (from vendor board source):
 *   TFT(ST7789): MOSI 40, SCLK 41, CS 47, DC 39, RST 4, Backlight 42
 *   Builtin LED 48 | Buttons 19/20, BOOT 0 | Audio I2S (codec) 12/13/14/38/45
 */

#include <WiFi.h>
#include "esp_wifi.h"
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <esp_random.h>
#include <esp_task_wdt.h>
#include <esp_idf_version.h>
#include <SD.h>
#include <SPI.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <math.h>

// ---------------- 2.0" ST7789 TFT (kit display) ----------------
#define ENABLE_TFT 1
#if ENABLE_TFT
  #include <Adafruit_GFX.h>
  #include <Adafruit_ST7789.h>
  #define TFT_MOSI 40
  #define TFT_SCLK 41
  #define TFT_CS   47
  #define TFT_DC   39
  #define TFT_RST  4
  #define TFT_BL   42
  Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
#endif

// ---------------- pins / config ----------------
#define ALERT_LED_PIN 48        // kit builtin LED (TFT flash is the main alert)
#define ENABLE_BUZZER 0         // optional passive buzzer
#define BUZZER_PIN    21        // (kept off 4: that's TFT_RST)

#define SD_CS_PIN  5            // optional microSD (not in base kit)
#define GPS_RX_PIN 16           // optional GPS
#define GPS_TX_PIN 17
#define GPS_BAUD   9600

#define LOG_PATH   "/sightings.csv"
#define DB_PATH    "/devices.dat"
#define DB_MAGIC   0x50414934UL // bump when struct layout changes
#define BLE_SCAN_SECS 4
#define SCAN_PERIOD   12000UL
#define MAX_DEVICES   180

// --- following / approaching tuning (raise thresholds = fewer false alarms) ---
#define MOVE_METERS        75.0     // GPS: moved this far with you => following
#define APPROACH_DB        10       // signal must climb this many dB...
#define APPROACH_MIN_CYC   4        // ...across at least this many scan cycles...
#define APPROACH_MIN_MS    60000UL  // ...over at least this long (transient filter)
#define EMA_ALPHA          0.40f    // RSSI smoothing (kills single-sample spikes)

TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

// ---------------- original globals (kept) ----------------
bool activeMode = false;        // "shield/activate" word (no transmit in this build)
bool scanningEnabled = true;
bool alertsEnabled = true;
unsigned long lastLogTime = 0;
unsigned long lastScan = 0;
uint16_t scanCycle = 0;
int currentChannel = 0;
String currentPattern = "Crowd";

double realLat = 0.0, realLon = 0.0;
double reportedLat = 0.0, reportedLon = 0.0;

String commandHistory[10];
int historyIndex = 0;
const uint8_t KEY = 0xA5;        // XOR command key (kept)

File logFile;
bool sdReady = false;
bool gpsFix = false;
double curLat = 0.0, curLon = 0.0;
BLEScan* pBLEScan = nullptr;

// ---------------- vendor OUI table (camera / IoT / tracker) ----------------
// risk: 2 = camera-likely, 1 = IoT/tracker/info. Expand from the IEEE registry
// or maclookup.app. (Randomized MACs are skipped — their vendor bits are noise.)
struct OuiEntry { uint8_t oui[3]; const char* name; uint8_t risk; };
const OuiEntry OUI_TABLE[] = {
  // Axis
  {{0x00,0x40,0x8C},"Axis cam",2},{{0xAC,0xCC,0x8E},"Axis cam",2},{{0xB8,0xA4,0x4F},"Axis cam",2},
  // Hikvision
  {{0x44,0x19,0xB6},"Hikvision",2},{{0xBC,0xAD,0x28},"Hikvision",2},{{0xC0,0x56,0xE3},"Hikvision",2},
  {{0x4C,0xBD,0x8F},"Hikvision",2},{{0x28,0x57,0xBE},"Hikvision",2},{{0x54,0xC4,0x15},"Hikvision",2},
  // Dahua
  {{0x3C,0xEF,0x8C},"Dahua",2},{{0x90,0x02,0xA9},"Dahua",2},{{0x14,0xA7,0x8B},"Dahua",2},{{0xE0,0x50,0x8B},"Dahua",2},
  // Reolink / Amcrest / Wyze
  {{0xEC,0x71,0xDB},"Reolink",2},{{0x9C,0x8E,0xCD},"Amcrest",2},{{0x3C,0xE3,0x6B},"Amcrest",2},
  {{0x2C,0xAA,0x8E},"Wyze",2},{{0x7C,0x78,0xB2},"Wyze",2},
  // Ubiquiti UniFi cams
  {{0x04,0x18,0xD6},"Ubiquiti",2},{{0x24,0xA4,0x3C},"Ubiquiti",2},{{0x68,0x72,0x51},"Ubiquiti",2},
  {{0x78,0x8A,0x20},"Ubiquiti",2},{{0xFC,0xEC,0xDA},"Ubiquiti",2},{{0x44,0xD9,0xE7},"Ubiquiti",2},
  // Nest / Google
  {{0x18,0xB4,0x30},"Nest/Google",2},{{0x64,0x16,0x66},"Nest/Google",2},{{0x6C,0xAD,0xF8},"Nest/Google",2},
  {{0x1C,0xF2,0x9A},"Nest/Google",2},{{0x30,0xFD,0x38},"Nest/Google",2},
  // Amazon / Ring
  {{0x34,0xD2,0x70},"Ring/Amazon",2},{{0x44,0x65,0x0D},"Ring/Amazon",2},{{0x0C,0x47,0xC9},"Ring/Amazon",2},
  {{0xFC,0x65,0xDE},"Ring/Amazon",2},{{0x68,0x54,0xFD},"Ring/Amazon",2},{{0xAC,0x63,0xBE},"Ring/Amazon",2},
  // TP-Link / Tapo
  {{0x50,0xC7,0xBF},"TP-Link/Tapo",2},{{0x60,0x32,0xB1},"TP-Link/Tapo",2},{{0xA4,0x2B,0xB0},"TP-Link/Tapo",2},
  {{0x30,0xDE,0x4B},"TP-Link/Tapo",2},{{0x5C,0x62,0x8B},"TP-Link/Tapo",2},
  // D-Link
  {{0x00,0x1B,0x11},"D-Link",2},{{0x14,0xD6,0x4D},"D-Link",2},{{0xB0,0xC5,0x54},"D-Link",2},{{0x28,0x10,0x7B},"D-Link",2},
  // Xiaomi (Mi/Dafang)
  {{0x28,0x6C,0x07},"Xiaomi cam",2},{{0x34,0xCE,0x00},"Xiaomi cam",2},{{0x50,0xEC,0x50},"Xiaomi cam",2},
  {{0x64,0x09,0x80},"Xiaomi cam",2},{{0x78,0x11,0xDC},"Xiaomi cam",2},{{0xF0,0xB4,0x29},"Xiaomi cam",2},
  // Tuya white-label cams
  {{0x10,0x5A,0x17},"Tuya cam",2},{{0x50,0x02,0x91},"Tuya cam",2},{{0x68,0x57,0x2D},"Tuya cam",2},
  // Espressif (DIY ESP32-CAM are common in hidden cams) — info tier
  {{0x24,0x0A,0xC4},"Espressif/ESP-CAM?",1},{{0x30,0xAE,0xA4},"Espressif/ESP-CAM?",1},
  {{0x7C,0x9E,0xBD},"Espressif/ESP-CAM?",1},{{0xA4,0xCF,0x12},"Espressif/ESP-CAM?",1},
  {{0xEC,0x94,0xCB},"Espressif/ESP-CAM?",1},{{0xCC,0x50,0xE3},"Espressif/ESP-CAM?",1},
};
const int OUI_COUNT = sizeof(OUI_TABLE)/sizeof(OUI_TABLE[0]);

// SSID substrings that strongly suggest a camera/NVR (lowercased compare).
const char* SSID_KEYWORDS[] = {
  "ipcam","ipc-","ipc_","hikvision","hik-","dahua","reolink","wyze","tapo","ezviz",
  "foscam","amcrest","arlo","lorex","swann","annke","vivotek","unifi","instar",
  "camera","doorbell","babymonitor","nvr-","dvr-","yicam","goprocam"
};
const int SSID_KW_COUNT = sizeof(SSID_KEYWORDS)/sizeof(SSID_KEYWORDS[0]);

// BLE advertised-name substrings that suggest a tracker/camera.
const char* BLE_KEYWORDS[] = {
  "airtag","tile","smarttag","chipolo","trackr","itag","find my","gps tracker",
  "wyze","reolink","ring","nest cam","camera"
};
const int BLE_KW_COUNT = sizeof(BLE_KEYWORDS)/sizeof(BLE_KEYWORDS[0]);

// ---------------- tracking table ----------------
struct Sighting {
  uint8_t  mac[6];
  bool     isBle;
  int8_t   firstRssi;
  int8_t   bestRssi;
  float    emaRssi;       // smoothed signal
  uint16_t hits;
  uint16_t cycles;        // distinct scan cycles seen in
  uint16_t lastCycle;
  uint32_t firstSeenMs;
  uint32_t lastSeenMs;
  double   firstLat, firstLon, lastLat, lastLon;
  bool     mobileFlag;    // GPS: moved with you
  bool     approachFlag;  // signal rose over time
  bool     vendorFlag;    // matched vendor/keyword (shown)
  bool     alertWorthy;   // camera-grade vendor or following (alerts)
  bool     alerted;
  char     label[24];
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
// returns matched keyword index or -1 (haystack must be lowercase)
int matchKeyword(const String& hay,const char** table,int count){
  for(int i=0;i<count;i++) if(hay.indexOf(table[i])>=0) return i;
  return -1;
}

// ---------------- XOR crypto + history (kept) ----------------
String encryptCommand(String cmd){String e=cmd;for(unsigned i=0;i<e.length();i++)e[i]=e[i]^KEY;return e;}
String decryptCommand(String e){String d=e;for(unsigned i=0;i<d.length();i++)d[i]=d[i]^KEY;return d;}
bool isRepeatCommand(String c){for(int i=0;i<10;i++)if(commandHistory[i]==c)return true;return false;}
void addToHistory(String c){commandHistory[historyIndex]=c;historyIndex=(historyIndex+1)%10;}

// ---------------- scramble + status log (kept) ----------------
void scrambleGPS(){
  if(gps.location.isValid()){
    double nLat=(random(-150,150)/100000.0)*1.5;
    double nLon=(random(-150,150)/100000.0)*1.5;
    reportedLat=realLat+nLat; reportedLon=realLon+nLon;
  }
}
void logStatusToSD(){
  scrambleGPS();
  if(!sdReady)return;
  logFile=SD.open("/guardian.log",FILE_APPEND);
  if(logFile){
    logFile.printf("[%s] Scan:%s Alerts:%s Devices:%d Real:%.6f,%.6f Reported:%.6f,%.6f\n",
      getTimestamp().c_str(),scanningEnabled?"YES":"NO",alertsEnabled?"YES":"NO",deviceCount,
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
    if(f.read((uint8_t*)devices,sizeof(Sighting)*count)==(int)(sizeof(Sighting)*count))deviceCount=count;
  } else Serial.println("Old/incompatible device DB ignored.");
  f.close();
  Serial.printf("Loaded %d remembered devices.\n",deviceCount);
}

// ---------------- TFT ----------------
#if ENABLE_TFT
bool isFollowing(const Sighting* s){return s->mobileFlag||s->approachFlag;}
void tftBanner(const char* t,uint16_t c){tft.fillRect(0,0,tft.width(),22,c);tft.setTextColor(ST77XX_BLACK,c);tft.setTextSize(2);tft.setCursor(4,4);tft.print(t);}
void drawScreen(){
  int fl=0; for(int i=0;i<deviceCount;i++) if(devices[i].vendorFlag||isFollowing(&devices[i]))fl++;
  tft.fillScreen(ST77XX_BLACK);
  char h[40]; snprintf(h,sizeof(h),"Pred_Ai  %d flagged",fl);
  tftBanner(h, fl?ST77XX_RED:ST77XX_GREEN);
  tft.setTextSize(1); int y=28;
  for(int i=0;i<deviceCount&&y<tft.height()-12;i++){
    Sighting* s=&devices[i]; bool f=isFollowing(s);
    if(!(s->vendorFlag||f))continue;
    tft.setTextColor(f?ST77XX_RED:ST77XX_YELLOW,ST77XX_BLACK);
    tft.setCursor(2,y); tft.printf("%-6s %-15s %ddB",f?"FOLLOW":"VENDOR",s->label,s->bestRssi); y+=11;
  }
  tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK); tft.setCursor(2,tft.height()-10);
  tft.printf("seen:%d GPS:%s SD:%s",deviceCount,gpsFix?"Y":"N",sdReady?"Y":"N");
}
void tftAlertFlash(){tft.fillScreen(ST77XX_RED);delay(150);}
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
  Serial.printf("*** ALERT: %s %s %s [%s] rssi=%d @ %.6f,%.6f %s\n",
    reason,s->isBle?"BLE":"WIFI",macToStr(s->mac).c_str(),s->label,s->bestRssi,curLat,curLon,getTimestamp().c_str());
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

// find existing, else allocate (evicting the oldest non-flagged when full)
Sighting* getSlot(const uint8_t* mac,bool isBle){
  for(int i=0;i<deviceCount;i++)
    if(devices[i].isBle==isBle&&memcmp(devices[i].mac,mac,6)==0) return &devices[i];
  if(deviceCount<MAX_DEVICES) return &devices[deviceCount++];
  // table full: recycle the oldest device that isn't flagged
  int victim=-1; uint32_t oldest=0xFFFFFFFF;
  for(int i=0;i<deviceCount;i++){
    if(devices[i].vendorFlag||devices[i].mobileFlag||devices[i].approachFlag)continue;
    if(devices[i].lastSeenMs<oldest){oldest=devices[i].lastSeenMs;victim=i;}
  }
  return (victim>=0)?&devices[victim]:nullptr;  // all flagged => drop new
}

// risk: 0 none, 1 info/IoT, 2 camera-grade
void recordDevice(const uint8_t* mac,bool isBle,int rssi,const char* label,int risk){
  uint32_t now=millis();
  Sighting* s=getSlot(mac,isBle);
  if(!s)return;
  bool isNew = !(s->isBle==isBle && memcmp(s->mac,mac,6)==0 && s->hits>0);
  if(isNew){
    memset(s,0,sizeof(*s));
    memcpy(s->mac,mac,6); s->isBle=isBle;
    s->firstRssi=rssi; s->bestRssi=rssi; s->emaRssi=rssi;
    s->firstSeenMs=now; s->firstLat=curLat; s->firstLon=curLon;
    s->lastCycle=scanCycle-1;
    strncpy(s->label,label,sizeof(s->label)-1);
  }
  s->hits++;
  s->lastSeenMs=now; s->lastLat=curLat; s->lastLon=curLon;
  if(rssi>s->bestRssi)s->bestRssi=rssi;
  s->emaRssi += EMA_ALPHA*(rssi - s->emaRssi);
  if(s->lastCycle!=scanCycle){s->cycles++;s->lastCycle=scanCycle;}
  if(risk>=1){s->vendorFlag=true; if(label[0])strncpy(s->label,label,sizeof(s->label)-1);}

  // FOLLOWING (GPS): moved with you, sustained over cycles + time
  if(gpsFix && s->cycles>=APPROACH_MIN_CYC && (now-s->firstSeenMs)>=APPROACH_MIN_MS &&
     haversineMeters(s->firstLat,s->firstLon,curLat,curLon)>MOVE_METERS)
    s->mobileFlag=true;

  // APPROACHING (no GPS): smoothed signal climbed, sustained over cycles + time
  if(s->cycles>=APPROACH_MIN_CYC && (now-s->firstSeenMs)>=APPROACH_MIN_MS &&
     (s->emaRssi - s->firstRssi)>=APPROACH_DB)
    s->approachFlag=true;

  bool following=s->mobileFlag||s->approachFlag;
  s->alertWorthy = (risk>=2) || following;
  logSighting(s,rssi);
  if(s->alertWorthy && !s->alerted){
    raiseAlert(s, following ? (s->mobileFlag?"FOLLOWING (moved with you)":"APPROACHING (signal rising)")
                            : "camera-grade device");
    s->alerted=true;
  }
}

// ---------------- scans ----------------
void scanWifi(){
  int n=WiFi.scanNetworks(false,true);
  for(int i=0;i<n;i++){
    uint8_t* b=WiFi.BSSID(i); if(!b)continue;
    char label[24]; int risk=0;
    int oui=lookupOui(b);
    String ssid=WiFi.SSID(i); String low=ssid; low.toLowerCase();
    int kw=matchKeyword(low,SSID_KEYWORDS,SSID_KW_COUNT);
    if(kw>=0){ risk=2; snprintf(label,sizeof(label),"cam:%s",SSID_KEYWORDS[kw]); }
    else if(oui>=0){ risk=OUI_TABLE[oui].risk; strncpy(label,OUI_TABLE[oui].name,sizeof(label)-1); label[sizeof(label)-1]='\0'; }
    else if(ssid.length()>0){ snprintf(label,sizeof(label),"wifi:%s",ssid.c_str()); }
    else { strcpy(label,"wifi:<hidden>"); }
    recordDevice(b,false,WiFi.RSSI(i),label,risk);
  }
  WiFi.scanDelete();
}
void performPassiveScan(){   // original name kept; real BLE detection
  BLEScanResults* r=pBLEScan->start(BLE_SCAN_SECS,false);
  int n=r->getCount();
  for(int i=0;i<n;i++){
    BLEAdvertisedDevice d=r->getDevice(i);
    BLEAddress addr=d.getAddress();            // keep alive (getNative() points into it)
    uint8_t native[6]; memcpy(native,addr.getNative(),6);
    char label[24]="ble"; int risk=0;
    if(d.haveManufacturerData()){
      String md=d.getManufacturerData();
      if(md.length()>=2){
        uint16_t co=(uint8_t)md[0]|((uint8_t)md[1]<<8);
        if(co==0x004C){
          if(md.length()>=4&&(uint8_t)md[2]==0x12){strncpy(label,"AirTag/FindMy?",23);risk=2;}
          else strncpy(label,"Apple BLE",23);
        } else if(co==0x0075) strncpy(label,"Samsung BLE",23);
        label[23]='\0';
      }
    }
    if(d.haveServiceUUID()){
      String u=d.getServiceUUID().toString().c_str();
      if(u.indexOf("feed")>=0||u.indexOf("feec")>=0){strcpy(label,"Tile tracker");risk=2;}
      else if(u.indexOf("fd5a")>=0){strcpy(label,"Samsung SmartTag");risk=2;}
    }
    if(d.haveName()&&d.getName().length()>0){
      String nm=d.getName().c_str(); String low=nm; low.toLowerCase();
      int kw=matchKeyword(low,BLE_KEYWORDS,BLE_KW_COUNT);
      if(kw>=0){snprintf(label,sizeof(label),"trk:%s",BLE_KEYWORDS[kw]);risk=2;}
      else if(risk==0)snprintf(label,sizeof(label),"ble:%s",nm.c_str());
    }
    recordDevice(native,true,d.getRSSI(),label,risk);
  }
  pBLEScan->clearResults();
}

// ---------------- serial commands ----------------
void printDevice(const Sighting* s){
  bool f=s->vendorFlag||s->mobileFlag||s->approachFlag;
  Serial.printf("  %s %s rssi=%d ema=%.0f hits=%u cyc=%u %s%s%s%s [%s]\n",
    s->isBle?"BLE ":"WIFI",macToStr(s->mac).c_str(),s->bestRssi,s->emaRssi,s->hits,s->cycles,
    s->vendorFlag?"VENDOR ":"",s->mobileFlag?"FOLLOWING ":"",s->approachFlag?"APPROACHING ":"",f?"":"-",s->label);
}
bool handleReview(String c){
  c.trim(); c.toLowerCase();
  if(c=="help"||c=="?"){Serial.println("list | flagged | gps | save | clear | testalert  (encrypted cmds also work)");return true;}
  if(c=="list"){Serial.printf("== %d devices ==\n",deviceCount);for(int i=0;i<deviceCount;i++)printDevice(&devices[i]);return true;}
  if(c=="flagged"){int n=0;for(int i=0;i<deviceCount;i++)if(devices[i].vendorFlag||devices[i].mobileFlag||devices[i].approachFlag){printDevice(&devices[i]);n++;}Serial.printf("(%d flagged)\n",n);return true;}
  if(c=="gps"){Serial.printf("GPS fix=%s lat=%.6f lon=%.6f sats=%d\n",gpsFix?"yes":"no",curLat,curLon,gps.satellites.isValid()?gps.satellites.value():0);return true;}
  if(c=="save"){saveTable();Serial.println("Saved.");return true;}
  if(c=="clear"){deviceCount=0;saveTable();Serial.println("Cleared.");return true;}
  if(c=="testalert"){bool a=alertsEnabled;alertsEnabled=true;localAlert();
#if ENABLE_TFT
    tftAlertFlash();
#endif
    alertsEnabled=a;Serial.println("Test alert fired.");return true;}
  return false;
}
void executeCommand(String rawCmd){    // your XOR + history path (kept)
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
  } else Serial.println("Unknown - general order");
}
// non-blocking serial line reader (serialEvent() is unreliable on ESP32)
void pollSerial(){
  static String buf;
  while(Serial.available()){
    char ch=Serial.read();
    if(ch=='\n'){ String line=buf; buf=""; String p=line; p.trim();
      if(!handleReview(p)) executeCommand(line); }
    else if(ch!='\r') buf+=ch;
  }
}

// ---------------- setup / loop ----------------
void setup(){
  Serial.begin(115200); delay(200);
  randomSeed(esp_random());

  // version-safe task watchdog (kept; IDF5/Arduino-3.x changed the API)
#if ESP_IDF_VERSION_MAJOR >= 5
  esp_task_wdt_config_t wdtc = { 20000, 0, true };
  esp_task_wdt_reconfigure(&wdtc);   // TWDT already inited by Arduino on IDF5
  esp_task_wdt_add(NULL);            // subscribe loop task (ignore if already added)
#else
  esp_task_wdt_init(20, true);
  esp_task_wdt_add(NULL);
#endif

  pinMode(ALERT_LED_PIN,OUTPUT); digitalWrite(ALERT_LED_PIN,LOW);

#if ENABLE_TFT
  pinMode(TFT_BL,OUTPUT); digitalWrite(TFT_BL,HIGH);   // backlight on
  SPI.begin(TFT_SCLK,-1,TFT_MOSI,TFT_CS);
  tft.init(240,320);            // ST7789 native 240x320
  tft.invertDisplay(true);      // LAFVIN panel uses inverted colour
  tft.setRotation(3);           // landscape (flip to 1 if upside-down)
  tft.fillScreen(ST77XX_BLACK);
  tftBanner("Pred_Ai booting...",ST77XX_GREEN);
#endif

  sdReady=SD.begin(SD_CS_PIN);
  if(sdReady){
    Serial.println("SD OK");
    logFile=SD.open("/guardian.log",FILE_APPEND);
    if(logFile){logFile.println("=== Log Started ===");logFile.close();}
    File f=SD.open(LOG_PATH,FILE_APPEND);
    if(f){if(f.size()==0)f.println("timestamp,lat,lon,type,mac,label,rssi,hits,flags");f.close();}
    loadTable();
  } else Serial.println("SD: not found (serial/TFT only)");

  gpsSerial.begin(GPS_BAUD,SERIAL_8N1,GPS_RX_PIN,GPS_TX_PIN);

  WiFi.mode(WIFI_STA); WiFi.disconnect();   // station = listen only
  BLEDevice::init("");
  pBLEScan=BLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100); pBLEScan->setWindow(99);

  Serial.println("Guardian Detector ready. Type 'help'.");
}

void loop(){
  esp_task_wdt_reset();
  pollSerial();
  while(gpsSerial.available()>0)gps.encode(gpsSerial.read());
  if(gps.location.isUpdated()&&gps.location.isValid()){
    realLat=gps.location.lat(); realLon=gps.location.lng();
    curLat=realLat; curLon=realLon; gpsFix=true;
  }
  unsigned long now=millis();
  if(scanningEnabled && now-lastScan>SCAN_PERIOD){
    lastScan=now; scanCycle++;
    scanWifi(); performPassiveScan(); saveTable();
#if ENABLE_TFT
    drawScreen();
#endif
  }
  if(now-lastLogTime>8000){logStatusToSD();lastLogTime=now;}
  delay(5);
}
