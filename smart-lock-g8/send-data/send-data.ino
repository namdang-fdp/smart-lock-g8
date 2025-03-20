#include <WiFi.h>
#include <WiFiClientSecure.h>

// Thông tin WiFi của bạn
const char* ssid     = "OPPO Reno8";
const char* password = "d29ph9xh";
const char* host = "05fc9a3d-8f86-4d0c-9a7e-6201d2f4072d-00-mk71iqq52uxk.sisko.replit.dev";
const int httpsPort = 443;

WiFiClientSecure client;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Kết nối WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");

  // Thiết lập client cho kết nối HTTPS (bỏ qua xác thực chứng chỉ)
  client.setInsecure();
}

void loop() {
  // Đọc giá trị cảm biến ánh sáng (ví dụ chân ADC 34)
  int lightValue = analogRead(34);
  Serial.print("Light value: ");
  Serial.println(lightValue);

  // Kết nối đến server Replit qua HTTPS
  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed");
    delay(5000);
    return;
  }

  // Tạo đường dẫn GET với giá trị ánh sáng
  String url = "/iot/data?light=" + String(lightValue);
  Serial.print("Requesting URL: ");
  Serial.println(url);

  // Gửi request HTTP GET
  client.println("GET " + url + " HTTP/1.1");
  client.println("Host: " + String(host));
  client.println("Connection: close");
  client.println(); // Kết thúc header

  // Đọc phản hồi (optional)
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }
  String response = client.readString();
  Serial.println("Response: ");
  Serial.println(response);

  // Đóng kết nối
  client.stop();

  // Gửi dữ liệu mỗi 5 giây (có thể thay đổi theo nhu cầu)
  delay(5000);
}
