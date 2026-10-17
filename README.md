# Lolin NodeMCU V3 (ESP8266) + Pantalla SPI TFT 2.8" (ILI9341)

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Core_6-orange?logo=platformio)](https://platformio.org/)
[![ESP8266](https://img.shields.io/badge/Board-NodeMCU_V3_(ESP8266)-blue?logo=espressif)](https://www.espressif.com/)
[![Display](https://img.shields.io/badge/TFT-ILI9341_SPI_2.8''-red)](https://www.adafruit.com/)
[![Framework](https://img.shields.io/badge/Framework-Arduino-00979D?logo=arduino)](https://www.arduino.cc/)

Prueba de concepto (**PoC**) para conectar y controlar una pantalla gráfica **TFT de 2.8 pulgadas SPI con controlador ILI9341** (resolución 320x240) utilizando una placa **Lolin NodeMCU V3 (ESP8266)** y **PlatformIO**.

La PoC se conecta a una red Wi-Fi (2.4 GHz) y presenta en la pantalla información en tiempo real:
* Estado de conexión con animación progresiva.
* SSID, dirección IP asignada, puerta de enlace y dirección MAC.
* Nivel de señal Wi-Fi (**RSSI**) con refresco en vivo.
* Memoria RAM libre y canal Wi-Fi activo.
* Reportes detallados por el puerto serie a **115200 baudios**.

---

## Diagrama de Conexión

### Tabla de Pines (Lolin NodeMCU V3 -> TFT ILI9341 SPI)

| Pin Pantalla TFT | Pin NodeMCU (Serigrafía) | GPIO ESP8266 | Descripción |
| :--- | :--- | :--- | :--- |
| **VCC** | **VIN (5V)** ó **3V3** | - | Alimentación (la mayoría de los módulos de 2.8" traen regulador interno de 3.3V en VCC) |
| **GND** | **GND** | - | Masa / Tierra común |
| **CS** | **D8** | GPIO15 | Chip Select de la pantalla |
| **RESET / RST** | **D4** | GPIO2 | Reset del controlador TFT |
| **DC / RS** | **D3** | GPIO0 | Data / Command selector |
| **SDI (MOSI)** | **D7** | GPIO13 | Bus HSPI - Datos hacia la pantalla (MOSI) |
| **SCK (CLK)** | **D5** | GPIO14 | Bus HSPI - Reloj (Clock) |
| **LED** | **3V3** | - | Retroiluminación (Backlight) |
| **SDO (MISO)** | **D6** *(Opcional)* | GPIO12 | Bus HSPI - Lectura de pantalla (no indispensable) |

> [!NOTE]
> Los pines del panel táctil (**T_CLK, T_CS, T_DIN, T_DO, T_IRQ**) y los pines de tarjeta SD (**SD_CS, SD_MOSI, SD_MISO, SD_SCK**) no se requieren para esta PoC y pueden dejarse sin conectar.

---

### Diagrama Esquemático

```text
       +----------------------------+
       |   Lolin NodeMCU V3         |
       |      (ESP8266)             |
       +----------------------------+
       |   VIN (5V) / 3V3           | -----> VCC   (Módulo TFT 2.8")
       |   GND                      | -----> GND   (Módulo TFT 2.8")
       |   D8 (GPIO15)              | -----> CS    (TFT Chip Select)
       |   D4 (GPIO2)               | -----> RESET (TFT Reset)
       |   D3 (GPIO0)               | -----> DC/RS (Data / Command)
       |   D7 (GPIO13 - HSPI MOSI)  | -----> SDI   (MOSI / TFT Data In)
       |   D5 (GPIO14 - HSPI CLK)   | -----> SCK   (CLK / SPI Clock)
       |   3V3                      | -----> LED   (TFT Backlight)
       |   D6 (GPIO12 - HSPI MISO)  | .....> SDO   (MISO - Opcional)
       +----------------------------+
```

---

## Estructura del Proyecto

```text
lolin-spi-tft-module-example/
├── platformio.ini              # Configuración y dependencias (Adafruit GFX / ILI9341)
├── .gitignore                  # Exclusiones de control de versiones y credenciales
├── include/
│   ├── credentials.h.example   # Plantilla de credenciales Wi-Fi
│   └── credentials.h           # Credenciales locales (ignorado por git)
├── src/
│   └── main.cpp                # Código fuente: TFT ILI9341 + Wi-Fi
└── README.md                   # Documentación y diagrama de cableado
```

---

## Configuración de Credenciales Wi-Fi

1. Copia la plantilla de credenciales:
   ```bash
   cp include/credentials.h.example include/credentials.h
   ```

2. Edita `include/credentials.h` con el nombre y contraseña de tu red Wi-Fi (solo redes de 2.4 GHz):
   ```cpp
   #define WIFI_SSID "Tu_Red_WiFi_2.4GHz"
   #define WIFI_PASSWORD "Tu_Password_Seguro"
   ```

---

## Compilación y Carga con PlatformIO

### 1. Compilar el firmware
```bash
pio run
```

### 2. Subir el binario al NodeMCU
Conecta la placa a la computadora con cable micro-USB y ejecuta:
```bash
pio run -t upload
```

### 3. Abrir el Monitor Serie
```bash
pio device monitor
```
*(Velocidad configurada en 115200 baudios. Salida: `Ctrl + ]`)*

---

## Consideraciones y Solución de Problemas

1. **Pantalla en blanco con luz de fondo encendida:**
   * Revisa que los pines `SCK (D5)` y `SDI/MOSI (D7)` estén firmemente conectados a las líneas de hardware HSPI.
   * Verifica la conexión de `DC (D3)` y `CS (D8)`.
2. **La placa no arranca / entra en modo bootloader inesperado:**
   * Los pines `GPIO0 (D3)` y `GPIO15 (D8)` participan en el modo de arranque del ESP8266. En caso de conflicto de resistencias pull-up externas del módulo TFT, conectar `RESET` a `RST` del NodeMCU y alimentar el pin `VCC` a `VIN` (5V).
3. **Red Wi-Fi:**
   * El ESP8266 solo opera en la banda de **2.4 GHz**.
