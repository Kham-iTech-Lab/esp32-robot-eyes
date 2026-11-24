/*
  ESP32 หุ่นยนต์ตากะพริบ — จอ OLED 0.96" (SSD1306 128x64 I2C)
  K iTech Review (TikTok) · Kham iTech Lab (YouTube / Facebook)

  ต่อสาย (4 เส้น)
    OLED VCC -> 3V3   GND -> GND   SDA -> GPIO 21   SCL -> GPIO 22

  การทำงาน
    วาดตา 2 ข้างเป็นสี่เหลี่ยมมุมมน
    กะพริบ = ย่อความสูงตาลงแป๊บเดียวแล้วกลับ (บางทีกะพริบ 2 ครั้งติด)
    มองซ้าย/ขวา/ขึ้น/ลง แบบสุ่มเวลา ตาเลื่อนแบบนุ่ม ๆ ไม่กระตุก

  ไลบรารี (Library Manager): Adafruit SSD1306  (จะถามให้ติดตั้ง Adafruit GFX ด้วย กด Install All)
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------- ตั้งค่า ----------
const int SDA_PIN = 21;
const int SCL_PIN = 22;
const uint8_t OLED_ADDR = 0x3C;   // จอส่วนใหญ่ 0x3C ถ้าจอดำ ลอง 0x3D

// ขนาดตา (ตรงกับในคลิป)
const int EYE_W = 36, EYE_H = 40, EYE_R = 10;
const int LEFT_X = 18, RIGHT_X = 74, EYE_Y = 12;

const int LOOK_X = 14;   // มองซ้ายขวาได้ไกลสุดกี่พิกเซล
const int LOOK_Y = 8;    // มองขึ้นลงได้ไกลสุดกี่พิกเซล

Adafruit_SSD1306 d(128, 64, &Wire, -1);

// ---------- สถานะ ----------
float ox = 0, oy = 0;          // ตำแหน่งตาตอนนี้ (เลื่อนจากกลาง)
float tx = 0, ty = 0;          // ตำแหน่งที่อยากมองไป
float openness = 1.0;          // 1 = ลืมตา, 0 = หลับ
unsigned long nextLook = 0, nextBlink = 0, blinkStart = 0;
bool blinking = false;
int blinksLeft = 0;

const unsigned long BLINK_MS = 160;   // กะพริบ 1 ครั้งใช้เวลาเท่านี้

void drawEyes() {
  d.clearDisplay();
  int h = max(2, (int)(EYE_H * openness));
  int r = min(EYE_R, h / 2);
  int y = EYE_Y + (EYE_H - h) / 2 + (int)oy;
  d.fillRoundRect(LEFT_X + (int)ox, y, EYE_W, h, r, WHITE);
  d.fillRoundRect(RIGHT_X + (int)ox, y, EYE_W, h, r, WHITE);
  d.display();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!d.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("ไม่เจอจอ OLED เช็คสาย SDA/SCL หรือเปลี่ยน OLED_ADDR เป็น 0x3D");
    while (true) delay(1000);
  }
  randomSeed(esp_random());
  drawEyes();
  nextLook = millis() + 1500;
  nextBlink = millis() + 2000;
}

void loop() {
  unsigned long now = millis();

  // มองไปทางใหม่แบบสุ่ม
  if (now >= nextLook) {
    if (random(3) == 0) { tx = 0; ty = 0; }            // กลับมามองตรง
    else { tx = random(-LOOK_X, LOOK_X + 1); ty = random(-LOOK_Y, LOOK_Y + 1); }
    nextLook = now + random(1200, 4000);
  }

  // ถึงเวลากะพริบ
  if (!blinking && now >= nextBlink) {
    blinking = true;
    blinkStart = now;
    blinksLeft = (random(4) == 0) ? 2 : 1;             // บางทีกะพริบ 2 ครั้ง
  }
  if (blinking) {
    float p = (long)(now - blinkStart) / (float)BLINK_MS;   // 0..1 (ติดลบ = รอกะพริบรอบสอง)
    if (p >= 1) {
      openness = 1;
      if (--blinksLeft > 0) blinkStart = now + 60;     // เว้นนิดนึงแล้วกะพริบอีกรอบ
      else { blinking = false; nextBlink = now + random(2000, 6000); }
    } else if (p >= 0) {
      openness = fabs(1 - 2 * p);                      // หลับลงแล้วลืมขึ้น
    }
  }

  // เลื่อนตาเข้าหาเป้าแบบนุ่ม ๆ
  ox += (tx - ox) * 0.25;
  oy += (ty - oy) * 0.25;

  drawEyes();
  delay(16);   // ประมาณ 60 ภาพต่อวินาที
}
