/*
    ESP32 WiFi 天气时钟（彩色屏版）

    功能：
    1. 连接 WiFi（SSID/密码见 wifi_config.h，不上传到 Git）
    2. 从聚合数据 API 获取北京实时温度/湿度（每 5 分钟刷新一次）
    3. NTP 同步当前时间（北京时间 UTC+8）
    4. T147B7-H12-11-V1 (1.47寸 172x320 ST7789V) 彩屏显示
       - 页面0：日期 + 大字时间 + 温湿度一行
       - 页面1（回滚页）：大字温度 / 大字湿度
       - 每 5 秒自动来回切换（回滚显示）

    显示屏：T147B7-H12-11-V1，驱动 IC: ST7789V，172(RGB)x320，杜邦线连接
    使用 ESP32 硬件 SPI (VSPI)，参考 ADC_TempSensor-4bit/User/lcd.c 的初始化序列

    接线（杜邦线，如引脚被占用可自行更换下方宏定义）：
      LCD       ->  ESP32
      VCC 3.3V  ->  3.3V
      GND       ->  GND
      SCL(SCLK) ->  GPIO18 (VSPI SCK)
      SDA(MOSI) ->  GPIO23 (VSPI MOSI)
      CS        ->  GPIO5
      DC        ->  GPIO27
      RES(RST)  ->  GPIO26
      BLK(背光) ->  GPIO25
*/

#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <SPI.h>
#include "font.h"
#include "wifi_config.h"   // 私密配置（WiFi/API Key），见 wifi_config_example.h

/* ==================== WiFi 配置 ==================== */
const char *ssid     = WIFI_SSID;
const char *password = WIFI_PASSWORD;

/* ==================== LCD 引脚配置 ==================== */
// 硬件 SPI：SCLK=18, MOSI=23 固定；其余控制脚可按杜邦线实际连接修改
#define LCD_SCLK   18
#define LCD_MOSI   23
#define LCD_CS     5
#define LCD_DC     27
#define LCD_RST    26
#define LCD_BLK    25

/* ==================== 屏幕参数 (ST7789V) ==================== */
// 横屏模式：320 x 172
#define LCD_W      320
#define LCD_H      172
#define LCD_ROW_OFFSET  34   // ST7789 172x320 面板的行偏移

/* ==================== 颜色定义 (RGB565) ==================== */
#define C_BLACK   0x0000
#define C_WHITE   0xFFFF
#define C_RED     0xF800
#define C_GREEN   0x07E0
#define C_BLUE    0x001F
#define C_CYAN    0x07FF
#define C_YELLOW  0xFFE0
#define C_GRAY    0x8410

/* ==================== 天气数据 ==================== */
String url = WEATHER_URL;
const unsigned long WEATHER_INTERVAL = 300000UL;  // 5 分钟刷新天气
const unsigned long PAGE_INTERVAL    = 5000UL;    // 5 秒回滚切换页面

int  Temp = -99;
int  Humi = -99;
String City = "BeiJing";

char Temp_Char[16];
char Humi_Char[16];
char Date_N[24];
char Time_N[16];
char Week_N[12];

/* ==================== NTP ==================== */
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp.aliyun.com", 28800);  // 北京时间 UTC+8

const char *WEEK_STR[] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};

/* ================================================================
 *                     ST7789V 底层驱动 (硬件SPI)
 * ================================================================ */

inline void LCD_CS_Low()  { digitalWrite(LCD_CS, LOW);  }
inline void LCD_CS_High() { digitalWrite(LCD_CS, HIGH); }
inline void LCD_DC_Low()  { digitalWrite(LCD_DC, LOW);  }
inline void LCD_DC_High() { digitalWrite(LCD_DC, HIGH); }

// 写一个字节到 SPI 总线
void LCD_Writ_Bus(uint8_t dat)
{
  SPI.transfer(dat);
}

// 写 8 位数据
void LCD_WR_DATA8(uint8_t dat)
{
  LCD_CS_Low();
  LCD_DC_High();
  SPI.transfer(dat);
  LCD_CS_High();
}

// 写 16 位数据 (RGB565 颜色)
void LCD_WR_DATA(uint16_t dat)
{
  LCD_CS_Low();
  LCD_DC_High();
  SPI.transfer16(dat);
  LCD_CS_High();
}

// 写命令
void LCD_WR_REG(uint8_t dat)
{
  LCD_CS_Low();
  LCD_DC_Low();
  SPI.transfer(dat);
  LCD_DC_High();
  LCD_CS_High();
}

// 设置显示窗口 (含横屏偏移)
void LCD_Address_Set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
  LCD_WR_REG(0x2A);                       // 列地址设置
  LCD_WR_DATA8(x1 >> 8);
  LCD_WR_DATA8(x1 & 0xFF);
  LCD_WR_DATA8(x2 >> 8);
  LCD_WR_DATA8(x2 & 0xFF);
  LCD_WR_REG(0x2B);                       // 行地址设置
  LCD_WR_DATA8((y1 + LCD_ROW_OFFSET) >> 8);
  LCD_WR_DATA8((y1 + LCD_ROW_OFFSET) & 0xFF);
  LCD_WR_DATA8((y2 + LCD_ROW_OFFSET) >> 8);
  LCD_WR_DATA8((y2 + LCD_ROW_OFFSET) & 0xFF);
  LCD_WR_REG(0x2C);                       // 开始写入GRAM
}

void LCD_Init(void)
{
  pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_DC, OUTPUT);
  pinMode(LCD_RST, OUTPUT);
  pinMode(LCD_BLK, OUTPUT);

  SPI.begin(LCD_SCLK, -1, LCD_MOSI, LCD_CS);   // 硬件 VSPI
  SPI.setFrequency(40000000);                  // 40MHz
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);

  digitalWrite(LCD_CS, HIGH);
  LCD_DC_High();

  // 复位
  digitalWrite(LCD_RST, LOW);
  delay(100);
  digitalWrite(LCD_RST, HIGH);
  delay(100);

  // 打开背光
  digitalWrite(LCD_BLK, HIGH);
  delay(100);

  // ST7789 初始化序列 (参考 ADC_TempSensor-4bit/User/lcd.c)
  LCD_WR_REG(0x11);  delay(120);            // Sleep Out
  LCD_WR_REG(0x36);
  LCD_WR_DATA8(0x70);                       // MADCTL: 横屏 RGB

  LCD_WR_REG(0x3A);
  LCD_WR_DATA8(0x05);                       // 16bit RGB565

  LCD_WR_REG(0xB2);
  LCD_WR_DATA8(0x0C);
  LCD_WR_DATA8(0x0C);
  LCD_WR_DATA8(0x00);
  LCD_WR_DATA8(0x33);
  LCD_WR_DATA8(0x33);

  LCD_WR_REG(0xB7);  LCD_WR_DATA8(0x35);    // Gate 控制
  LCD_WR_REG(0xBB);  LCD_WR_DATA8(0x35);    // VCOM
  LCD_WR_REG(0xC0);  LCD_WR_DATA8(0x2C);    // LCM 控制
  LCD_WR_REG(0xC2);  LCD_WR_DATA8(0x01);
  LCD_WR_REG(0xC3);  LCD_WR_DATA8(0x13);    // VRHV
  LCD_WR_REG(0xC4);  LCD_WR_DATA8(0x20);    // VDV
  LCD_WR_REG(0xC6);  LCD_WR_DATA8(0x0F);    // 帧率 60Hz

  LCD_WR_REG(0xD0);
  LCD_WR_DATA8(0xA4);
  LCD_WR_DATA8(0xA1);

  LCD_WR_REG(0xD6);  LCD_WR_DATA8(0xA1);

  // Gamma 校正 (Positive)
  LCD_WR_REG(0xE0);
  LCD_WR_DATA8(0xF0); LCD_WR_DATA8(0x00); LCD_WR_DATA8(0x04); LCD_WR_DATA8(0x04);
  LCD_WR_DATA8(0x04); LCD_WR_DATA8(0x05); LCD_WR_DATA8(0x29); LCD_WR_DATA8(0x33);
  LCD_WR_DATA8(0x3E); LCD_WR_DATA8(0x38); LCD_WR_DATA8(0x12); LCD_WR_DATA8(0x12);
  LCD_WR_DATA8(0x28); LCD_WR_DATA8(0x30);

  // Gamma 校正 (Negative)
  LCD_WR_REG(0xE1);
  LCD_WR_DATA8(0xF0); LCD_WR_DATA8(0x07); LCD_WR_DATA8(0x0A); LCD_WR_DATA8(0x0D);
  LCD_WR_DATA8(0x0B); LCD_WR_DATA8(0x07); LCD_WR_DATA8(0x28); LCD_WR_DATA8(0x33);
  LCD_WR_DATA8(0x3E); LCD_WR_DATA8(0x36); LCD_WR_DATA8(0x14); LCD_WR_DATA8(0x14);
  LCD_WR_DATA8(0x29); LCD_WR_DATA8(0x32);

  LCD_WR_REG(0x21);                          // 反显开启 (IPS 屏)
  LCD_WR_REG(0x11);  delay(120);             // Sleep Out
  LCD_WR_REG(0x29);                          // Display On
}

// 矩形区域填充
void LCD_Fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
  if (w == 0 || h == 0) return;
  LCD_Address_Set(x, y, x + w - 1, y + h - 1);
  LCD_CS_Low();
  LCD_DC_High();
  uint32_t n = (uint32_t)w * h;
  while (n--) {
    SPI.transfer16(color);
  }
  LCD_CS_High();
}

// 全屏清屏
void LCD_Clear(uint16_t color)
{
  LCD_Fill(0, 0, LCD_W, LCD_H, color);
}

// 画横线
void LCD_DrawHLine(uint16_t x, uint16_t y, uint16_t len, uint16_t color)
{
  LCD_Fill(x, y, len, 1, color);
}

/* ================================================================
 *                          文字显示
 * 取模方式：逐列式 (与 font.h 中 asc2_xxxx 一致)
 * size: 12 / 16 / 24，scale: 放大倍数(1~4)
 * ================================================================ */
#define CHAR_BUF_W  (12 * 4)   // 24号字最大宽12px，最大4倍放大

void LCD_ShowChar(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t scale, uint16_t color)
{
  if (chr < ' ' || chr > '~') chr = '?';
  uint16_t idx = chr - ' ';

  const uint8_t *font;
  if (size == 12)      font = asc2_1206[idx];
  else if (size == 16) font = asc2_1608[idx];
  else if (size == 24) font = asc2_2412[idx];
  else return;

  uint8_t  fw  = size / 2;                                // 字体像素宽
  uint8_t  bpc = (size / 8 + ((size % 8) ? 1 : 0));       // 每列字节数
  uint16_t cw  = fw * scale;                              // 实际字符宽
  uint16_t ch  = size * scale;                            // 实际字符高

  uint8_t lineBuf[CHAR_BUF_W * 2];

  LCD_Address_Set(x, y, x + cw - 1, y + ch - 1);
  LCD_CS_Low();
  LCD_DC_High();
  for (uint16_t r = 0; r < ch; r++) {
    uint16_t n = 0;
    for (uint16_t c = 0; c < cw; c++) {
      uint8_t col = c / scale;
      uint8_t row = r / scale;
      uint8_t dat = font[col * bpc + row / 8];
      uint16_t px = (dat & (0x80 >> (row % 8))) ? color : C_BLACK;
      lineBuf[n++] = px >> 8;
      lineBuf[n++] = px & 0xFF;
    }
    SPI.writeBytes(lineBuf, n);
  }
  LCD_CS_High();
}

// 显示字符串
void LCD_ShowString(uint16_t x, uint16_t y, const char *str, uint8_t size, uint8_t scale, uint16_t color)
{
  uint16_t cw = (size / 2) * scale;
  while (*str) {
    LCD_ShowChar(x, y, *str, size, scale, color);
    x += cw;
    str++;
  }
}

/* ================================================================
 *                        业务逻辑
 * ================================================================ */

void fetchWeather(void)
{
  HTTPClient http;
  http.begin(url);
  int http_code = http.GET();

  Serial.printf("Http 状态码 %d\r\n", http_code);

  if (http_code == 200) {
    String response = http.getString();
    Serial.print("响应数据: ");
    Serial.println(response);

    DynamicJsonDocument doc(2048);
    DeserializationError err = deserializeJson(doc, response);
    if (!err) {
      int t = doc["result"]["realtime"]["temperature"].as<int>();
      int h = doc["result"]["realtime"]["humidity"].as<int>();
      String info = doc["result"]["realtime"]["info"].as<String>();
      if (t != 0 || h != 0) {
        Temp = t;
        Humi = h;
      }
      Serial.printf("City:%s Temp:%d Humi:%d Info:%s\r\n",
                    City.c_str(), Temp, Humi, info.c_str());
    } else {
      Serial.printf("JSON 解析失败: %s\r\n", err.c_str());
    }
  } else {
    Serial.println("天气请求失败");
  }
  http.end();
}

// 画分隔线上的页面公共元素（顶部城市 + 状态条）
void drawPageMain(void)
{
  LCD_Clear(C_BLACK);

  // 顶部：城市 + 日期 + 星期
  unsigned long epoch = timeClient.getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epoch);
  int wday = ((epoch / 86400UL) + 4) % 7;   // 1970-01-01 是周四

  sprintf(Date_N, "%04d-%02d-%02d %s",
          ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, WEEK_STR[wday]);

  LCD_ShowString(8, 8, City.c_str(), 16, 1, C_CYAN);
  LCD_ShowString(8, 32, Date_N, 16, 1, C_GRAY);

  // 中间：大字时间 HH:MM:SS  (24号字2倍放大: 高48 宽24)
  sprintf(Time_N, "%02d:%02d:%02d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
  uint16_t tw = 8 * 24;                       // 8字符 x 24px
  LCD_ShowString((LCD_W - tw) / 2, 62, Time_N, 24, 2, C_YELLOW);

  // 分隔线
  LCD_DrawHLine(8, 124, LCD_W - 16, C_GRAY);

  // 底部：温度(红) + 湿度(青)
  if (Temp == -99) sprintf(Temp_Char, "Temp:--C");
  else             sprintf(Temp_Char, "Temp:%dC", Temp);
  if (Humi == -99) sprintf(Humi_Char, "Humi:--%");
  else             sprintf(Humi_Char, "Humi:%d%%", Humi);

  LCD_ShowString(8, 134, Temp_Char, 16, 2, C_RED);
  LCD_ShowString(176, 134, Humi_Char, 16, 2, C_CYAN);
}

// 回滚页：大字温度 / 大字湿度
void drawPageRoll(void)
{
  LCD_Clear(C_BLACK);

  LCD_ShowString(8, 6, "Temperature", 16, 1, C_GRAY);
  if (Temp == -99) sprintf(Temp_Char, "-- C");
  else             sprintf(Temp_Char, "%d C", Temp);
  // 24号字2倍放大: 高48 宽24/字符
  LCD_ShowString(16, 26, Temp_Char, 24, 2, C_RED);

  LCD_ShowString(8, 96, "Humidity", 16, 1, C_GRAY);
  if (Humi == -99) sprintf(Humi_Char, "-- %");
  else             sprintf(Humi_Char, "%d %%", Humi);
  LCD_ShowString(16, 116, Humi_Char, 24, 2, C_CYAN);
}

void connectWiFi(void)
{
  Serial.println();
  Serial.println("******************************************************");
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000UL) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi 连接失败，稍后重试");
  }
}

void setup() {
  Serial.begin(115200);

  LCD_Init();
  LCD_Clear(C_BLACK);
  LCD_ShowString(60, 70, "WiFi Connecting...", 16, 1, C_WHITE);

  connectWiFi();

  timeClient.begin();

  LCD_Clear(C_BLACK);
  LCD_ShowString(60, 70, "Fetching weather...", 16, 1, C_WHITE);
  fetchWeather();

  drawPageMain();
}

void loop() {
  timeClient.update();

  static unsigned long lastWeather = 0;
  static unsigned long lastPageFlip = 0;
  static unsigned long lastSecond  = 0;
  static uint8_t page = 0;

  // WiFi 断线重连
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  // 每 5 分钟刷新天气
  if (millis() - lastWeather >= WEATHER_INTERVAL) {
    lastWeather = millis();
    fetchWeather();
  }

  // 每 5 秒回滚切换页面
  if (millis() - lastPageFlip >= PAGE_INTERVAL) {
    lastPageFlip = millis();
    page ^= 1;
    lastSecond = millis();     // 切页时立即重绘
    if (page) drawPageRoll();
    else      drawPageMain();
  }

  // 每秒刷新一次当前页面（时钟走秒）
  if (millis() - lastSecond >= 1000) {
    lastSecond = millis();
    if (page) drawPageRoll();
    else      drawPageMain();
  }

  delay(50);
}
