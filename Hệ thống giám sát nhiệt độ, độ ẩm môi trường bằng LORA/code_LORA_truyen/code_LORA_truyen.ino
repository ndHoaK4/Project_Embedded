#include <DHT.h>
#include <HardwareSerial.h>

#define DHTPIN 25       // Chân kết nối với DHT
#define DHTTYPE DHT11  

DHT dht(DHTPIN, DHTTYPE);
HardwareSerial LoRaSerial(2);  // Sử dụng UART2

void setup() {
  Serial.begin(115200);
  LoRaSerial.begin(9600, SERIAL_8N1, 16, 17); // Baudrate của LoRa AS32 là 9600

  dht.begin();  // Khởi động cảm biến DHT
  delay(2000);
  Serial.println("Bắt đầu gửi dữ liệu từ cảm biến...");
}

void loop() {
  // Đọc nhiệt độ và độ ẩm
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Kiểm tra nếu có lỗi khi đọc
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Không thể đọc cảm biến DHT!");
    return;
  }

  // Chuẩn bị dữ liệu gửi đi, thêm ký tự phân cách giữa 2 giá trị
  String message = String(temperature,1) + "," + String(humidity,1);
  
  // Gửi dữ liệu qua LoRa
  LoRaSerial.println(message);
  Serial.println("Gửi tin nhắn: " + message);

  delay(2000);  // Gửi mỗi 2 giây
}
