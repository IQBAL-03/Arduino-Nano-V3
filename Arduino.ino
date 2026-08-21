#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
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

char ssidArr[33] = "[nama_wifi]";
char passArr[33] = "[password_wifi]";
const char server[] = "[domain_tujuan]";

LiquidCrystal_I2C lcd(0x27, 20, 4);
SoftwareSerial espSerial(6, 7);
WiFiEspClient client;
WiFiEspServer serverWeb(80);

int status = WL_IDLE_STATUS;
int skorKiri = 0, skorKanan = 0;
unsigned long waktuMulai;
unsigned long durasiGame = 60000;
char currentCommand[10] = "idle";
bool systemActive = true;

int batasJarakKiri = 6.5;
int batasJarakKanan = 6.5;

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

void tulisWiFiKeEEPROM(String qssid, String qpass) {
  for (int i = 0; i < 32; ++i)
    EEPROM.write(i, i < (int)qssid.length() ? qssid[i] : 0);
  for (int i = 0; i < 32; ++i)
    EEPROM.write(32 + i, i < (int)qpass.length() ? qpass[i] : 0);
}

void bacaWiFiDariEEPROM() {
  String s = "", p = "";
  for (int i = 0; i < 32; ++i) {
    char c = EEPROM.read(i);
    if (c != 0 && c != 255) s += c;
  }
  for (int i = 0; i < 32; ++i) {
    char c = EEPROM.read(32 + i);
    if (c != 0 && c != 255) p += c;
  }
  if (s.length() > 0 && s.length() <= 32) s.toCharArray(ssidArr, 33);
  if (p.length() > 0 && p.length() <= 32) p.toCharArray(passArr, 33);
}

String urlDecode(String str) {
  String decoded = "";
  char temp[] = "0x00";
  int len = str.length();
  int i = 0;
  while (i < len) {
    if (str[i] == '+') {
      decoded += ' ';
      i++;
    } else if (str[i] == '%' && i + 2 < len) {
      temp[2] = str[i + 1];
      temp[3] = str[i + 2];
      decoded += (char)strtol(temp, NULL, 16);
      i += 3;
    } else {
      decoded += str[i];
      i++;
    }
  }
  return decoded;
}

void modeAP() {
  lcd.clear();
  tengah(F("Mode Setup WiFi"), 0);
  tengah(F("Konek: THUNDER-HOOPS"), 1);
  tengah(F("IP: 192.168.4.1"), 2);

  WiFi.beginAP("THUNDER-HOOPS", 10, "", 0);
  serverWeb.begin();

  while (true) {
    WiFiEspClient clientAP = serverWeb.available();
    if (!clientAP) continue;

    String req = "";
    unsigned long startWait = millis();

    while (clientAP.connected() && (millis() - startWait < 2000)) {
      if (clientAP.available()) {
        req += (char)clientAP.read();
        if (req.length() >= 700) break;
      }
    }

    if (req.indexOf(F("POST /save")) != -1) {
      int bodyStart = req.indexOf(F("\r\n\r\n"));
      if (bodyStart == -1) { clientAP.stop(); continue; }

      String body = req.substring(bodyStart + 4);
      body.trim();

      int sStart = body.indexOf(F("s="));
      int sEnd   = body.indexOf(F("&p="));

      if (sStart == -1 || sEnd == -1) { clientAP.stop(); continue; }

      String newSsid = body.substring(sStart + 2, sEnd);
      String newPass = body.substring(sEnd + 3);

      newSsid = urlDecode(newSsid);
      newPass = urlDecode(newPass);

      if (newSsid.length() == 0) { clientAP.stop(); continue; }

      tulisWiFiKeEEPROM(newSsid, newPass);

      clientAP.println(F("HTTP/1.1 200 OK"));
      clientAP.println(F("Content-Type: text/html"));
      clientAP.println(F("Connection: close"));
      clientAP.println();
      clientAP.println(F("<html><body style='text-align:center;font-family:sans-serif;padding:30px;'>"));
      clientAP.println(F("<h2>&#10003; WiFi Berhasil Disimpan!</h2>"));
      clientAP.println(F("<p>Arduino akan restart...</p>"));
      clientAP.println(F("</body></html>"));
      delay(500);
      clientAP.stop();

      lcd.clear();
      tengah(F("WiFi Disimpan!"), 1);
      tengah(F("Restarting..."), 2);
      delay(1500);
      asm volatile ("  jmp 0");

    } else {
      clientAP.println(F("HTTP/1.1 200 OK"));
      clientAP.println(F("Content-Type: text/html"));
      clientAP.println(F("Connection: close"));
      clientAP.println();
      clientAP.println(F("<html><head><meta name='viewport' content='width=device-width,initial-scale=1.0'></head>"));
      clientAP.println(F("<body style='font-family:sans-serif;padding:20px;max-width:400px;margin:auto;'>"));
      clientAP.println(F("<h2>Setup WiFi</h2>"));
      clientAP.println(F("<form action='/save' method='POST'>"));
      clientAP.println(F("<label>SSID:</label><br>"));
      clientAP.println(F("<input type='text' name='s' style='width:100%;padding:10px;margin:8px 0;' required><br>"));
      clientAP.println(F("<label>Password:</label><br>"));
      clientAP.println(F("<input type='password' name='p' style='width:100%;padding:10px;margin:8px 0;'><br><br>"));
      clientAP.println(F("<input type='submit' value='Simpan' style='width:100%;padding:12px;background:#007bff;color:#fff;'>"));
      clientAP.println(F("</form></body></html>"));
      delay(500);
      clientAP.stop();
    }
  }
}

void connectWiFi() {
  bacaWiFiDariEEPROM();
  WiFi.disconnect();
  delay(1000);

  int attempts = 0;
  lcd.clear();
  tengah(F("CONNECTING WiFi"), 1);
  tengah(String(ssidArr), 2);

  while (attempts < 4) {
    status = WiFi.begin(ssidArr, passArr);
    if (status == WL_CONNECTED) {
      lcd.clear();
      tengah(F("WiFi Connected!"), 1);
      bip(300);
      delay(1000);
      return;
    }
    attempts++;
    delay(2500);
  }
  modeAP();
}

void clearCommand() {
  if (!client.connect(server, 80)) return;
  client.println(F("GET /thunder-hoops/api/clear_command.php HTTP/1.1"));
  client.print(F("Host: ")); client.println(server);
  client.println(F("User-Agent: Arduino/1.0"));
  client.println(F("Connection: close"));
  client.println();
  delay(100);
  client.stop();
  delay(100);
}

void updateSettings() {
  if (!client.connect(server, 80)) return;

  String cleanSsid = String(ssidArr);
  String cleanPass = String(passArr);
  cleanSsid.replace(F(" "), F("%20"));
  cleanPass.replace(F(" "), F("%20"));

  client.print(F("GET /thunder-hoops/api/get_settings.php?ack=1&active_ssid="));
  client.print(cleanSsid);
  client.print(F("&active_pass="));
  client.print(cleanPass);
  client.println(F(" HTTP/1.1"));
  client.print(F("Host: ")); client.println(server);
  client.println(F("User-Agent: Arduino/1.0"));
  client.println(F("Connection: close"));
  client.println();

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 4000) { client.stop(); delay(200); return; }
  }

  bool headerSelesai = false;
  int hitungNewline = 0;
  timeout = millis();
  while ((client.connected() || client.available()) && !headerSelesai && (millis() - timeout < 3000)) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n')      hitungNewline++;
      else if (c != '\r') hitungNewline = 0;
      if (hitungNewline >= 2) headerSelesai = true;
    }
  }

  if (!headerSelesai) { client.stop(); delay(200); return; }

  char body[256];
  int idx = 0;
  timeout = millis();
  while ((client.connected() || client.available()) && idx < 255) {
    if (client.available()) {
      body[idx++] = client.read();
      timeout = millis();
    } else {
      if (millis() - timeout > 1500) break;
    }
  }
  body[idx] = '\0';
  client.stop();
  delay(200);

  char* p = strstr(body, "match_duration\":");
  if (p) {
    int val = atoi(p + 16);
    if (val > 0) durasiGame = (unsigned long)val * 1000;
  }

  p = strstr(body, "game_command\":\"");
  if (p) {
    p += 15;
    char cmd[10];
    int j = 0;
    while (p[j] != '"' && p[j] != '\0' && j < 9) { cmd[j] = p[j]; j++; }
    cmd[j] = '\0';
    strcpy(currentCommand, cmd);
  }

  char* pSync = strstr(body, "wifi_sync_pending\":");
  int syncPending = 0;
  if (pSync) {
    syncPending = atoi(pSync + 19);
  }

  if (syncPending == 1) {
    char* pSsid = strstr(body, "wifi_ssid\":\"");
    char* pPass = strstr(body, "wifi_password\":\"");
    if (pSsid && pPass) {
      pSsid += 12;
      char webSsid[33]; int j1 = 0;
      while (pSsid[j1] != '"' && pSsid[j1] != '\0' && j1 < 32) { webSsid[j1] = pSsid[j1]; j1++; }
      webSsid[j1] = '\0';

      pPass += 16;
      char webPass[33]; int j2 = 0;
      while (pPass[j2] != '"' && pPass[j2] != '\0' && j2 < 32) { webPass[j2] = pPass[j2]; j2++; }
      webPass[j2] = '\0';

      if (strlen(webSsid) > 0) {
        tulisWiFiKeEEPROM(String(webSsid), String(webPass));
        delay(1000);
        asm volatile ("  jmp 0");
      }
    }
  }
}

void kirimDataKeWeb() {
  delay(100);
  if (!client.connect(server, 80)) return;
  
  const char* pmn = (skorKiri > skorKanan) ? "KIRI" : (skorKanan > skorKiri) ? "KANAN" : "SERI";
  
  client.print(F("GET /thunder-hoops/api/receive.php?skor_kiri="));
  client.print(skorKiri);
  client.print(F("&skor_kanan="));
  client.print(skorKanan);
  client.print(F("&pemenang="));
  client.print(pmn);
  client.print(F("&durasi="));
  client.print(durasiGame / 1000);
  client.println(F(" HTTP/1.1"));
  client.print(F("Host: ")); client.println(server);
  client.println(F("User-Agent: Arduino/1.0"));
  client.println(F("Connection: close"));
  client.println();
  
  unsigned long timeout = millis();
  while (client.available() == 0 && millis() - timeout < 4000) { }

  client.stop();
  delay(200);
}

long getDistance(int t, int e) {
  digitalWrite(t, LOW); delayMicroseconds(2);
  digitalWrite(t, HIGH); delayMicroseconds(10);
  digitalWrite(t, LOW);
  long d = pulseIn(e, HIGH, 15000);
  return (d == 0) ? 999 : d * 0.034 / 2;
}

void bacaSensor() {
  static unsigned long lastScoreKiri = 0;
  static unsigned long lastScoreKanan = 0;
  if (millis() - lastScoreKiri > 600) {
    if (getDistance(TRIG_KIRI, ECHO_KIRI) < batasJarakKiri) {
      skorKiri++; bip(70); lastScoreKiri = millis();
    }
  }
  if (millis() - lastScoreKanan > 600) {
    if (getDistance(TRIG_KANAN, ECHO_KANAN) < batasJarakKanan) {
      skorKanan++; bip(70); lastScoreKanan = millis();
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
  tengah(F("INITIALIZING..."), 1);
  
  while (espSerial.available()) espSerial.read();
  delay(300);

  WiFi.init(&espSerial);
  if (WiFi.status() == WL_NO_SHIELD) {
    lcd.clear(); tengah(F("ESP ERROR!"), 1);
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
          systemActive = true;
          break;
        }
      }
    }
  }

  skorKiri = 0; skorKanan = 0;
  strcpy(currentCommand, "idle");
  clearCommand();

  lcd.clear();
  tengah(F("=== THUNDER ==="), 0);
  tengah(F("=== HOOPS ==="), 1);
  tengah(F("READY TO PLAY?"), 2);
  tengah(F("Pencet START/Web"), 3);
  
  while (true) {
    if (WiFi.status() != WL_CONNECTED) {
      connectWiFi();
      lcd.clear();
      tengah(F("=== THUNDER ==="), 0);
      tengah(F("=== HOOPS ==="), 1);
      tengah(F("READY TO PLAY?"), 2);
      tengah(F("Pencet START/Web"), 3);
    }

    static unsigned long terakhirCek = 0;
    if (millis() - terakhirCek > 3000) {
      updateSettings();
      terakhirCek = millis();
    }

    if (digitalRead(PIN_OFF) == HIGH) {
      delay(50);
      if (digitalRead(PIN_OFF) == HIGH) {
        bip(100);
        systemActive = false;
        return;
      }
    }

    if (digitalRead(PIN_START) == HIGH || strcmp(currentCommand, "start") == 0) {
      bip(100);
      clearCommand();
      break;
    }
    delay(100);
  }

  for (int i = 3; i >= 1; i--) {
    lcd.clear();
    lcd.setCursor(9, 1); lcd.print(i);
    bip(150); delay(850);
  }
  lcd.clear(); tengah(F("MULAI!!!"), 1); bip(500);
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
      lcd.setCursor(0, 0); lcd.print(F("KIRI: ")); lcd.print(skorKiri); lcd.print(F(" "));
      lcd.setCursor(11, 0); lcd.print(F("KANAN: ")); lcd.print(skorKanan); lcd.print(F(" "));
      lcd.setCursor(0, 3);
      lcd.print(F("Sisa: ")); lcd.print(sisa / 1000); lcd.print(F(" detik  "));
      lastLcdUpdate = millis();
    }
  }

  bip(1000);
  lcd.clear(); tengah(F("WAKTU HABIS!"), 1);
  delay(1500);
  kirimDataKeWeb();
  
  lcd.clear();
  String pemenang = (skorKiri > skorKanan) ? "WINNER: KIRI" : (skorKanan > skorKiri) ? "WINNER: KANAN" : "HASIL: SERI";
  tengah(F("GAME OVER"), 0);
  tengah(pemenang, 1);
  tengah(F("--------------------"), 2);
  tengah(F("Pencet RESET/Web"), 3);

  while (true) {
    static unsigned long terakhirCekReset = 0;
    if (millis() - terakhirCekReset > 3000) {
      updateSettings();
      terakhirCekReset = millis();
    }

    if (digitalRead(PIN_OFF) == HIGH) {
      systemActive = false;
      return;
    }
    
    if (digitalRead(PIN_RESET) == HIGH || strcmp(currentCommand, "reset") == 0) {
      bip(100);
      clearCommand();
      break;
    }
    delay(200);
  }
}
