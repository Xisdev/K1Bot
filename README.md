# 🤖 K1bot Ultimate

![K1bot Icon](icon.png)

**K1bot** adalah proyek robotika hybrid berbasis **Arduino** dan **Android (Jetpack Compose)**. Sistem ini memungkinkan pengendalian robot secara manual (Joystick/Arrow) dan otomatis (Line Follower) dengan kemampuan **Live PID Tuning** tanpa perlu upload ulang kode ke mikrokontroler.

Proyek ini dirancang untuk fleksibilitas tinggi, kemudahan penggunaan, dan performa yang dapat disesuaikan secara *real-time*.

## ✨ Fitur Utama

### 📱 Android App (Kotlin + Jetpack Compose)
* **Dual Control Mode:**
    * **Manual:** Joystick Analog (Differential Steering Mixing) & Arrow Keys (D-Pad).
    * **Auto:** Switch mode untuk mengaktifkan Line Follower.
* **Live Tuning System:** Ubah 19 parameter robot (PID, Speed, Correction Zones) secara langsung lewat Bluetooth.
* **Config Management:**
    * Simpan preset tuning ke penyimpanan HP (format JSON).
    * Muat (Load) preset yang tersimpan dengan mudah.
    * Hapus preset yang tidak dibutuhkan.
* **Action Emotes:** Tombol pintas untuk gerakan preset (Spin, Shock, Run, Happy, Angry).
* **Relay Control:** Tombol khusus (Hold-to-Active) untuk mengaktifkan modul tambahan (Pin A5).
* **Modern UI:** Antarmuka Landscape Fullscreen yang responsif dan futuristik.

### 🤖 Arduino Firmware
* **Smooth PID Algorithm:** Algoritma Line Follower yang agresif namun halus (dikalibrasi oleh tim).
* **Two-Way Communication:** Mengirim dan menerima data konfigurasi dari Android.
* **Hybrid Logic:** Prioritas eksekusi cerdas (Relay > Mode > Manual/Auto).
* **Momentary Relay:** Logika relay aktif hanya saat tombol ditekan.

---

## 🛠️ Hardware Requirements

| Komponen | Deskripsi |
| :--- | :--- |
| **Microcontroller** | Arduino Uno R3 |
| **Motor Driver** | Adafruit Motor Shield v1 (L293D) |
| **Communication** | Modul Bluetooth HC-05 / HC-06 |
| **Sensors** | 5-Channel IR Line Sensor Array |
| **Actuators** | 2x DC Gearbox Motors + Roda |
| **Power** | Baterai Li-ion 18650 |
| **Extra** | Modul Relay |

---

## 🔌 Wiring & Pinout

Pastikan koneksi kabel sesuai dengan konfigurasi di kode program (`k1bot.ino`):

| Arduino Pin | Komponen | Keterangan |
| :--- | :--- | :--- |
| **D2** | HC-05 TX | SoftwareSerial RX (Arduino) |
| **D13** | HC-05 RX | SoftwareSerial TX (Arduino) |
| **A0** | Sensor S1 | Kiri Luar |
| **A1** | Sensor S2 | Kiri |
| **A2** | Sensor S3 | Tengah |
| **A3** | Sensor S4 | Kanan |
| **A4** | Sensor S5 | Kanan Luar |
| **A5** | Relay | Output Active High |
| **M1** | Motor Kiri | Terminal M1 di Shield |
| **M2** | Motor Kanan | Terminal M2 di Shield |

> **Catatan:** Pin `D13` Arduino terhubung ke `RX` Bluetooth. Karena level logika HC-05 biasanya 3.3V, disarankan menggunakan *Voltage Divider* (Resistor) jika perlu, meskipun seringkali bekerja langsung pada 5V untuk waktu singkat.

---

## 🚀 Instalasi

### 1. Arduino Firmware
1.  Buka folder `Arduino/` (atau lokasi kode Arduino kamu).
2.  Pastikan library **AFMotor** (Adafruit Motor Shield v1) sudah terinstall di Arduino IDE.
3.  Upload file `.ino` ke board Arduino Uno.
4.  **Penting:** Cabut kabel RX/TX Bluetooth saat proses upload agar tidak error.

---

## 📖 Panduan Penggunaan

1.  **Koneksi:** Nyalakan Robot -> Buka App -> Klik Ikon Bluetooth -> Pilih HC-05 (atau nama dari modul bluetooth).
2.  **Mode Manual:** Gunakan Joystick di kiri untuk bergerak. Gunakan tombol aksi di kanan (Lari, Putar, dll).
3.  **Mode Auto:** Aktifkan switch **"Line Follower Mode"**. Robot akan mengabaikan joystick dan mengikuti garis secara otomatis.
4.  **Relay:** Tekan dan tahan tombol **"Otak"** (Cyan) untuk menyalakan Relay. Lepas untuk mematikan.
5.  **Tuning (Ikon Gear):**
    * **READ:** Membaca settingan yang sedang dipakai robot saat ini.
    * **SAVE AS:** Menyimpan settingan di layar ke memori HP (Beri nama file).
    * **LOAD FILE:** Membuka daftar file yang tersimpan (Bisa hapus file di sini).
    * **UPLOAD:** Mengirim settingan di layar ke Robot.

---

## ⚙️ Parameter Tuning

Berikut penjelasan singkat parameter yang bisa diatur lewat aplikasi:

* **KP, KI, KD:** Konstanta PID utama untuk kestabilan tracking garis.
* **Base Speed:** Kecepatan dasar robot saat jalan lurus.
* **Max Speed:** Batas kecepatan maksimum.
* **Dead Band:** Toleransi error sensor di mana robot dianggap "lurus".
* **Correction Limits:** Batas kekuatan belokan motor agar tidak *overshoot*.

---

## 👥 Credits

Project ini dikembangkan dan di-tuning dengan penuh ❤️ oleh:
* **Kusdinar** (Lead Developer)
* Mila (Software)
* Helkhan (Mekanik)
* Mochrendi (Mekanik)
* Indra (Mekanik)
* Farid (ELektrikal)
* Esti (Elektrikal)

---

> **Disclaimer:** Pastikan baterai robot terisi penuh saat melakukan tuning PID untuk hasil yang akurat.
