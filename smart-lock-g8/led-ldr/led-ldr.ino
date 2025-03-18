// Định nghĩa chân cho LDR và LED
#define LDR_PIN 34      // D34 dùng để đọc giá trị analog từ quang trở
#define LED_PIN 3       // Sử dụng RX0 (GPIO3) cho LED

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Khởi tạo LCD I2C với địa chỉ 0x27, 16 cột x 2 hàng
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Các biến để quản lý thời gian chuyển LED
unsigned long previousToggle = 0;
const long toggleInterval = 5000; // 5000 ms = 5 giây
bool ledState = false;

void setup() {
  // Cấu hình chân LED làm OUTPUT
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // LED tắt ban đầu

  // Khởi tạo LCD
  lcd.begin();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LDR & LED Test");
  delay(2000);
}

void loop() {
  // Sử dụng millis() để chuyển trạng thái LED mỗi 5 giây
  unsigned long currentMillis = millis();
  if (currentMillis - previousToggle >= toggleInterval) {
    previousToggle = currentMillis;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  }
  
  // Đọc giá trị từ quang trở
  int ldrValue = analogRead(LDR_PIN);
  
  // Cập nhật giá trị lên LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LDR: ");
  lcd.print(ldrValue);
  
  lcd.setCursor(0, 1);
  lcd.print("LED: ");
  lcd.print(ledState ? "ON" : "OFF");
  
  delay(500);  // Cập nhật mỗi 500ms
}
