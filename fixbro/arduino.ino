#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "DFRobot_ESP_PH.h"
#include "EEPROM.h"
#include <ArduinoEigen.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define btn1_pin 5
#define btn2_pin 13
#define ESPADC 4096.0
#define ESPVOLTAGE 3300
#define PH_PIN 4
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32  
#define OLED_RESET 4
#define API_KEY "AIzaSyCqFzfUAZbbA0nIB6OHZwsrEbLgMpY37YI"
#define DATABASE_URL "https://mahrusul-58273-default-rtdb.asia-southeast1.firebasedatabase.app/"

enum MenuState {
  MAIN_MENU,
  MEASUREMENT,
  WIFI_SETTING,
  DATA_TRANSFER,
  AGE_INPUT_TENS,
  AGE_INPUT_ONES,
  ID_INPUT_HUNDREDS,
  ID_INPUT_TENS,
  ID_INPUT_ONES,
  MEASURING,
  SHOW_RESULTS,
  WIFI_AP_MODE,
  WIFI_CONNECTING,
  WIFI_TURNING_OFF,
  WIFI_TURNING_ON
};

FirebaseAuth auth;
FirebaseData fbdo;
FirebaseConfig config;
using namespace Eigen;

String idPath = "/deviceID";
String gluPath = "/glucose";
String uridPath = "/urid";
String agePath = "/age";
String pHPath = "/pH";
String timePath = "/timestamp";

MenuState currentState = MAIN_MENU;
int currentMenuItem = 0;
float voltage, phValue, outpH, pHREG;
float phBuffer[60];
int bufferIndex;
float averagePH;
bool collectingData;
int ageTens = 0;
int ageOnes = 0;
int age = 0;
int idHundreds = 0;
int idTens = 0;
int idOnes = 0;
int deviceID = 0;
unsigned long measurementStartTime = 0;
bool measurementCompleted = false;
unsigned long lastBtn1PressTime = 0;
unsigned long lastBtn2PressTime = 0;
unsigned long btn1PressStartTime = 0;
unsigned long btn2PressStartTime = 0;
bool btn1Pressed = false;
bool btn2Pressed = false;
unsigned long btnDebounceTime = 200;
String stored_ssid = "";
String stored_password = "";
bool wifiConfigured = false;
bool wifiCredentialsExist = false;
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* var2Path = "/data/value.txt";
AsyncWebServer server(80);
bool transferInProgress = false;
int transferProgress = 0;
float temperature = 25.0;
float GLU, ACD;
String timestamp1;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DFRobot_ESP_PH ph;
bool wifiWasActiveBeforeMeasurement = false;
WiFiMode_t previousWiFiMode = WIFI_OFF;
bool attemptingReconnectAfterMeasurement = false;
const long utcOffsetInSeconds = 7 * 3600;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

void startWiFiAPMode();
void attemptWiFiConnection();
String readFile(fs::FS &fs, const char *path);
void writeFile(fs::FS &fs, const char *path, const char *message);
void showWiFiAPScreen();
void showWiFiConnectingScreen(const String& targetSsid, bool isConnecting, const String& message = "");
void saveSelectedDataToJson(float GLU, float ACD, int age, float finalAvgPH, int deviceIDs, String timestamp11);
float myNeuralNetworkFunction(const Eigen::Vector2f& input, int network_id);
uint8_t roundUpToUint8(float value);
float roundToOneDecimal(float value);
void createDir(fs::FS &fs, const char *path);
String getTimestamp();
void checkAndRestoreWiFi();
void turnOffWiFiForMeasurement();
void deleteFile(fs::FS &fs, const char *path);
void setupFirebase();
void sendDataToFirebase();
void printSavedData();

void setup() {
  Serial.begin(115200);
  EEPROM.begin(32);
  ph.begin();
  pinMode(btn1_pin, INPUT_PULLUP);
  pinMode(btn2_pin, INPUT_PULLUP);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 Failed"));
    for (;;);
  }
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  delay(500);
  display.clearDisplay();
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount Failed. Formatting and retrying...");
    display.clearDisplay();
    display.setCursor(0, 8);
    display.println("LittleFS Error!");
    display.display();
    delay(3000);
  } else {
    Serial.println("LittleFS Mounted Successfully");
  }
  showGLUMON();
}

void loop() {
  unsigned long currentMillis = millis();
  handleButton1(currentMillis);
  handleButton2(currentMillis);
  switch (currentState) {
    case MAIN_MENU:
      break;
    case MEASUREMENT:
      turnOffWiFiForMeasurement();
      if (currentState == MEASUREMENT) {
        currentState = AGE_INPUT_TENS;
        showAgeInputTens();
      }
      break;
    case WIFI_TURNING_OFF:
      break;
    case AGE_INPUT_TENS:
    case AGE_INPUT_ONES:
    case ID_INPUT_HUNDREDS:
    case ID_INPUT_TENS:
    case ID_INPUT_ONES:
      break;
    case MEASURING:
      if (collectingData) {
        if (bufferIndex < 60) {
          static unsigned long timepoint = millis();
          if (millis() - timepoint > 1000U) {
            timepoint = millis();
            readPH();
            updateMeasurementOLED();
          }
        } else {
          collectingData = false;
          measurementCompleted = true;
          calculateANNValues();
          currentState = SHOW_RESULTS;
          showResults();
        }
      }
      break;
    case SHOW_RESULTS:
      break;
    case WIFI_SETTING:
      startWiFiAPMode();
      break;
    case WIFI_AP_MODE:
      break;
    case WIFI_CONNECTING:
      break;
    case DATA_TRANSFER:
      showDataTransfer();
      break;
  }
}

void turnOffWiFiForMeasurement() {
  timestamp1 = getTimestamp();
  printSavedData();
  delay(300);
  previousWiFiMode = WiFi.getMode();
  if (previousWiFiMode != WIFI_OFF) {
    wifiWasActiveBeforeMeasurement = true;
    currentState = WIFI_TURNING_OFF;
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 8);
    display.println("OFF WiFi untuk");
    display.setCursor(0, 16);
    display.println("Ukur pH...");
    display.display();
    Serial.println("WiFi active, disabling for pH measurement...");
    if (previousWiFiMode == WIFI_AP || previousWiFiMode == WIFI_AP_STA) {
      WiFi.softAPdisconnect(true);
      server.end();
      Serial.println("AP Mode and server stopped.");
    }
    if (WiFi.status() == WL_CONNECTED) {
      WiFi.disconnect(true, false);
      Serial.println("STA Mode disconnected.");
    }
    WiFi.mode(WIFI_OFF);
    delay(1000);
    wifiConfigured = false;
    Serial.println("WiFi radio turned OFF.");
    currentState = AGE_INPUT_TENS;
    showAgeInputTens();
  } else {
    wifiWasActiveBeforeMeasurement = false;
    Serial.println("WiFi was not active. Proceeding to measurement.");
  }
}


void checkAndRestoreWiFi() {
  if (wifiWasActiveBeforeMeasurement) {
    Serial.println("Measurement/Operation finished. Re-attempting WiFi connection...");
    attemptingReconnectAfterMeasurement = true;
    attemptWiFiConnection();
  } else {
    Serial.println("WiFi was not active before. Returning to Main Menu.");
    currentState = MAIN_MENU;
    showMainMenu();
  }
  wifiWasActiveBeforeMeasurement = false;
}

void handleButton1(unsigned long currentMillis) {
  if (digitalRead(btn1_pin) == LOW) {
    if (!btn1Pressed) {
      btn1Pressed = true;
      btn1PressStartTime = currentMillis;
    } else {
      if ((currentState == ID_INPUT_ONES) && (currentMillis - btn1PressStartTime >= 2000)) {
        deviceID = (idHundreds * 100) + (idTens * 10) + idOnes;
        startMeasurement();
        btn1Pressed = false;
        return;
      } else if (currentState == MEASURING && collectingData && (currentMillis - btn1PressStartTime >= 2000)) {
        stopAndResetMeasurement();
        btn1Pressed = false;
        return;
      }
    }
  } else {
    if (btn1Pressed) {
      if (currentMillis - lastBtn1PressTime >= btnDebounceTime) {
        lastBtn1PressTime = currentMillis;
        if (currentState == MAIN_MENU) {
          selectMenuItem();
        }
        else if (currentState == AGE_INPUT_TENS) { ageTens = (ageTens + 1) % 10; showAgeInputTens(); }
        else if (currentState == AGE_INPUT_ONES) { ageOnes = (ageOnes + 1) % 10; showAgeInputOnes(); }
        else if (currentState == ID_INPUT_HUNDREDS) { idHundreds = (idHundreds + 1) % 10; showIDInputHundreds(); }
        else if (currentState == ID_INPUT_TENS) { idTens = (idTens + 1) % 10; showIDInputTens(); }
        else if (currentState == ID_INPUT_ONES) { idOnes = (idOnes + 1) % 10; showIDInputOnes(); }
        else if (currentState == SHOW_RESULTS) {
          checkAndRestoreWiFi();
        }
        else if (currentState == WIFI_AP_MODE) {
          WiFi.softAPdisconnect(true); server.end();
          currentState = MAIN_MENU; showMainMenu();
        }
        else if (currentState == WIFI_CONNECTING || currentState == WIFI_TURNING_OFF) {
          WiFi.disconnect(true);
          WiFi.mode(WIFI_OFF);
          wifiConfigured = false;
          attemptingReconnectAfterMeasurement = false;
          wifiWasActiveBeforeMeasurement = false;
          currentState = MAIN_MENU; showMainMenu();
        }
        else if (currentState == DATA_TRANSFER) {
          if (!transferInProgress) {
            if (wifiConfigured) {
              transferInProgress = true; transferProgress = 0;
            } else {
              display.clearDisplay();
              display.setTextSize(1);
              display.setCursor(0, 8);
              display.println("WiFi tidak terhubung!");
              display.setCursor(0, 16);
              display.println("Atur WiFi dahulu.");
              display.display();
              delay(2000);
            }
          } else {
            transferInProgress = false;
          }
          showDataTransfer();
        }
      }
      btn1Pressed = false;
    }
  }
}

void handleButton2(unsigned long currentMillis) {
  if (digitalRead(btn2_pin) == LOW) {
    if (!btn2Pressed) { btn2Pressed = true; btn2PressStartTime = currentMillis; }
  } else {
    if (btn2Pressed) {
      if (currentMillis - lastBtn2PressTime >= btnDebounceTime) {
        lastBtn2PressTime = currentMillis;
        if (currentState == MAIN_MENU) { currentMenuItem = (currentMenuItem + 1) % 3; showMainMenu(); }
        else if (currentState == AGE_INPUT_TENS) { currentState = AGE_INPUT_ONES; showAgeInputOnes(); }
        else if (currentState == AGE_INPUT_ONES) { age = (ageTens * 10) + ageOnes; currentState = ID_INPUT_HUNDREDS; showIDInputHundreds(); }
        else if (currentState == ID_INPUT_HUNDREDS) { currentState = ID_INPUT_TENS; showIDInputTens(); }
        else if (currentState == ID_INPUT_TENS) { currentState = ID_INPUT_ONES; showIDInputOnes(); }
        else if (currentState == ID_INPUT_ONES) { deviceID = (idHundreds * 100) + (idTens * 10) + idOnes; showIDInputOnes(); }
        else if (currentState == WIFI_AP_MODE) {
          WiFi.softAPdisconnect(true); server.end();
          currentState = MAIN_MENU; showMainMenu();
        }
        else if (currentState == WIFI_CONNECTING || currentState == WIFI_TURNING_OFF) {
          WiFi.disconnect(true); WiFi.mode(WIFI_OFF);
          wifiConfigured = false;
          attemptingReconnectAfterMeasurement = false;
          wifiWasActiveBeforeMeasurement = false;
          currentState = MAIN_MENU; showMainMenu();
        }
        else if (currentState == SHOW_RESULTS) {
          checkAndRestoreWiFi();
        }
        else if (currentState == DATA_TRANSFER) {
          transferInProgress = false;
          currentState = MAIN_MENU;
          showMainMenu();
        }
      }
      btn2Pressed = false;
    }
  }
}

void showGLUMON() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor((SCREEN_WIDTH - (6 * 12)) / 2, (SCREEN_HEIGHT - 16) / 2); 
  display.print("GLUMON");
  display.display();
  delay(2000);
  stored_ssid = readFile(LittleFS, ssidPath);
  stored_password = readFile(LittleFS, passPath);
  wifiCredentialsExist = (stored_ssid.length() > 0);
  if (wifiCredentialsExist) {
    attemptWiFiConnection();
  } else {
    currentState = MAIN_MENU;
    showMainMenu();
  }
  timeClient.begin();
  createDir(LittleFS, "/data");
  //deleteFile(LittleFS, var2Path);
}

void showMainMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(82, 0);
  if (wifiConfigured && WiFi.status() == WL_CONNECTED) {
    display.print("WiFi: ON");
  } else {
    display.print("WiFi: OFF");
  }

  for (int i = 0; i < 3; i++) {
    display.setCursor(0, 8 + (i * 8)); 
    if (i == currentMenuItem) {
      display.print("> ");
    } else {
      display.print("  ");
    }
    switch (i) {
      case 0: display.print("Pengukuran"); break;
      case 1: display.print("Setting WiFi"); break;
      case 2: display.print("Transfer Data"); break;
    }
  }
  display.display();
}

void selectMenuItem() {
  switch (currentMenuItem) {
    case 0:
      currentState = MEASUREMENT;
      break;
    case 1:
      currentState = WIFI_SETTING;
      break;
    case 2:
      currentState = DATA_TRANSFER;
      transferInProgress = false;
      break;
  }
}

String readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if (!file || file.isDirectory()) {
    Serial.println("- empty or failed to open file for reading");
    return String();
  }
  String fileContent;
  if (file.available()) {
    fileContent = file.readString();
  }
  file.close();
  fileContent.trim();
  return fileContent;
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

void startWiFiAPMode() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect(false);
    delay(100);
  }
  WiFi.mode(WIFI_AP);
  currentState = WIFI_AP_MODE;
  WiFi.softAP("GLUMON-Setup", "12345678");
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(apIP);
  showWiFiAPScreen();
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/wifimanager.html", "text/html");
  });
  server.on("/savewifi", HTTP_POST, [](AsyncWebServerRequest *request) {
    String new_ssid;
    String new_pass;
    bool credsChanged = false;
    if (request->hasParam("ssid", true)) {
      new_ssid = request->getParam("ssid", true)->value();
      if (new_ssid != stored_ssid) {
        writeFile(LittleFS, ssidPath, new_ssid.c_str());
        stored_ssid = new_ssid;
        credsChanged = true;
      }
    }
    if (request->hasParam("pass", true)) {
      new_pass = request->getParam("pass", true)->value();
      if (new_pass != stored_password || (credsChanged && stored_password.isEmpty())) {
        writeFile(LittleFS, passPath, new_pass.c_str());
        stored_password = new_pass;
        credsChanged = true;
      }
    }
    wifiCredentialsExist = (stored_ssid.length() > 0);
    request->send(200, "text/plain", "OK. Credentials saved. Device will attempt to connect if changed.");
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("WiFi Credentials");
    display.setCursor(0, 10);
    if(credsChanged) display.println("Tersimpan!"); else display.println("Tidak ada perubahan.");
    display.setCursor(0, 20);
    display.println("Keluar dari mode AP...");
    display.display();
    delay(2000);
    WiFi.softAPdisconnect(true);
    server.end();
    if (credsChanged && wifiCredentialsExist) {
      attemptingReconnectAfterMeasurement = false;
      attemptWiFiConnection();
    } else {
      currentState = MAIN_MENU;
      showMainMenu();
    }
  });
  server.begin();
}

void attemptWiFiConnection() {
  String temp_ssid = readFile(LittleFS, ssidPath);
  String temp_pass = readFile(LittleFS, passPath);
  wifiCredentialsExist = (temp_ssid.length() > 0);
  if (wifiCredentialsExist) {
    stored_ssid = temp_ssid;
    stored_password = temp_pass;
    currentState = WIFI_CONNECTING;
    String connectingMsg = "Menyambungkan ke:";
    if(attemptingReconnectAfterMeasurement) {
      connectingMsg = "Memulihkan WiFi:";
    }
    showWiFiConnectingScreen(stored_ssid, true, connectingMsg);
    WiFi.mode(WIFI_STA);
    WiFi.begin(stored_ssid.c_str(), stored_password.c_str());
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(stored_ssid);
    unsigned long startTime = millis();
    int dots = 0;
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime < 10000)) {
      Serial.print(".");
      display.setCursor(0, 24); 
      display.print("                "); 
      display.setCursor(0, 24);
      for(int i=0; i<dots; i++) display.print(".");
      display.display();
      dots = (dots + 1) % 16;
      delay(500);
      if (digitalRead(btn1_pin) == LOW || digitalRead(btn2_pin) == LOW) {
      }
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi connected!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      wifiConfigured = true;
      showWiFiConnectingScreen(stored_ssid, false);
      delay(2000);
    } else {
      Serial.println("\nFailed to connect.");
      wifiConfigured = false;
      showWiFiConnectingScreen(stored_ssid, false);
      delay(2000);
    }
  } else {
    Serial.println("No WiFi credentials stored to attempt connection.");
    wifiConfigured = false;
  }
  attemptingReconnectAfterMeasurement = false;
  currentState = MAIN_MENU;
  showMainMenu();
}

void showWiFiAPScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Setup WiFi (Mode AP)");
  display.setCursor(0, 8);
  display.println("SSID: GLUMON-Setup");
  display.setCursor(0, 16);
  display.println("IP: " + WiFi.softAPIP().toString());
  display.setCursor(0, 24);
  display.println("Pass: 12345678");
  display.display();
}

void showWiFiConnectingScreen(const String& targetSsid, bool isConnecting, const String& message) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  if (!message.isEmpty()) {
    display.println(message);
  } else {
    display.println("Koneksi WiFi");
  }
  display.setCursor(0, 8);
  display.print("SSID: ");
  display.println(targetSsid.substring(0,12));

  if (isConnecting) {
    display.setCursor(0, 16);
    display.print("Menunggu...");
  } else {
    display.setCursor(0, 16);
    if (wifiConfigured) {
      display.println("TERHUBUNG!");
      display.setCursor(0, 24);
      display.print("IP:" + WiFi.localIP().toString());
    } else {
      display.println("KONEKSI GAGAL!");
    }
  }
  display.display();
}

void showAgeInputTens() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Umur: Puluhan");

  display.setTextSize(2);
  display.setCursor(58, 10); 
  display.print(ageTens);
  display.print("_");

  display.setTextSize(1);
  display.setCursor(0, 26);
  display.print("Btn1=Ubah Btn2=Next");
  display.display();
}

void showAgeInputOnes() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Umur: Satuan");

  display.setTextSize(2);
  display.setCursor(45, 10); 
  display.print(ageTens);
  display.print(ageOnes);

  display.setTextSize(1);
  display.setCursor(0, 26);
  display.print("Btn1=Ubah Btn2=Next");
  display.display();
}

void showIDInputHundreds() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("ID: Ratusan");

  display.setTextSize(2);
  display.setCursor(50, 10);
  display.print(idHundreds);
  display.print("__");

  display.setTextSize(1);
  display.setCursor(0, 26);
  display.print("Btn1=Ubah Btn2=Next");
  display.display();
}

void showIDInputTens() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("ID: Puluhan");

  display.setTextSize(2);
  display.setCursor(45, 10);
  display.print(idHundreds);
  display.print(idTens);
  display.print("_");

  display.setTextSize(1);
  display.setCursor(0, 26);
  display.print("Btn1=Ubah Btn2=Next");
  display.display();
}

void showIDInputOnes() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("ID: Satuan");

  display.setTextSize(2);
  display.setCursor(45, 10);
  display.print(idHundreds);
  display.print(idTens);
  display.print(idOnes);

  display.setTextSize(1);
  display.setCursor(0, 26);
  display.print("Tahan Btn1=Mulai");
  display.display();
}

void startMeasurement() {
  measurementStartTime = millis();
  collectingData = true;
  bufferIndex = 0;
  averagePH = 0;
  measurementCompleted = false;
  currentState = MEASURING;
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Mengukur..."); 
  display.setCursor(0, 8);
  display.print("ID:");
  display.print(deviceID);
  display.print(" Usia:");
  display.print(age);
  display.setCursor(0, 16);
  display.println("WiFi: MATI");
  display.setCursor(0, 24);
  display.print("Btn1=Batal");
  display.display();
}

void readPH() {
  voltage = analogRead(PH_PIN) / ESPADC * ESPVOLTAGE;
  phValue = ph.readPH(voltage, temperature);
  pHREG = -0.1184 + 1.083 * phValue;
  if (bufferIndex < 60) {
    phBuffer[bufferIndex] = pHREG;
    averagePH += pHREG;
    bufferIndex++;
  }
}

void updateMeasurementOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Mengukur pH");
  display.setCursor(90, 0);
  display.print(bufferIndex);
  display.print("/60");

  display.setCursor(0, 8);
  display.print("pH: ");
  display.print(pHREG, 2);
  display.setCursor(64, 8);
  display.print("Avg: ");
  if (bufferIndex > 0) {
    display.print(averagePH / bufferIndex, 2);
  } else {
    display.print("N/A");
  }

  display.setCursor(0, 16);
  display.print("ID:");
  display.print(deviceID);
  display.setCursor(64, 16);
  display.print("Umur:");
  display.print(age);

  display.setCursor(0, 24);
  display.print("Tahan Btn1 u/ batal");
  display.display();
}

void stopAndResetMeasurement() {
  collectingData = false;
  bufferIndex = 0;
  averagePH = 0;
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 8);
  display.println("Pengukuran dihentikan");
  display.setCursor(0, 16);
  display.println("Kembali...");
  display.display();
  delay(1500);
  checkAndRestoreWiFi();
}

void calculateANNValues() {
  float finalPH = 0;
  if (bufferIndex > 0) {
    finalPH = averagePH / bufferIndex;
  } else if (measurementCompleted && bufferIndex == 0 && averagePH != 0) {
    finalPH = averagePH / 60.0;
  }

  Eigen::Vector2f inputData;
  inputData << age, finalPH;
  GLU = roundUpToUint8(myNeuralNetworkFunction(inputData, 1));
  ACD = roundToOneDecimal(myNeuralNetworkFunction(inputData, 2));

  Serial.print("Device ID: "); Serial.println(deviceID);
  Serial.print("Age: "); Serial.println(age);
  Serial.print("Final Avg pH: "); Serial.println(finalPH, 2);
  Serial.print("Glucose: "); Serial.println(GLU);
  Serial.print("Uric Acid: "); Serial.println(ACD, 1);
}

void showResults() {
  display.clearDisplay();
  display.setTextSize(1);
  
  display.setCursor(0, 0);
  display.print("GLU: ");
  display.print((int)GLU);
  display.setCursor(64, 0);
  display.print("ACD: ");
  display.print(ACD, 1);
  
  display.setCursor(0, 8);
  display.print("pH Avg: ");
  float finalAvgPH = 0;
  if (measurementCompleted && bufferIndex == 60) {
      finalAvgPH = averagePH / 60.0;
  } else if (bufferIndex > 0) {
      finalAvgPH = averagePH / bufferIndex;
  } else if (averagePH != 0) {
      finalAvgPH = averagePH / 60.0;
  }
  display.print(finalAvgPH, 2);
  
  display.setCursor(0, 16);
  display.print("ID: ");
  display.print(deviceID);
  display.setCursor(64, 16);
  display.print("Umur: ");
  display.print(age);
  
  display.setCursor(0, 24);
  display.print("Tekan tombol u/ Menu");

  display.display();
  saveSelectedDataToJson(GLU, ACD, age, finalAvgPH, deviceID, timestamp1);
  delay(500);
}

void showDataTransfer() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Transfer Data");
  
  static uint8_t step = 0;
  static unsigned long lastActionTime = 0;
  static bool transferSuccess = false;
  static unsigned long finishTime = 0;

  if (!wifiConfigured) {
    display.setCursor(0, 8);
    display.println("WiFi tidak terhubung!");
    display.setCursor(0, 16);
    display.println("Atur WiFi dahulu.");
    display.setCursor(0, 24);
    display.print("Btn2: Kembali");
    display.display();
    step = 0;
    transferInProgress = false;
    return;
  }

  if (!transferInProgress) {
    step = 0;
    transferProgress = 0;
    display.setCursor(0, 8);
    display.println("Siap untuk transfer.");
    display.setCursor(0, 16);
    display.println("Btn1: Mulai");
    display.setCursor(0, 24);
    display.println("Btn2: Kembali");
    display.display();
    return;
  }

  switch(step) {
    case 0:
      display.setCursor(0, 12);
      display.print("Inisialisasi Firebase...");
      display.display();
      setupFirebase();
      delay(500);
      step = 1;
      lastActionTime = millis();
      break;
    case 1:
      display.setCursor(0, 12);
      display.print("Menyiapkan data...");
      display.display();
      if (millis() - lastActionTime > 600) {
        step = 2;
        transferProgress = 0;
      }
      break;
    case 2:
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Mengirim data...");
      display.setCursor(0, 8);
      display.print("Progres: ");
      display.print(transferProgress);
      display.print("%");
      display.drawRect(0, 22, 128, 10, SSD1306_WHITE);
      display.fillRect(1, 23, (int)(transferProgress * 1.26f), 8, SSD1306_WHITE);
      display.display();
      sendDataToFirebase();
      delay(600);
      if (transferProgress < 100) {
        transferProgress += 34;
        if (transferProgress >= 100) {
          transferProgress = 100;
          step = 3;
        }
      }
      break;
    case 3:
      transferSuccess = true;
      display.clearDisplay();
      display.setCursor(0, 12);
      if (transferSuccess) {
        display.println("Transfer Selesai!");
      } else {
        display.println("Transfer Gagal!");
      }
      display.display();
      if (finishTime == 0) {
        finishTime = millis();
      }
      if (millis() - finishTime > 2000) {
        finishTime = 0;
        transferInProgress = false;
        step = 0;
        currentState = MAIN_MENU;
        showMainMenu();
      }
      break;
  }
}

uint8_t roundUpToUint8(float value) {
  return static_cast<uint8_t>(ceil(value));
}

float roundToOneDecimal(float value) {
  return round(value * 10.0f) / 10.0f;
}

float myNeuralNetworkFunction(const Eigen::Vector2f& input, int network_id) {
  Eigen::Vector2f x = input;
  Eigen::Vector2f xp1_func;
  Eigen::VectorXf a1_func;
  float a2_func;
  float y1_output_func;

  switch (network_id) {
    case 1: {
      Eigen::Vector2f x_offset(20, 4);
      Eigen::Vector2f gain(0.0338983050847458, 0.5);
      float ymin = -1.0f;

      xp1_func = (x - x_offset).array() * gain.array() + ymin;

      Eigen::Matrix<float, 3, 2> IW1_1;
      IW1_1 << -0.0341205414892376, 0.194609558662760, -0.775647603912303,
               -0.595577267457639, 0.0337247547836874, -0.192710374520464;

      Eigen::Vector3f b1_nn;
      b1_nn << 0.0407180406625324, -0.406812727844678, -0.0416473331855847;

      Eigen::Vector3f a1_eigen_nn = (IW1_1 * xp1_func + b1_nn).unaryExpr([](float val) {
        return 2.0f / (1.0f + exp(-2.0f * val)) - 1.0f;
      });

      float b2_nn = -0.4513649673311226;
      Eigen::RowVector3f LW2_1;
      LW2_1 << -0.2131715910488703, -0.6368758170205874, 0.2108618688416308;

      a2_func = LW2_1 * a1_eigen_nn + b2_nn;

      float y_gain = 0.0138888888888889;
      float y_xoffset = 70;
      float y_ymin = -1.0f;
      y1_output_func = (a2_func - y_ymin) / y_gain + y_xoffset;
      break;
    }
    case 2: {
      Eigen::Vector2f x_offset(9, 4);
      Eigen::Vector2f gain(0.0285714285714286, 0.5);
      xp1_func = (x - x_offset).array() * gain.array() + (-1.0f);

      Eigen::VectorXf b1_eigen_nn(3);
      b1_eigen_nn << -0.039989071764135138143, 0.002798737081869573113,
                     0.16710924791856981986;
      Eigen::MatrixXf IW1_1(3, 2);
      IW1_1 << -0.13295832881039401641, 0.16255663871690292921,
               0.048838490426261982336, -0.062213751675714128175,
               -0.79077477289835607088, -0.97703844702008724177;

      Eigen::VectorXf layer1 = IW1_1 * xp1_func + b1_eigen_nn;
      a1_func.resize(3);
      for (int i = 0; i < 3; i++) {
        a1_func[i] = 2.0f / (1.0f + exp(-2.0f * layer1[i])) - 1.0f;
      }

      float b2_nn = 0.01197581698435526594;
      Eigen::RowVectorXf LW2_1(3);
      LW2_1 << -0.2224241913705942153f, 0.079396198239153337184f,
               -0.91512771341384602231f;
      a2_func = LW2_1 * a1_func + b2_nn;

      float y_gain = 0.416666666666667;
      float y_xoffset = 2.4;
      y1_output_func = (a2_func - (-1.0f)) / y_gain + y_xoffset;
      break;
    }
    default:
      return -1.0f;
  }
  return y1_output_func;
}

void saveSelectedDataToJson(float GLU, float ACD, int age, float finalAvgPH, int deviceIDs, String timestamp11) {
  DynamicJsonDocument doc(8192);
  File file = LittleFS.open(var2Path, FILE_READ);
  if (file) {
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error || !doc["sensor"] || !doc["sensor"].is<JsonArray>()) {
      doc.clear(); 
      doc.createNestedArray("sensor");
    }
  } else {
    doc.clear();
    doc.createNestedArray("sensor");
  }

  JsonArray sensorArray = doc["sensor"].as<JsonArray>();
  JsonObject data = sensorArray.createNestedObject();
  data["timestamp"] = timestamp11;
  data["glucose"] = GLU;
  data["uric_acid"] = ACD;
  data["age"] = age;
  data["avg_ph"] = finalAvgPH;
  data["devicesID"] = deviceIDs;

  file = LittleFS.open(var2Path, FILE_WRITE);
  if (!file) {
    Serial.println("Gagal membuka file untuk menulis.");
    return;
  }
  serializeJson(doc, file);
  file.close();
  Serial.println("Data berhasil DITAMBAHKAN ke file JSON di LittleFS");
}

String getTimestamp() {
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();
  if (epochTime < 1577836800UL) {
    Serial.println("[ERROR] NTP belum sinkron! epoch: " + String(epochTime) + ". Mengembalikan default timestamp.");
    return "1970-01-01T00:00:00+07:00";
  }
  time_t epoch_for_gmtime = epochTime;
  struct tm *ptm = gmtime(&epoch_for_gmtime);
  if (!ptm) {
    Serial.println("[ERROR] gmtime gagal mengkonversi epoch: " + String(epoch_for_gmtime));
    return "0000-00-00T00:00:00+07:00";
  }
  char buffer[26];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d+07:00",
           ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
           ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
  return String(buffer);
}

void createDir(fs::FS &fs, const char *path) {
  fs.mkdir(path);
}

void setupFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Firebase.reconnectWiFi(true);
  } else {
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void deleteFile(fs::FS &fs, const char *path) {
  fs.remove(path);
}

void sendDataToFirebase() {
  if (!Firebase.ready()) {
    return;
  }

  File file = LittleFS.open(var2Path, FILE_READ);
  if (!file) {
    return;
  }

  DynamicJsonDocument doc(8192);
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    return;
  }

  JsonArray sensorArray = doc["sensor"].as<JsonArray>();
  bool allDataSent = true;

  for (JsonObject data : sensorArray) {
    String timestamp = data["timestamp"].as<String>();
    FirebaseJson json;
    json.set(idPath.c_str(), String(data["devicesID"].as<int>()));
    json.set(gluPath.c_str(), String(data["glucose"].as<float>()));
    json.set(uridPath.c_str(), String(data["uric_acid"].as<float>()));
    json.set(pHPath.c_str(), String(data["avg_ph"].as<float>()));
    json.set(agePath.c_str(), String(data["age"].as<int>()));
    json.set(timePath.c_str(), timestamp);

    String databasePath = "/sensor";
    String parentPath = databasePath + "/" + timestamp;

    if (Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json)) {
      Serial.println("Data JSON berhasil dikirim di firebase");
    } else {
      allDataSent = false;
    }
  }

  if (allDataSent) {
    deleteFile(LittleFS, var2Path);
    Serial.print("Data JSON telah dihapus dari memori flash");
  }
}

void printSavedData() {
  File file = LittleFS.open(var2Path, FILE_READ);
  if (!file) {
    Serial.println("File tidak ditemukan!");
    return;
  }
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
  Serial.println();
}
