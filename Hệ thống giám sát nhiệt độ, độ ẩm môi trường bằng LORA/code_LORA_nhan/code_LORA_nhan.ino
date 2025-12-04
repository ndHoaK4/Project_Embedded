#include <HardwareSerial.h>
HardwareSerial LoRaSerial(2);  // Sử dụng UART2

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>

#include <Wire.h>
#include <RTClib.h>
#include <EEPROM.h>  // Thư viện EEPROM để lưu giá trị vào bộ nhớ

RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sun", "Mon", "Tue", "Wed", "Thur", "Fri", "Sat"};

#define TFT_CS         5
#define TFT_RST        4                                            
#define TFT_DC         2
#define TFT_SCLK       18
#define TFT_MOSI       23 
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

const int button1Pin = 15;  // Nút nhấn tăng giá trị
const int button2Pin = 26;  // Nút nhấn giảm giá trị
const int button3Pin = 27;  // Nút nhấn chuyển đổi màn hình và lưu giá trị
const int ledPin = 32;      // Đèn LED
const int buzzerPin = 33;   // Còi báo

float temperatureWarning = 0;  // Ngưỡng nhiệt độ cảnh báo
float humidityWarning = 0;     // Ngưỡng độ ẩm cảnh báo
float step = 0.5;              // Bước tăng/giảm giá trị
const int EEPROM_TEMP_ADDR = 0;  // Địa chỉ lưu nhiệt độ trong EEPROM
const int EEPROM_HUMI_ADDR = 10; // Địa chỉ lưu độ ẩm trong EEPROM

unsigned long prevTime = millis();
unsigned long lastButtonPress = 0;
unsigned long debounceDelay = 200;
int screenState = 0;  // 0: Màn hình chính, 1: Điều chỉnh nhiệt độ, 2: Điều chỉnh độ ẩm

static const uint8_t bitmap_Temp [] = {
	0x00, 0x06, 0x0f, 0x00, 0x00, 0x06, 0x1f, 0x80, 0x00, 0x06, 0x30, 0xc0, 0x02, 0x00, 0x20, 0x40, 
	0x07, 0x00, 0x20, 0x40, 0x03, 0x0f, 0x26, 0x40, 0x00, 0x3f, 0xe6, 0x40, 0x00, 0x70, 0x66, 0x40, 
	0x00, 0xc0, 0x26, 0x40, 0x00, 0xc0, 0x26, 0x40, 0x01, 0x80, 0x26, 0x40, 0x39, 0x80, 0x26, 0x40, 
	0x39, 0x80, 0x26, 0x40, 0x01, 0x80, 0x26, 0x40, 0x00, 0xc0, 0x26, 0x40, 0x00, 0xc0, 0x26, 0x40, 
	0x00, 0x70, 0x66, 0x40, 0x00, 0x3f, 0xe6, 0x60, 0x01, 0x1f, 0xe6, 0x60, 0x07, 0x00, 0xcf, 0x30, 
	0x06, 0x00, 0xcf, 0x30, 0x00, 0x06, 0xd9, 0xb0, 0x00, 0x06, 0xcf, 0xb0, 0x00, 0x06, 0xcf, 0x30, 
	0x00, 0x00, 0x60, 0x60, 0x00, 0x00, 0x70, 0xe0, 0x00, 0x00, 0x1f, 0xc0, 0x00, 0x00, 0x0f, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t bitmap_Humi [] = {
	0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x11, 0x00, 
	0x00, 0x10, 0x11, 0x80, 0x00, 0x30, 0x20, 0x80, 0x00, 0x28, 0x20, 0x80, 0x00, 0x4c, 0x40, 0x40, 
	0x00, 0x44, 0x42, 0x40, 0x00, 0x86, 0x42, 0x40, 0x01, 0x82, 0x60, 0x40, 0x01, 0x01, 0x20, 0x80, 
	0x02, 0x01, 0x1f, 0x00, 0x02, 0x00, 0x80, 0x00, 0x06, 0x00, 0x80, 0x00, 0x04, 0x00, 0x40, 0x00, 
	0x0c, 0x00, 0x40, 0x00, 0x08, 0x00, 0x60, 0x00, 0x08, 0x00, 0x20, 0x00, 0x08, 0x00, 0x20, 0x00, 
	0x08, 0x02, 0x20, 0x00, 0x08, 0x06, 0x20, 0x00, 0x08, 0x0c, 0x60, 0x00, 0x0c, 0x08, 0x40, 0x00, 
	0x06, 0x00, 0xc0, 0x00, 0x03, 0x01, 0x80, 0x00, 0x01, 0xee, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


void setup() {
  Serial.begin(115200);
  LoRaSerial.begin(9600, SERIAL_8N1, 16, 17); // Baudrate của LoRa AS32 là 9600
  Serial.println("Chờ nhận dữ liệu LoRa...");

  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);
  pinMode(button3Pin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  digitalWrite(buzzerPin, LOW);

  // Đọc giá trị ngưỡng cảnh báo từ EEPROM
  EEPROM.begin(512);
  EEPROM.get(EEPROM_TEMP_ADDR, temperatureWarning);
  EEPROM.get(EEPROM_HUMI_ADDR, humidityWarning);
  EEPROM.commit();

  Wire.begin(21, 22); // SDA = GPIO 21, SCL = GPIO 22
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  // rtc.adjust(DateTime(__DATE__, __TIME__));

  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST7735_BLACK);
  tft.setRotation(3);
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - prevTime > 1000) {
    if (screenState == 0) {
      updateTime();
      updateWeather();
    }
    prevTime = currentTime;
  }

  checkButtons();  // Kiểm tra các nút nhấn
}

void checkButtons() {
  if (digitalRead(button3Pin) == LOW && millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis();
    screenState++;
    if (screenState > 2) {
      screenState = 0;  // Quay lại màn hình chính
    }

    if (screenState == 1) {
      // Xóa màn hình và chuyển sang màn hình điều chỉnh nhiệt độ
      tft.fillScreen(ST7735_BLACK);
      tft.setCursor(20, 30);
      tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
      tft.setTextSize(2);
      tft.println("Set Temp:");
      displayTemperatureWarning();
    } else if (screenState == 2) {
      // Xóa màn hình và chuyển sang màn hình điều chỉnh độ ẩm
      tft.fillScreen(ST7735_BLACK);
      tft.setCursor(20, 30);
      tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
      tft.setTextSize(2);
      tft.println("Set Humi:");
      displayHumidityWarning();
    } else if (screenState == 0) {
      // Lưu ngưỡng nhiệt độ và độ ẩm vào EEPROM và quay lại màn hình chính
      EEPROM.put(EEPROM_TEMP_ADDR, temperatureWarning);
      EEPROM.put(EEPROM_HUMI_ADDR, humidityWarning);
      EEPROM.commit();
      Serial.print("Temperature and humidity warning saved: ");
      Serial.print(temperatureWarning);
      Serial.print(" C, ");
      Serial.println(humidityWarning);
      tft.fillScreen(ST7735_BLACK);
      Serial.println("Back to main screen");
    }
  }

  if (screenState == 1 && millis() - lastButtonPress > debounceDelay) {
    // Điều chỉnh ngưỡng nhiệt độ
    if (digitalRead(button1Pin) == LOW) {
      temperatureWarning += step;
      displayTemperatureWarning();
      lastButtonPress = millis();
    } else if (digitalRead(button2Pin) == LOW) {
      temperatureWarning -= step;
      displayTemperatureWarning();
      lastButtonPress = millis();
    }
  }
  if (screenState == 2 && millis() - lastButtonPress > debounceDelay) {
    // Điều chỉnh ngưỡng độ ẩm
    if (digitalRead(button1Pin) == LOW) {
      humidityWarning += step;
      displayHumidityWarning();
      lastButtonPress = millis();
    } else if (digitalRead(button2Pin) == LOW) {
      humidityWarning -= step;
      displayHumidityWarning();
      lastButtonPress = millis();
    }
  }
}

void updateTime() {
  DateTime now = rtc.now();
  tft.setCursor(13, 10);
  tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  tft.setTextSize(1);
  String thu_ = daysOfTheWeek[now.dayOfTheWeek()];
  tft.println(thu_);

  tft.setCursor(58, 10);
  tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  tft.setTextSize(1);
  String currentDate = String(now.day()) + "-" + String(now.month()) + "-" + String(now.year());
  tft.println(currentDate);

  tft.drawRect(12, 24, 106, 28, ST77XX_MAGENTA); // x, y, width, height
  tft.setCursor(18, 31);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setTextSize(2);
  String formattedTime = 
    (now.hour() < 10 ? "0" + String(now.hour()) : String(now.hour())) + ":" +
    (now.minute() < 10 ? "0" + String(now.minute()) : String(now.minute())) + ":" +
    (now.second() < 10 ? "0" + String(now.second()) : String(now.second()));

  tft.print(formattedTime);

}

void updateWeather() {
  if (LoRaSerial.available()) {
    String receivedMessage = LoRaSerial.readString();
    int commaIndex = receivedMessage.indexOf(',');
    if (commaIndex != -1) {
      String receivedTempStr = receivedMessage.substring(0, commaIndex);
      String receivedHumStr = receivedMessage.substring(commaIndex + 1);

      // Chuyển đổi từ String sang float
      float receivedTemp = receivedTempStr.toFloat();
      float receivedHum = receivedHumStr.toFloat();

      tft.drawBitmap(12, 60, bitmap_Temp, 30, 30, ST7735_BLUE);
      tft.setCursor(46, 67);
      tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
      tft.setTextSize(2);
      tft.print(receivedTemp);
      printText("o", 105, 65, 1, ST77XX_CYAN);
      printText("C", 110, 67, 2, ST77XX_CYAN);

      tft.drawBitmap(12, 96, bitmap_Humi, 30, 30, ST7735_YELLOW);
      tft.setCursor(46, 103);
      tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
      tft.setTextSize(2);
      tft.print(receivedHum);
      printText("%", 94, 103, 2, ST77XX_CYAN);

      // Kiểm tra nếu vượt ngưỡng cảnh báo, đồng thời tránh trường hợp nhận giá trị rỗng hoặc không hợp lệ
      if (receivedTemp > temperatureWarning || receivedHum > humidityWarning) {
        digitalWrite(ledPin, HIGH);
        tone(buzzerPin, 3000);  // Kích hoạt còi báo với tần số 3000Hz
      } else {
        digitalWrite(ledPin, LOW);
        noTone(buzzerPin);  // Tắt còi báo
      }
    }
  }
}


void displayTemperatureWarning() {
  tft.setCursor(20, 50);
  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.print(temperatureWarning);
  tft.println(" C");
}

void displayHumidityWarning() {
  tft.setCursor(20, 50);
  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.print(humidityWarning);
  tft.println(" %");
}

void printText(String text, int x, int y, int text_size, uint16_t color) {
  tft.setCursor(x, y);
  tft.setTextColor(color, ST77XX_BLACK);
  tft.setTextSize(text_size);
  tft.println(text);
}
