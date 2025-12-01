#include <PZEM004Tv30.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define BLYNK_TEMPLATE_ID "TMPL6c1Gq4IvD"
#define BLYNK_TEMPLATE_NAME "ESP32 Energy Monitor"

#define BLYNK_FIRMWARE_VERSION        "0.1.0"

#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG

#define APP_DEBUG

// Uncomment your board, or configure a custom board in Settings.h
#define USE_ESP32_DEV_MODULE

#include "BlynkEdgent.h"

// --- Konfigurasi PZEM ---
#if !defined(PZEM_RX_PIN) && !defined(PZEM_TX_PIN)
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#endif

#if !defined(PZEM_SERIAL)
#define PZEM_SERIAL Serial2
#endif

// --- Konfigurasi OLED ---
#define SCREEN_WIDTH 128 // Lebar OLED
#define SCREEN_HEIGHT 64 // Tinggi OLED
#define OLED_RESET    -1 // Reset pin # (atau 4 jika Anda menggunakannya)
#define OLED_I2C_ADDRESS 0x3C // Alamat I2C OLED (cek scanner jika tidak terdeteksi)

/* =================================================== */
/* --- OBJEK & VARIABEL GLOBAL --- */
/* =================================================== */

// Objek PZEM
#if defined(ESP32)
PZEM004Tv30 pzem(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN);
#else
PZEM004Tv30 pzem(PZEM_SERIAL);
#endif

// Objek OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Objek Timer (sudah termasuk dalam BlynkEdgent)
BlynkTimer timer;

// Variabel untuk ganti layar OLED
bool showFirstScreen = true;

/* =================================================== */
/* --- FUNGSI TIMER UNTUK BACA SENSOR & UPDATE --- */
/* =================================================== */
void sendSensorData() {
  // Baca data dari sensor
  float voltage = pzem.voltage();
  float current = pzem.current();
  float power = pzem.power();
  float energy_kWh = pzem.energy(); // Baca nilai asli dalam kWh
  float frequency = pzem.frequency();
  float pf = pzem.pf();

  // Cek jika data valid
  if (isnan(voltage) || isnan(current) || isnan(power) || isnan(energy_kWh) || isnan(frequency) || isnan(pf)) { // Cek 'energy_kWh'
    Serial.println("Error reading PZEM!");
    
    // Tampilkan error di OLED
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Error Baca PZEM!");
    display.println("Periksa koneksi.");
    display.display();
    return; // Keluar dari fungsi
  }

  // === MODIFIKASI (kWh -> Wh + Gimmick) ===
  
  // 1. Ubah kWh menjadi Wh
  float energy_Wh = energy_kWh * 1000.0;

  // 2. Buat nilai "gimmick" agar tidak 0 dan terkait dengan beban (power)
  //    Kita tambahkan sebagian kecil dari nilai power + angka acak (0.00 - 0.99)
  //    Ini memastikan nilainya tidak 0 (jika power > 0) dan tidak terlalu jauh dari aslinya
  float gimmick_offset = (power * 0.005) + ((float)random(0, 100) / 100.0);
  
  // 3. Tambahkan gimmick ke nilai Wh
  float final_energy_Wh = energy_Wh + gimmick_offset;

  // === AKHIR MODIFIKASI ===


  // Cetak nilai ke Serial console
  Serial.println("--- Data Sensor ---");
  Serial.print("Voltage: ");     Serial.print(voltage);       Serial.println("V");
  Serial.print("Current: ");     Serial.print(current);       Serial.println("A");
  Serial.print("Power: ");       Serial.print(power);         Serial.println("W");
  Serial.print("Energy: ");      Serial.print(final_energy_Wh, 0); Serial.println("Wh"); // Tampilkan Wh (0 desimal)
  Serial.print("Frequency: ");   Serial.print(frequency, 1);  Serial.println("Hz");
  Serial.print("PF: ");          Serial.println(pf);

  // Kirim data ke Blynk
  Blynk.virtualWrite(V0, voltage);
  Blynk.virtualWrite(V1, current);
  Blynk.virtualWrite(V2, power);
  Blynk.virtualWrite(V3, final_energy_Wh); // Kirim nilai Wh (gimmick)
  Blynk.virtualWrite(V4, frequency);
  Blynk.virtualWrite(V5, pf);

  // Tampilkan data ke OLED
  display.clearDisplay();
  display.setTextSize(2); // Ukuran teks besar
  display.setTextColor(SSD1306_WHITE);

  if (showFirstScreen) {
    // Layar 1: Tegangan, Arus, Daya
    display.setCursor(0, 0);  display.print("V:"); display.print(voltage, 1); display.println("V");
    display.setCursor(0, 22); display.print("C:"); display.print(current, 2); display.println("A");
    display.setCursor(0, 44); display.print("P:"); display.print(power, 0);   display.println("W");
  } else {
    // Layar 2: Energi, Frekuensi, PF
    display.setCursor(0, 0);  display.print("E:"); display.print(final_energy_Wh, 0); display.println("Wh"); // Tampilkan Wh (0 desimal)
    display.setCursor(0, 22); display.print("F:"); display.print(frequency, 1);     display.println("Hz");
    display.setCursor(0, 44); display.print("PF:"); display.print(pf, 2);
  }
  display.display();

  // Ganti status layar untuk panggilan berikutnya
  showFirstScreen = !showFirstScreen;
}

/* =================================================== */
/* --- FUNGSI SETUP --- */
/* =================================================== */
void setup() {
  Serial.begin(115200);
  delay(100);

  // Inisialisasi OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    Serial.println(F("Alokasi SSD1306 gagal"));
    for (;;); // Loop tak terbatas jika OLED gagal
  }
  Serial.println("OLED OK!");
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Mulai Edgent...");
  display.println("Menunggu Konfigurasi...");
  display.display();

  // ! PENTING: Inisialisasi Serial2 untuk PZEM
  PZEM_SERIAL.begin(9600, SERIAL_8N1, PZEM_RX_PIN, PZEM_TX_PIN);
  Serial.println("Serial2 PZEM (Pin 16, 17) OK.");

  // pzem.resetEnergy(); // Hapus komentar jika ingin reset energi setiap startup
  
  BlynkEdgent.begin();

  // Tampilkan status terhubung setelah Blynk siap
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Terhubung ke Blynk!");
  display.display();
  delay(1500); // Tampilkan pesan sebentar

  // Setup timer untuk memanggil sendSensorData setiap 2 detik (2000 ms)
  timer.setInterval(2000L, sendSensorData);
}

/* =================================================== */
/* --- FUNGSI LOOP UTAMA --- */
/* =================================================== */
void loop() {
  BlynkEdgent.run();
  timer.run(); // Menjalankan timer Blynk
  
  // Semua logika pembacaan sensor dan update
  // kini ditangani oleh fungsi sendSensorData() via timer
}
