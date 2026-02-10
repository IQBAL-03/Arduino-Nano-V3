#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "WiFiEsp.h"
#include <SoftwareSerial.h>

#define PIN_ON      11   
#define PIN_OFF     12   
#define PIN_START   9    
#define PIN_RESET   10   
#define PIN_BUZZER  8    
#define TRIG_KIRI   2
#define ECHO_KIRI   3
#define TRIG_KANAN  4
#define ECHO_KANAN  5

char ssid[] = "A";             
char pass[] = "16792020Emol"; 
char server[] = "10.158.183.217"; 

LiquidCrystal_I2C lcd(0x27, 20, 4);
SoftwareSerial espSerial(6, 7); 
WiFiEspClient client;

int status = WL_IDLE_STATUS;
int skorKiri = 0, skorKanan = 0;
unsigned long waktuMulai;
unsigned long durasiGame = 60000; 
String currentCommand = "idle";
bool systemActive = true; 

void tengah(String teks, int baris) {
  int l = teks.length();
  int pos = (20 - l) / 2;
  if (pos < 0) pos = 0;
  lcd.setCursor(pos, baris);
  lcd.print(teks);
}

void bip(int d) { 
  digitalWrite(PIN_BUZZER, LOW); 
  delay(d); 
  digitalWrite(PIN_BUZZER, HIGH); 
}

void connectWiFi() {
  status = WL_IDLE_STATUS;
  while (status != WL_CONNECTED) {
    lcd.clear();
    tengah("CONNECTING WiFi", 1);
    status = WiFi.begin(ssid, pass);
    
    if (status == WL_CONNECTED) {
      lcd.clear();
      tengah("WiFi Connected!", 1);
      delay(1500);
    } else {
      tengah("Retrying...", 3);
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(9600); 
  espSerial.begin(9600); 
  
  pinMode(PIN_BUZZER, OUTPUT); digitalWrite(PIN_BUZZER, HIGH);
  pinMode(PIN_START, INPUT);
  pinMode(PIN_RESET, INPUT);
  pinMode(PIN_ON, INPUT);
  pinMode(PIN_OFF, INPUT);
  
  pinMode(TRIG_KIRI, OUTPUT); pinMode(ECHO_KIRI, INPUT);
  pinMode(TRIG_KANAN, OUTPUT); pinMode(ECHO_KANAN, INPUT);
  
  lcd.init(); lcd.backlight();
  tengah("INITIALIZING...", 1);
  
  WiFi.init(&espSerial);
  if (WiFi.status() == WL_NO_SHIELD) {
    lcd.clear(); tengah("ESP ERROR!", 1);
    while (true);
  }
  connectWiFi();
}

void loop() {
  if (!systemActive) {
    lcd.clear();
    lcd.noBacklight(); 
    while (!systemActive) {
      if (digitalRead(PIN_ON) == HIGH) {
        delay(50);
        if (digitalRead(PIN_ON) == HIGH) {
          bip(200);
          lcd.backlight();
          lcd.clear();
          tengah("REBOOTING...", 1);
          delay(1000);
          connectWiFi(); 
          systemActive = true; 
          break;
        }
      }
    }
  }

  skorKiri = 0; skorKanan = 0;
  currentCommand = "idle"; 
  clearCommand(); 

  lcd.clear();
  tengah("=== THUNDER ===", 0);
  tengah("=== HOOPS ===", 1);
  tengah("READY TO PLAY?", 2);
  tengah("Pencet START/Web", 3);
  
  while (true) {
    if (WiFi.status() != WL_CONNECTED) connectWiFi();
    updateSettings();

    if (digitalRead(PIN_OFF) == HIGH) {
      delay(50);
      if (digitalRead(PIN_OFF) == HIGH) {
        bip(100);
        systemActive = false;
        return; 
      }
    }

    if (digitalRead(PIN_START) == HIGH || currentCommand == "start") {
      bip(100); clearCommand(); break; 
    }
    delay(200); 
  }

  for (int i = 3; i >= 1; i--) { 
    lcd.clear(); 
    lcd.setCursor(9, 1); lcd.print(i); 
    bip(150); delay(850); 
  }
  lcd.clear(); tengah("MULAI!!!", 1); bip(500);
  delay(500);
  lcd.clear();

  waktuMulai = millis();
  bool gameRunning = true;
  while (gameRunning) {
    unsigned long sekarang = millis();
    long sisa = durasiGame - (sekarang - waktuMulai);
    if (sisa <= 0) { gameRunning = false; break; }

    bacaSensor();

    if (digitalRead(PIN_OFF) == HIGH) { systemActive = false; return; }

    static unsigned long lastLcdUpdate = 0;
    if (millis() - lastLcdUpdate > 250) {
      lcd.setCursor(0, 0); lcd.print("KIRI  : "); lcd.print(skorKiri); lcd.print("   ");
      lcd.setCursor(11, 0); lcd.print("KANAN : "); lcd.print(skorKanan); lcd.print("   ");
      lcd.setCursor(0, 3);
      lcd.print("Sisa: "); lcd.print(sisa / 1000); lcd.print(" detik  ");
      lastLcdUpdate = millis();
    }
  }

  bip(1000);
  lcd.clear(); tengah("WAKTU HABIS!", 1);
  delay(1500);
  kirimDataKeWeb();
  
  lcd.clear();
  String pemenang = (skorKiri > skorKanan) ? "WINNER: KIRI" : (skorKanan > skorKiri) ? "WINNER: KANAN" : "HASIL: SERI";
  tengah("GAME OVER", 0);
  tengah(pemenang, 1);
  tengah("--------------------", 2);
  tengah("Pencet RESET/Web", 3);

  while (true) {
    updateSettings();
    
    if (digitalRead(PIN_OFF) == HIGH) {
      systemActive = false;
      return; 
    }

    if (digitalRead(PIN_RESET) == HIGH || currentCommand == "reset") {
      bip(100); 
      clearCommand(); 
      break; 
    }
    delay(500);
  }
}

void updateSettings() {
  if (client.connect(server, 80)) {
    client.println("GET /thunder-hoops/api/get_settings.php HTTP/1.1");
    client.print("Host: "); client.println(server);
    client.println("Connection: close");
    client.println();
    unsigned long timeout = millis();
    while (client.available() == 0) { if (millis() - timeout > 2000) { client.stop(); return; } }
    if(client.find("match_duration\":")) { int val = client.parseInt(); if (val > 0) durasiGame = (unsigned long)val * 1000; }
    if(client.find("game_command\":\"")) { currentCommand = client.readStringUntil('\"'); }
    client.stop();
  }
}

void clearCommand() {
  if (client.connect(server, 80)) {
    client.println("GET /thunder-hoops/api/clear_command.php HTTP/1.1");
    client.print("Host: "); client.println(server);
    client.println("Connection: close");
    client.println();
    client.stop();
  }
}

void kirimDataKeWeb() {
  if (client.connect(server, 80)) {
    String pmn = (skorKiri > skorKanan) ? "KIRI" : (skorKanan > skorKiri) ? "KANAN" : "SERI";
    client.print("GET /thunder-hoops/api/receive.php?skor_kiri=");
    client.print(skorKiri);
    client.print("&skor_kanan=");
    client.print(skorKanan);
    client.print("&pemenang=");
    client.print(pmn);
    client.print("&durasi=");
    client.print(durasiGame/1000);
    client.println(" HTTP/1.1");
    client.print("Host: "); client.println(server);
    client.println("Connection: close");
    client.println();
    client.stop();
  }
}

void bacaSensor() {
  static unsigned long lastScoreKiri = 0;
  static unsigned long lastScoreKanan = 0;
  if (millis() - lastScoreKiri > 600) { 
    if (getDistance(TRIG_KIRI, ECHO_KIRI) < 12) { 
      skorKiri++; bip(70); lastScoreKiri = millis(); 
    } 
  }
  if (millis() - lastScoreKanan > 600) { 
    if (getDistance(TRIG_KANAN, ECHO_KANAN) < 12) { 
      skorKanan++; bip(70); lastScoreKanan = millis(); 
    } 
  }
}

long getDistance(int t, int e) {
  digitalWrite(t, LOW); delayMicroseconds(2); digitalWrite(t, HIGH); delayMicroseconds(10); digitalWrite(t, LOW);
  long d = pulseIn(e, HIGH, 15000); return (d == 0) ? 999 : d * 0.034 / 2;
}
