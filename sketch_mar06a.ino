#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

//======================
// CẤU HÌNH LCD I2C
//======================
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Địa chỉ I2C có thể là 0x27 hoặc 0x3F

//======================
// CẤU HÌNH RFID RC522
//======================
#define RST_PIN 2     // RST nối GPIO2
#define SS_PIN  5     // SDA (SS) nối GPIO5
MFRC522 mfrc522(SS_PIN, RST_PIN);

// UID của thẻ RFID được cấp phép: 4A:DA:36:02
byte authorizedRFID[] = { 0x4A, 0xDA, 0x36, 0x02 };

//======================
// CẤU HÌNH FINGERPRINT AS608
//======================
#define FINGER_RX 16  // Nối TX của cảm biến
#define FINGER_TX 17  // Nối RX của cảm biến
HardwareSerial fingerSerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSerial);

// Slot vân tay được dùng để xác thực (ID = 1)
uint8_t authorizedFingerID = 1;

//======================
// CẤU HÌNH NÚT RESET
//======================
#define RESET_BUTTON_PIN 12  // Nút reset nối vào GPIO12

//======================
// CẤU HÌNH KEYPAD 4x4
//======================
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {13, 14, 27, 26};  // Ví dụ: các chân cho hàng
byte colPins[COLS] = {15, 25, 32, 33};   // Ví dụ: các chân cho cột
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

//======================
// Lưu mật khẩu hệ thống (password đăng ký)
//======================
String systemPassword = "";  // Nếu rỗng, chưa đăng ký

//=====================================================
// Hàm enrollFinger: đăng ký vân tay và lưu vào slot id
//=====================================================
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

//=====================================================
// Hàm clearAllFingerprints: xóa tất cả mẫu vân tay (ID 1-127)
//=====================================================
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
// Hàm initFingerprintSystem: xóa mẫu cũ và nếu chưa có mẫu thì enroll mới
//=====================================================
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

//=====================================================
// Hàm getFingerprintID: quét và xác thực vân tay, trả về ID nếu thành công, -1 nếu thất bại
//=====================================================
uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return p;
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return p;
  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) return p;
  return finger.fingerID;
}

//=====================================================
// Hàm checkRFID: kiểm tra xem thẻ RFID có đúng không
//=====================================================
bool checkRFID() {
  if (!mfrc522.PICC_IsNewCardPresent()) return false;
  if (!mfrc522.PICC_ReadCardSerial()) return false;
  
  // So sánh UID
  if (mfrc522.uid.size != sizeof(authorizedRFID)) return false;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] != authorizedRFID[i]) return false;
  }
  return true;
}

//=====================================================
// Hàm registerPassword: đăng ký mật khẩu lần đầu qua Keypad
//=====================================================
void registerPassword() {
  lcd.clear();
  lcd.print("Set Password:");
  Serial.println("Vui lòng nhập mật khẩu (4 số):");
  String pwd = "";
  while (pwd.length() < 4) {
    char key = keypad.getKey();
    if (key != NO_KEY) {
      pwd += key;
      Serial.print(key);  // in ra Serial để debug
      lcd.setCursor(pwd.length()-1, 1);
      lcd.print("*");     // hiển thị dấu *
      delay(300);
    }
  }
  // Yêu cầu xác nhận mật khẩu
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

//=====================================================
// Hàm checkPassword: xác thực mật khẩu qua Keypad
// Cho phép 5 lần thử, nếu sai quá 5 lần thì trả về false.
//=====================================================
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
      lcd.print("Access Granted");
      delay(2000);
      return true;
    } else {
      pwdAttempts++;
      Serial.print("Mật khẩu sai. Lần thử: ");
      Serial.println(pwdAttempts);
      lcd.clear();
      lcd.print("Invalid Password");
      delay(2000);
    }
  }
  return false;
}

//=====================================================
// Hàm securityCheck: lớp bảo mật gồm RFID, Fingerprint và Keypad Password
//=====================================================
bool securityCheck() {
  // Kiểm tra thẻ RFID
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
  
  // Kiểm tra vân tay
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
  
  // Kiểm tra mật khẩu qua Keypad
  if (!checkPassword()) {
    Serial.println("Mật khẩu không khớp sau 5 lần thử.");
    lcd.clear();
    lcd.print("Restart process");
    delay(3000);
    return false;
  }
  
  return true;
}

//=====================================================
// SETUP
//=====================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Khởi tạo hệ thống bảo mật (RFID, FP, Keypad)...");
  
  // Khởi tạo LCD sử dụng lcd.begin()
  lcd.begin();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Start...");
  delay(2000);

  // Cấu hình nút reset (button) nối vào GPIO12 với nội bộ pull-up
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

  // Khởi tạo RFID RC522 qua SPI
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("RFID đã khởi tạo");

  // Khởi tạo fingerprint sensor
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
  
  // Khởi tạo Keypad (thư viện Keypad tự khởi tạo qua đối tượng)
  Serial.println("Keypad đã khởi tạo");

  // Đăng ký mật khẩu nếu lần đầu (nếu systemPassword rỗng)
  if (systemPassword == "") {
    Serial.println("Chưa có mật khẩu. Đăng ký mật khẩu...");
    lcd.clear();
    lcd.print("Set Password");
    registerPassword();
  }
  
  // Khởi tạo hệ thống vân tay: xóa và enroll nếu cần
  initFingerprintSystem();
  
  Serial.println("Hệ thống bảo mật sẵn sàng. Quẹt thẻ RFID để xác thực...");
  lcd.clear();
  lcd.print("Scan RFID");
}

//=====================================================
// LOOP
//=====================================================
void loop() {
  // Kiểm tra và thực hiện lớp bảo mật: RFID, FP, và Keypad
  if (securityCheck()) {
    Serial.println("Truy cập được cấp phép!");
    lcd.clear();
    lcd.print("Access Granted");
    // Sau khi xác thực thành công, hệ thống không kiểm tra thêm
    while (true) {
      // Nếu muốn khởi động lại hệ thống, chờ nút reset được nhấn
      if (digitalRead(RESET_BUTTON_PIN) == LOW) {
        Serial.println("Reset button pressed. Restarting security process...");
        lcd.clear();
        lcd.print("Restarting...");
        delay(1000);
        break;
      }
      delay(100);
    }
  }
}
