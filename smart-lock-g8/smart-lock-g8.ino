#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <Preferences.h>

Preferences preferences;

const char* ssid = "OPPO Reno8";
const char* wifiPassword = "d29ph9xh";
const char* BOT_TOKEN = "7290998230:AAGi2WAFE0QgAWh-FmmyhAU5sRyGVbkFMbc";
const char* CHAT_ID = "-4793674715";
const char* host = "e06671e9-6ab1-4100-85bb-683f3be2387b-00-wneid9i0hk23.sisko.replit.dev";
const int httpsPort = 443;

WiFiClientSecure secured_client;
UniversalTelegramBot telegramBot(BOT_TOKEN, secured_client);

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define RST_PIN 2
#define SS_PIN  5
MFRC522 mfrc522(SS_PIN, RST_PIN);
byte authorizedRFID[] = { 0x4A, 0xDA, 0x36, 0x02 };

#define FINGER_RX 16
#define FINGER_TX 17
HardwareSerial fingerSerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSerial);
uint8_t nextFingerID = 1;

#define RESET_BUTTON_PIN 12

volatile bool buttonPressed = false;
volatile bool authCancelled = false;
volatile unsigned long lastInterruptTime = 0;

void IRAM_ATTR handleButtonPress() {
  unsigned long currentMillis = millis();
  if (currentMillis- lastInterruptTime >= 100) {
    buttonPressed = true;
    authCancelled = true;
    lastInterruptTime = currentMillis;
  }
}

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {13, 14, 27, 26};
byte colPins[COLS] = {15, 25, 32, 33};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
 
#define LDR_PIN 34  
#define RELAY_PIN 1 

String systemPassword = "";
String currentOTP = "";

void waitForDelay(unsigned long ms) {
  unsigned long start = millis();
  while(millis() - start < ms) {
    if (buttonPressed) {  // check the flag
      break;
    }
    delay(10);
  }
}

bool checkRFID() {
  if (authCancelled) return true;
  if (!mfrc522.PICC_IsNewCardPresent()) return false;
  if (!mfrc522.PICC_ReadCardSerial()) return false;
  if (mfrc522.uid.size != sizeof(authorizedRFID)) return false;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] != authorizedRFID[i]) return false;
  }
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  return true;
}

bool checkFingerprint() {
  if (authCancelled) return true;
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return false;
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return false;
  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) return false;
  return (finger.fingerID > 0);
}

bool checkPassword() {
  lcd.setCursor(0, 1);
  String entered = "";
  while (entered.length() < 4) {
    if (authCancelled) return true;
    char key = keypad.getKey();
    if (key != NO_KEY) {
      lcd.print('*');
      entered += key;
      waitForDelay(300);
    }
  }
  return (entered == systemPassword);
}

String generateOTP() {
  int otp = random(100000, 1000000);
  return String(otp);
}

bool sendOTPViaTelegram(String otp) {
  String message = "Your OTP is: " + otp;
  return telegramBot.sendMessage(CHAT_ID, message, "");
}

bool checkOTP() {
  lcd.setCursor(0, 1);
  currentOTP = generateOTP();
  if (!sendOTPViaTelegram(currentOTP)) {
    return false;
  }
  String enteredOTP = "";
  while (enteredOTP.length() < 6) {
    if (authCancelled) return true;
    char key = keypad.getKey();
    if (key != NO_KEY) {
      lcd.print('*');
      enteredOTP += key;
      waitForDelay(300);
    }
  }
  return (enteredOTP == currentOTP);
}

bool tryAuthStep(const char* prompt, bool (*authFunc)(), const char* successMsg) {
  const int maxAttempts = 5;
  for (int attempt = 0; attempt < maxAttempts; attempt++) {
    if (buttonPressed) {
      buttonPressed = false;
      return true;
    }
    lcd.clear();
    lcd.print(prompt);
    waitForDelay(3000);
    if (authFunc()) {
      lcd.clear();
      lcd.print(successMsg);
      waitForDelay(3000);
      return true;
    }
    lcd.clear();
    lcd.print("Try again");
    lcd.setCursor(0, 1);
    lcd.print("Attempts: ");
    lcd.print(attempt + 1);
    waitForDelay(3000);
  }
  lcd.clear();
  lcd.print("Over 5 tries:");
  lcd.setCursor(0, 1);
  lcd.print("Restart process");
  waitForDelay(3000);
  return false;
}

bool securityCheck() {
  lcd.clear();
  lcd.print("Security Check:");
  waitForDelay(1000);
  
  authCancelled = false;
  
  if (!tryAuthStep("Input RFID", checkRFID, "RFID OK!"))
    return false;
  if (authCancelled) return true;
 
  if (!tryAuthStep("Scan Finger", checkFingerprint, "Fingerprint OK!"))
    return false;
  if (authCancelled) return true;
  
  if (!tryAuthStep("Enter Password", checkPassword, "Password OK!"))
    return false;
  if (authCancelled) return true;
  
  if (!tryAuthStep("Enter OTP", checkOTP, "OTP OK!"))
    return false;
  if (authCancelled) return true;
      
  return true;
}

void sendLightDataToWeb() {
  static unsigned long lastSend = 0;
  unsigned long now = millis();
  if (now - lastSend >= 1000) {
    lastSend = now;
    int lightValue = analogRead(LDR_PIN); 
    if (!secured_client.connect(host, httpsPort)) {
      return;
    }
    String url = "/iot/data?light=" + String(lightValue);
    secured_client.println("GET " + url + " HTTP/1.1");
    secured_client.println("Host: " + String(host));
    secured_client.println("Connection: close");
    secured_client.println();
    while (secured_client.connected()) {
      String line = secured_client.readStringUntil('\n');
      if (line == "\r") break;
    }
    secured_client.readString();
    secured_client.stop();
  }
}

void savePassword(const String &pwd) {
  preferences.begin("auth", false);
  preferences.putString("pwd", pwd);
  preferences.end();
}

String loadPassword() {
  preferences.begin("auth", true);
  String pwd = preferences.getString("pwd", "");
  preferences.end();
  return pwd;
}

void registerPassword() {
  lcd.clear();
  lcd.print("Set Password:");
  lcd.setCursor(0, 1);
  String pwd = "";
  while (pwd.length() < 4) {
    char key = keypad.getKey();
    if (key != NO_KEY) {
      lcd.print('*');
      pwd += key;
      delay(300);
    }
  }
  
  lcd.clear();
  lcd.print("Confirm:");
  lcd.setCursor(0, 1);
  String pwdConfirm = "";
  while (pwdConfirm.length() < 4) {      
    char key = keypad.getKey();
    if (key != NO_KEY) {
      lcd.print('*');
      pwdConfirm += key;
      delay(300);
    }
  }
  if (pwd == pwdConfirm) {
    systemPassword = pwd;
    savePassword(systemPassword);
    lcd.clear();
    lcd.print("Password Set!");
    delay(2000);
  } else {
    lcd.clear();
    lcd.print("Mismatch! Retry");
    delay(2000);
    registerPassword();
  }
}

void resetPasswordPrompt() {
  lcd.clear();
  lcd.print("Reset Password?");
  lcd.setCursor(0, 1);
  lcd.print("A: Yes");
  char key = 0;
  while (key == 0) { 
    key = keypad.getKey();
  }
  if (key == 'A') {
    registerPassword();
  }
  lcd.clear();
  lcd.print("Close after");
  lcd.setCursor(0, 1);
  lcd.print("10 seconds");
}

uint8_t enrollFinger(uint8_t id) {
  int p = 0;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enroll FP 1/2");
  while ((p = finger.getImage()) != FINGERPRINT_OK) {
    delay(200);
  }
  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Err FP 1");
    return p;
  }
  lcd.clear();
  lcd.print("Remove finger");
  delay(2000);
  lcd.clear();
  lcd.print("Enroll FP 2/2");
  while ((p = finger.getImage()) != FINGERPRINT_OK) {
    delay(200);
  }
  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Err FP 2");
    return p;
  }
  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Enroll Mismatch");
    return p;
  }
  p = finger.storeModel(id);
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Store Error");
    return p;
  }
  return FINGERPRINT_OK;
}

void initFingerprintSystem() {
  finger.getTemplateCount();
  if (finger.templateCount == 0) {
    lcd.clear();
    lcd.print("No FP stored");
    delay(2000);
    while (enrollFinger(nextFingerID) != FINGERPRINT_OK) {
      lcd.clear();
      delay(2000);
    }
    lcd.clear();
    lcd.print("Enroll Success");
    delay(2000);
    nextFingerID++; 
  } else {
    nextFingerID = finger.templateCount + 1;
    lcd.clear();
    lcd.print("System Ready");
    delay(2000);
  }
}

void clearAllFingerprints() {
  lcd.clear();
  lcd.print("Clearing FP...");
  for (uint8_t id = 1; id <= 127; id++) {
    uint8_t p = finger.deleteModel(id);
    if (p != FINGERPRINT_OK) id--;
    delay(50);
  }
  lcd.clear();
  lcd.print("Clear Complete");
  delay(2000);
}

void promptResetAndAddFingerprint() {
  char key = 0;
  lcd.clear();
  lcd.print("Reset FP?");
  lcd.setCursor(0, 1);
  lcd.print("A: Confirm");
  while (key == 0) { 
    key = keypad.getKey();
  }
  if (key == 'A') {
    clearAllFingerprints();
    lcd.clear();
    lcd.print("Setup New FP");
    delay(2000);
    while (enrollFinger(nextFingerID) != FINGERPRINT_OK) {
      lcd.clear();
      delay(2000);
    }
    lcd.clear();
    lcd.print("Enroll Success");
    nextFingerID = 1;
  }
  
  key = 0;
  lcd.clear();
  lcd.print("Enroll FP?");
  lcd.setCursor(0, 1);
  lcd.print("A: Confirm");
  while (key == 0) { 
    key = keypad.getKey();
  }
  while (key == 'A') {
    while (enrollFinger(nextFingerID) != FINGERPRINT_OK) {
      lcd.clear();
      delay(2000);
    }
    lcd.clear();
    lcd.print("Enroll Success");
    delay(2000);
    nextFingerID++; 
    key = 0;
    lcd.clear();
    lcd.print("Add another? A:Y B:N");
    while (key == 0) {
      key = keypad.getKey();
    }
    if (key != 'A') break;
  }
}

void setup() {
  WiFi.begin(ssid, wifiPassword);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  secured_client.setInsecure();
  sendLightDataToWeb();
  
  lcd.begin();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Start...");
  delay(2000);

  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RESET_BUTTON_PIN), handleButtonPress, FALLING);
  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  SPI.begin();
  mfrc522.PCD_Init();

  fingerSerial.begin(57600, SERIAL_8N1, FINGER_RX, FINGER_TX);
  delay(100);
  finger.begin(57600);
  if (!finger.verifyPassword()) {
    lcd.clear();
    lcd.print("FP sensor ERR");
    while (true) { delay(1); }
  }

  systemPassword = loadPassword();
  if (systemPassword == "") {
    lcd.clear();
    lcd.print("Set Password");
    registerPassword();
  }

  initFingerprintSystem();

  lcd.clear();
  sendLightDataToWeb();
}

bool isDoorOpen = false;
unsigned long doorOpenTime = 0;

void loop() {
  // Check and clear the button flag
  bool localButtonPressed = buttonPressed;
  buttonPressed = false;
  
  if (localButtonPressed) {
    if (isDoorOpen) {
      digitalWrite(RELAY_PIN, HIGH);
      lcd.clear();
      lcd.print("Door Locked");
      isDoorOpen = false;
    }
    else {
      digitalWrite(RELAY_PIN, LOW);
      lcd.clear();
      lcd.print("Door Unlocked");
      isDoorOpen = true;
      doorOpenTime = millis();
    }
  }
  
  // Auto-lock door after 10 seconds if open.
  if (isDoorOpen && (millis() - doorOpenTime >= 10000)) {
    digitalWrite(RELAY_PIN, HIGH);
    lcd.clear();
    lcd.print("Door Auto Locked");
    isDoorOpen = false;
  }
  
  // When door is closed, perform security check.
  if (!isDoorOpen && !buttonPressed) {
    if (securityCheck()) {
      digitalWrite(RELAY_PIN, LOW);
      lcd.clear();
      lcd.print("Access Granted");
      lcd.setCursor(0, 1);
      lcd.print("Door Unlocked");
      isDoorOpen = true;
      sendLightDataToWeb();
      promptResetAndAddFingerprint();
      resetPasswordPrompt();
      doorOpenTime = millis();
    }
  }
  
  // Periodically send light data.
  sendLightDataToWeb();
}
