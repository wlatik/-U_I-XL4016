#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // Ширина OLED дисплея в пикселях
#define SCREEN_HEIGHT 64 // Высота OLED дисплея в пикселях

// Настройка I2C адреса (обычно 0x3C или 0x3D)
#define OLED_RESET     -1 // Пина сброса нет, ставим -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Константы для математического расчета
const float V_REF = 2.50;      // Точное напряжение на TL431 (впишите свое!)
const float DELITEL_V = 10.1;  // Коэффициент делителя напряжения (9.1к + 1к) / 1к
const float GAIN_I = 48.0;     // Коэффициент усиления ОУ тока (1 + 47к/1к)
const float R_SHUNT = 0.018;   // Наш шунт R018 (0.018 Ом)

// Переменные для подсчета емкости
unsigned long lastTime = 0;
float cumulativemAh = 0.0;

void setup() {
  // АРХИВАЖНО: Включаем внешнее опорное напряжение на выводе AREF!
  analogReference(EXTERNAL); 
  
  // Инициализация OLED дисплея по адресу 0x3C
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    for(;;); // Если дисплей не найден, вешаем процессор (защита от ошибок)
  }
  
  display.clearDisplay();
  display.display();
  lastTime = millis();
}

void loop() {
  // --- 1. Считывание АЦП с усреднением (50 замеров) ---
  long rawV = 0;
  long rawI = 0;
  int samples = 50; 
  
  for (int i = 0; i < samples; i++) {
    rawV += analogRead(A0);
    rawI += analogRead(A1);
    delay(1); 
  }
  float avgRawV = (float)rawV / samples;
  float avgRawI = (float)rawI / samples;

  // --- 2. Математический пересчет в Вольты и Амперы ---
  float voltageA0 = (avgRawV * V_REF) / 1023.0;
  float realVoltage = voltageA0 * DELITEL_V;

  float voltageA1 = (avgRawI * V_REF) / 1023.0;
  float voltageShunt = voltageA1 / GAIN_I;
  float realCurrent = voltageShunt / R_SHUNT;
  
  // Отсечка микро-шумов около нуля
  if (realCurrent < 0.005) realCurrent = 0.0;
  if (realVoltage < 0.05) realVoltage = 0.0;

  // --- 3. Подсчет емкости (мАч) ---
  unsigned long currentTime = millis();
  float durationHours = (float)(currentTime - lastTime) / 3600000.0; 
  cumulativemAh += (realCurrent * 1000.0) * durationHours; 
  lastTime = currentTime;

  // --- 4. Вывод данных на OLED дисплей ---
  display.clearDisplay(); // Очищаем буфер экрана
  display.setTextColor(SSD1306_WHITE); // OLED монохромный, цвет белый

  // Строка НАПРЯЖЕНИЯ
  display.setTextSize(2);          // Крупный шрифт
  display.setCursor(0, 0);
  display.print(realVoltage, 2);   // Вывод вольт (например, 4.20)
  display.print(" V");

  // Строка ТОКА
  display.setTextSize(2);          // Крупный шрифт
  display.setCursor(0, 22);
  display.print(realCurrent, 2);   // Вывод ампер (например, 1.55)
  display.print(" A");

  // Делаем разделительную линию для красоты
  display.drawFastHLine(0, 42, 128, SSD1306_WHITE);

  // Строка ЕМКОСТИ
  display.setTextSize(1);          // Шрифт поменьше, чтобы всё влезло
  display.setCursor(0, 48);
  display.print("Capacity: ");
  display.print((int)cumulativemAh);
  display.print(" mAh");

  display.display(); // Отправляем данные из буфера на физический экран
  delay(150);        // Скорость обновления комфортна для глаз
}
