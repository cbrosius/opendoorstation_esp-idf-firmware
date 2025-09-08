# ESP32-S3 SIP Türstation

## 📖 Beschreibung
Dieses Projekt beschreibt die Entwicklung einer modular erweiterbaren SIP-basierten Türstation auf Basis eines **ESP32-S3**.  
Die Türstation soll flexibel an verschiedene Anwendungsfälle angepasst werden können: von einer einfachen Klingel mit SIP-Anbindung bis hin zu einer komplexen Video-/Audio-Türsprechanlage mit biometrischer Zugangskontrolle.  

---

## ✨ Features
- **Modularer Aufbau**:
  - 1–n Klingeltaster mit zugehörigem Klingelschild
  - Beleuchtete Klingelschilder (optional mehrfarbig zur Zustandsanzeige)
- **Audio**:
  - Standardmikrofon
  - Optional: **XMOS DSP** für Echo- und Geräuschunterdrückung
- **Relaissteuerung**:
  - 1–n Relais für Türöffner, Lichtsteuerung oder weitere Funktionen
- **Video (optional)**:
  - Kameraunterstützung (ESP32-S3 + kompatible Kamera)
- **Zugangskontrolle (optional)**:
  - Fingerabdruckleser
  - Zahlencode-Tastatur
  - NFC-Reader für Smartphone-Zutritt
- **Netzteil**:
  - Großer Eingangsspannungsbereich
  - Optional **PoE** (Power over Ethernet)
- **Netzwerk**:
  - WLAN
  - Optional Ethernet (via PHY)
  - Webserver zur Konfiguration und Steuerung

---

## 📋 Bill of Materials (BoM)
| Komponente              | Beschreibung                                | Pflicht/Optional |
|--------------------------|---------------------------------------------|------------------|
| ESP32-S3 Modul          | Hauptcontroller mit SIP-Stack               | Pflicht          |
| Mikrofon                | Standard, MEMS oder ECM                     | Pflicht          |
| XMOS DSP                | Erweiterung für Noise Cancelling            | Optional         |
| Klingeltaster 1–n       | Mechanisch oder kapazitiv                   | Pflicht          |
| LED(s)                  | Für Klingelschild-Beleuchtung / Statusanzeige | Optional        |
| Relais 1–n              | Für Türöffner, Licht, etc.                  | Optional         |
| Kamera (ESP32-CAM/OV2640)| Videounterstützung                         | Optional         |
| Fingerprint-Sensor      | Biometrische Authentifizierung              | Optional         |
| Tastatur (Matrix)       | PIN-Code Eingabe                            | Optional         |
| NFC-Reader (PN532 o.ä.) | Smartphone-/RFID-Zugang                     | Optional         |
| Netzteil                | Weitbereichseingang, ggf. PoE               | Pflicht          |
| Lautsprecher            | zur Kommunikation mit dem Anwender          | Pflicht          |
| Lautsprechertreiber     | zur Ansteuerung des Lautsprechers           | Pflicht          |


---

## ⚙️ Installation
1. Hardwaremodule entsprechend den gewünschten Features auswählen und bestücken.  
2. Stromversorgung (Netzteil oder PoE) anschließen.  
3. ESP32-S3 flashen (Firmware).  
4. Netzwerkverbindung konfigurieren (WLAN oder Ethernet).  
5. SIP-Server/Registrar konfigurieren (z. B. Asterisk, FritzBox, Home Assistant Add-on).  

---

## 🚀 Verwendung
- Anruf wird per SIP an registrierte Clients (Smartphone-App, VoIP-Telefon, Home Assistant) weitergeleitet.  
- Relais können über definierte SIP-DTMF-Codes oder lokale Eingaben angesteuert werden.  
- Beleuchtung und Status-LEDs zeigen Klingel-/Systemzustände an.  
- Kamera über SIP-Video-Call oder HTTP/MJPEG-Stream verfügbar.  
- Zugangskontrolle über Fingerprint, Tastatur oder NFC.  

---

## 💻 Programmierung
- **Framework**: ESPHome oder ESP-IDF/Arduino mit SIP-Bibliothek  
- **SIP-Stack**: z. B. [PJSIP](https://www.pjsip.org/) oder ESP-SIP-Library  
- **Modulare Architektur**:
  - Core: SIP, Relaissteuerung, Klingeltaster
  - Erweiterungen: Kamera, Fingerprint, DSP  
- **Updatefähigkeit**:
  - OTA-Updates über WLAN  
  - Lokale UART/USB-Programmierung  

---

## ✅ ToDos
- [ ] Auswahl SIP-Bibliothek für ESP32-S3  
- [ ] Hardwaredesign für modularen Aufbau (Backplane + Module)  
- [ ] Stromversorgung (Weitbereich + PoE-Option)  
- [ ] Implementierung Klingeltaster + LED-Beleuchtung  
- [ ] Audio-Implementierung (DSP optional)  
- [ ] Relais-Ansteuerung (min. 2 Kanäle)  
- [ ] Kamera-Integration  
- [ ] Fingerprint / NFC / Tastatur-Unterstützung  
- [ ] Home Assistant / MQTT Integration  

---

## 🛠️ Troubleshooting
| Problem                      | Mögliche Ursache                | Lösungsvorschlag              |
|------------------------------|---------------------------------|-------------------------------|
| Keine SIP-Registrierung      | Falsche SIP-Serverdaten         | Zugangsdaten prüfen, Server erreichbar? |
| Kein Audio                   | Mikrofon nicht erkannt/defekt   | Verkabelung prüfen, Treiber aktivieren |
| Relais schaltet nicht        | Fehlerhafte Zuweisung GPIO      | Pin-Mapping in Firmware prüfen |
| Kamera liefert kein Bild     | Inkompatibles Modell            | Kamera-Treiber/Firmware prüfen |
| Fingerprint/NFC nicht erkannt| Modul nicht initialisiert       | Verdrahtung + Init-Sequenz checken |

---
