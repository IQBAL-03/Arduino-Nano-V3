# THUNDER HOOPS - Firmware Arduino Nano V3

Firmware sistem IoT untuk mesin arcade / mini game basket **THUNDER HOOPS** berbasis **Arduino Nano V3** dan modul **ESP8266 (WiFiEsp)**.

---

## Web Application & Dashboard (Hub / Penghubung IoT)

Proyek ini adalah sistem berbasis **IoT (Internet of Things)** yang membutuhkan web server/dashboard sebagai perantara dan monitoring kontrol sistem (mulai game, reset game, ganti durasi, sinkronisasi WiFi, dan rekap skor permainan).

**Repository Web App:**
[https://github.com/CandyFrog/THUNDER-HOOPS](https://github.com/CandyFrog/THUNDER-HOOPS)

Pastikan server web / dashboard sudah berjalan sebelum menghubungkan perangkat Arduino.

---

## Konfigurasi Awal Firmware

Sebelum mengunggah kode [`Arduino.ino`](Arduino.ino) ke papan Arduino Nano, pastikan Anda menyesuaikan target konfigurasi default berikut:

```cpp
char ssidArr[33] = "[nama_wifi]";        // Ganti dengan SSID / nama WiFi Anda
char passArr[33] = "[password_wifi]";    // Ganti dengan Password WiFi Anda
const char server[] = "[domain_tujuan]"; // Ganti dengan Domain/IP Server Web Dashboard (contoh: 192.168.1.100 atau domain.com)
```

---

## Pinout & Komponen

| Komponen / Fungsi                               | Pin Arduino Nano       | Keterangan                      |
| :---------------------------------------------- | :--------------------- | :------------------------------ |
| **ESP8266 RX**                            | D6 (SoftwareSerial TX) | Komunikasi WiFi serial          |
| **ESP8266 TX**                            | D7 (SoftwareSerial RX) | Komunikasi WiFi serial          |
| **Sensor Ultrasonik Kiri (Trig / Echo)**  | D2 / D3                | Deteksi bola basket ring kiri   |
| **Sensor Ultrasonik Kanan (Trig / Echo)** | D4 / D5                | Deteksi bola basket ring kanan  |
| **Buzzer**                                | D8                     | Efek suara / notifikasi nada    |
| **Tombol START**                          | D9                     | Mulai permainan                 |
| **Tombol RESET**                          | D10                    | Reset skor / standby            |
| **Tombol ON**                             | D11                    | Menyalakan sistem / backlight   |
| **Tombol OFF**                            | D12                    | Mematikan sistem / standby mode |
| **LCD I2C 20x4**                          | SDA (A4) / SCL (A5)    | Tampilan antarmuka permainan    |

---

## Fitur Utama

- **Koneksi WiFi & Fallback Mode AP:** Apabila gagal terhubung ke WiFi utama, Arduino otomatis membuka WiFi Access Point `THUNDER-HOOPS` (IP: `192.168.4.1`) untuk konfigurasi WiFi langsung dari HP/Browser dan menyimpannya di EEPROM.
- **Sinkronisasi Web Real-Time:** Menerima perintah *START*, *RESET*, perubahan *durasi pertandingan*, dan *sinkronisasi WiFi* langsung dari dashboard web.
- **Pengiriman Skor Otomatis:** Mengirim hasil akhir permainan beserta pemenang ke web API setelah pertandingan selesai.
- **Dual Sensor Ultrasonik:** Deteksi ring basket presisi untuk pemain Kiri dan Kanan.
