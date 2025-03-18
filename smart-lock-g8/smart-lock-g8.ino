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

//========= CẤU HÌNH WiFi & Telegram (giữ nguyên) =========
const char* ssid = "OPPO Reno8";
const char* wifiPassword = "d29ph9xh";
const char* BOT_TOKEN = "7290998230:AAGi2WAFE0QgAWh-FmmyhAU5sRyGVbkFMbc";
const char* CHAT_ID = "-4793674715";

WiFiClientSecure secured_client;
UniversalTelegramBot telegramBot(BOT_TOKEN, secured_client);

//========= CẤU HÌNH LCD I2C (giữ nguyên) =========
LiquidCrystal_I2C lcd(0x27, 16, 2);

//========= CẤU HÌNH RFID RC522 (giữ nguyên) =========
#define RST_PIN 2
#define SS_PIN  5
MFRC522 mfrc522(SS_PIN, RST_PIN);
byte authorizedRFID[] = { 0x4A, 0xDA, 0x36, 0x02 };

//========= CẤU HÌNH FINGERPRINT AS608 (giữ nguyên) =========
#define FINGER_RX 16
#define FINGER_TX 17
HardwareSerial fingerSerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSerial);
uint8_t authorizedFingerID = 1;

//========= CẤU HÌNH NÚT RESET (giữ nguyên) =========
#define RESET_BUTTON_PIN 12

//========= CẤU HÌNH KEYPAD 4x4 (giữ nguyên) =========
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

//========= Lưu mật khẩu hệ thống (giữ nguyên) =========
String systemPassword = "";

//========= Biến lưu OTP (giữ nguyên) =========
String currentOTP = "";

//========= THÊM MỚI: CẤU HÌNH RELAY (điều khiển solenoid lock) =========
// Chọn 1 chân output an toàn. Ví dụ: GPIO14 (nếu board cho phép). 
// Nếu không, bạn thay thế bằng chân phù hợp, KHÔNG xung đột với flash/USB.
#define RELAY_PIN 1 

//========= CÁC HÀM OTP, KEY, ENROLL, CHECKRFID, CHECKPASSWORD,... (GIỮ NGUYÊN) =========
String generateOTP() {
  int otp = random(100000, 1000000);
  return String(otp);
}
bool sendOTPViaTelegram(String otp) {
  String message = "Your OTP is: " + otp;
  if (telegramBot.sendMessage(CHAT_ID, message, "")) {
    Serial.println("OTP sent via Telegram!");
    return true;
  } else {
    Serial.println("Error sending OTP via Telegram!");
    return false;
  }
}
String getKeypadInput() {
  String input = "";
  lcd.clear();
  lcd.print("Enter OTP:");
  lcd.setCursor(0, 1);
  while (input.length() < 6) {
    char key = keypad.getKey();
    if (key != NO_KEY) {
      input += key;
      lcd.print("*");
      delay(300);
    }
  }
  return input;
}
bool checkOTP() {
  currentOTP = generateOTP();
  Serial.print("Generated OTP: ");
  Serial.println(currentOTP);
  if (!sendOTPViaTelegram(currentOTP)) {
    lcd.clear();
    lcd.print("OTP send Err");
    delay(2000);
    return false;
  }
  const int maxOtpAttempts = 5;
  int otpAttempts = 0;
  while (otpAttempts < maxOtpAttempts) {
    lcd.clear();
    lcd.print("Enter OTP:");
    String enteredOTP = getKeypadInput();
    Serial.print("Entered OTP: ");
    Serial.println(enteredOTP);
    if (enteredOTP == currentOTP) {
      lcd.clear();
      lcd.print("OTP Correct");
      delay(2000);
      return true;
    } else {
      otpAttempts++;
      lcd.clear();
      lcd.print("OTP Incorrect");
      delay(2000);
    }
  }
  return false;
}

bool checkRFID() {
  if (!mfrc522.PICC_IsNewCardPresent()) return false;
  if (!mfrc522.PICC_ReadCardSerial()) return false;
  
  if (mfrc522.uid.size != sizeof(authorizedRFID)) return false;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] != authorizedRFID[i]) return false;
  }
  return true;
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return p;
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return p;
  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) return p;
  return finger.fingerID;
}

bool checkPassword() {
  const int maxPwdAttempts = 5;
  int pwdAttempts = 0;
  while (pwdAttempts < maxPwdAttempts) {
    lcd.clear();
    lcd.print("Enter Password:");
    Serial.println("Nhập mật khẩu:");
    String entered = "";
    while (entered.length() < 4) {
      char key = keypad.getKey();
      if (key != NO_KEY) {
        entered += key;
        Serial.print(key);
        lcd.setCursor(entered.length()-1, 1);
        lcd.print("*");
        delay(300);
      }
    }
    Serial.println();
    if (entered == systemPassword) {
      Serial.println("Mật khẩu đúng!");
      lcd.clear();
      lcd.print("Pwd Correct");
      delay(5000);
      lcd.print("Press button to restart");
      return true;
    } else {
      pwdAttempts++;
      Serial.print("Mật khẩu sai. Lần thử: ");
      Serial.println(pwdAttempts);
      lcd.clear();
      lcd.print("Pwd Incorrect");
      delay(2000);
    }
  }
  return false;
}
// enrollFinger, clearAllFingerprints, initFingerprintSystem, getFingerprintID, checkRFID, registerPassword, checkPassword
// GIỮ NGUYÊN như code của bạn, không thay đổi, copy/paste.

//========= HÀM securityCheck (giữ nguyên) =========
bool securityCheck() {
  delay(5000);
  if (!checkRFID()) {
    Serial.println("RFID không được cấp quyền.");
    lcd.clear();
    lcd.print("RFID Invalid");
    delay(2500);
    return false;
  }
  Serial.println("RFID đúng.");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RFID OK!");
  delay(3000);

  Serial.println("Vui lòng quét vân tay...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan Finger");
  delay(3000);

  const int maxFpAttempts = 5;
  int fpAttempts = 0;
  bool fpVerified = false;
  while (fpAttempts < maxFpAttempts) {
    uint8_t fpID = getFingerprintID();
    if (fpID == authorizedFingerID) {
      Serial.println("Xác thực vân tay thành công!");
      fpVerified = true;
      break;
    } else {
      fpAttempts++;
      Serial.print("Vân tay không khớp. Lần thử: ");
      Serial.println(fpAttempts);
      lcd.clear();
      lcd.print("Try Finger Again");
      delay(3000);
    }
  }
  if (!fpVerified) {
    Serial.println("Quá 5 lần thử vân tay, vui lòng quét lại thẻ RFID.");
    lcd.clear();
    lcd.print("Restart process");
    delay(3000);
    return false;
  }

  if (!checkPassword()) {
    Serial.println("Mật khẩu không khớp sau 5 lần thử.");
    lcd.clear();
    lcd.print("Restart process");
    delay(3000);
    return false;
  }

  if (!checkOTP()) {
    Serial.println("OTP không khớp sau 5 lần thử.");
    lcd.clear();
    lcd.print("Restart process");
    delay(3000);
    return false;
  }

  return true;
}

void registerPassword() {
  lcd.clear();
  lcd.print("Set Password:");
  Serial.println("Nhập mật khẩu (4 số):");
  String pwd = "";
  while (pwd.length() < 4) {
    char key = keypad.getKey();
    if (key != NO_KEY) {
      pwd += key;
      Serial.print(key);
      lcd.setCursor(pwd.length()-1, 1);
      lcd.print("*");
      delay(300);
    }
  }
  lcd.clear();
  lcd.print("Confirm:");
  Serial.println("\nXác nhận mật khẩu:");
  String pwdConfirm = "";
  while (pwdConfirm.length() < 4) {
    char key = keypad.getKey();
    if (key != NO_KEY) {
      pwdConfirm += key;
      Serial.print(key);
      lcd.setCursor(pwdConfirm.length()-1, 1);
      lcd.print("*");
      delay(300);
    }
  }
  if (pwd == pwdConfirm) {
    systemPassword = pwd;
    Serial.println("\nMật khẩu đăng ký thành công!");
    lcd.clear();
    lcd.print("Password Set!");
    delay(2000);
  } else {
    Serial.println("\nMật khẩu không khớp. Vui lòng thử lại.");
    lcd.clear();
    lcd.print("Mismatch! Retry");
    delay(2000);
    registerPassword();
  }
}

uint8_t enrollFinger(uint8_t id) {
  int p = 0;
  Serial.println("Vui lòng đặt ngón tay (lần 1)...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enroll FP 1/2");
  while ((p = finger.getImage()) != FINGERPRINT_OK) {
    if (p == FINGERPRINT_NOFINGER) {
      // chờ
    } else {
      Serial.print("Lỗi quét lần 1: ");
      Serial.println(p);
    }
    delay(200);
  }
  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    Serial.println("Chuyển đổi ảnh lần 1 thất bại");
    lcd.clear();
    lcd.print("Err FP 1");
    return p;
  }
  Serial.println("Nhấc ngón tay ra, đợi 2 giây...");
  lcd.clear();
  lcd.print("Remove finger");
  delay(2000);
  Serial.println("Vui lòng đặt ngón tay (lần 2)...");
  lcd.clear();
  lcd.print("Enroll FP 2/2");
  while ((p = finger.getImage()) != FINGERPRINT_OK) {
    if (p == FINGERPRINT_NOFINGER) {
      // chờ
    } else {
      Serial.print("Lỗi quét lần 2: ");
      Serial.println(p);
    }
    delay(200);
  }
  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    Serial.println("Chuyển đổi ảnh lần 2 thất bại");
    lcd.clear();
    lcd.print("Err FP 2");
    return p;
  }
  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    Serial.println("Hai lần quét không khớp");
    lcd.clear();
    lcd.print("Enroll Mismatch");
    return p;
  }
  p = finger.storeModel(id);
  if (p != FINGERPRINT_OK) {
    Serial.println("Lưu model thất bại");
    lcd.clear();
    lcd.print("Store Error");
    return p;
  }
  return FINGERPRINT_OK;
}

void initFingerprintSystem() {
  clearAllFingerprints();
  finger.getTemplateCount();
  Serial.print("Số mẫu hiện có: ");
  Serial.println(finger.templateCount);
  
  if (finger.templateCount == 0) {
    Serial.println("Chưa có mẫu, bắt đầu đăng ký vân tay...");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("No FP stored");
    delay(2000);
    while (enrollFinger(authorizedFingerID) != FINGERPRINT_OK) {
      Serial.println("Đăng ký thất bại, thử lại...");
      lcd.clear();
      lcd.print("Enroll Failed");
      delay(2000);
    }
    Serial.println("Đăng ký vân tay thành công!");
    lcd.clear();
    lcd.print("Enroll Success");
    delay(2000);
  } else {
    Serial.println("Đã có mẫu, hệ thống sẵn sàng.");
    lcd.clear();
    lcd.print("System Ready");
    delay(2000);
  }
}
void clearAllFingerprints() {
  Serial.println("Đang xóa tất cả mẫu vân tay...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Clearing FP...");
  for (uint8_t id = 1; id <= 127; id++) {
    uint8_t p = finger.deleteModel(id);
    if (p == FINGERPRINT_OK) {
      Serial.print("Đã xóa mẫu ID: ");
      Serial.println(id);
    } else {
      Serial.print("ID ");
      Serial.print(id);
      Serial.print(" xóa không được, code: 0x");
      Serial.println(p, HEX);
    }
    delay(50);
  }
  Serial.println("Xóa hết mẫu vân tay hoàn tất!");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Clear Complete");
  delay(2000);
}

//=====================================================
// SETUP: KHÔNG THAY ĐỔI CODE CŨ, CHỈ THÊM PHẦN RELAY_PIN
//=====================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Khởi tạo hệ thống bảo mật (RFID, FP, Keypad, OTP)...");

  WiFi.begin(ssid, wifiPassword);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi đã kết nối. IP: " + WiFi.localIP().toString());
  secured_client.setInsecure();

  lcd.begin();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Start...");
  delay(2000);

  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

  // THÊM MỚI: Relay pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Khóa cửa ban đầu (lock)

  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("RFID đã khởi tạo");

  fingerSerial.begin(57600, SERIAL_8N1, FINGER_RX, FINGER_TX);
  delay(100);
  finger.begin(57600);
  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor đã khởi tạo");
  } else {
    Serial.println("Không tìm thấy fingerprint sensor!");
    lcd.clear();
    lcd.print("FP sensor ERR");
    while (true) { delay(1); }
  }

  Serial.println("Keypad đã khởi tạo");

  if (systemPassword == "") {
    Serial.println("Chưa có mật khẩu. Đăng ký mật khẩu...");
    lcd.clear();
    lcd.print("Set Password");
    registerPassword();
  }

  initFingerprintSystem();

  Serial.println("Hệ thống bảo mật sẵn sàng. Quẹt thẻ RFID để xác thực...");
  lcd.clear();
  lcd.print("Scan RFID");
}

//=====================================================
// LOOP: THÊM ĐIỀU KHIỂN MỞ/ĐÓNG KHÓA SAU securityCheck
//=====================================================
void loop() {
  // Nếu 4 lớp bảo mật OK
  if (securityCheck()) {
    Serial.println("Truy cập được cấp phép!");
    lcd.clear();
    lcd.print("Access Granted");

    // Mở khóa (bật relay)
    digitalWrite(RELAY_PIN, LOW);  
    lcd.setCursor(0, 1);
    lcd.print("Door Unlocked");

    // Chờ nút reset để khởi động lại quá trình bảo mật
    Serial.println("Nhấn nút reset để bật lại hệ thống bảo mật.");
    while (digitalRead(RESET_BUTTON_PIN) == HIGH) {
      delay(100);
    }
    // Khi người dùng nhấn nút reset => quay về trạng thái ban đầu (đóng cửa)
    digitalWrite(RELAY_PIN, HIGH); 
    lcd.clear();
    lcd.print("Door Locked");
    delay(2000);

    lcd.clear();
    lcd.print("Restarting...");
    Serial.println("Reset button pressed. Restarting security process...");
    delay(1000);
  }
}
