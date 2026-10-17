#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "credentials.h"

/**
 * PoC Lolin NodeMCU V3 (ESP8266) + Pantalla TFT SPI 2.8" (ILI9341)
 *
 * Mapeo de Pines SPI (ESP8266 HSPI):
 *   TFT_CS   -> D8 (GPIO15)
 *   TFT_DC   -> D3 (GPIO0)
 *   TFT_RST  -> D4 (GPIO2 / LED_BUILTIN)
 *   TFT_MOSI -> D7 (GPIO13 / HSPI MOSI)
 *   TFT_CLK  -> D5 (GPIO14 / HSPI SCK)
 *   TFT_MISO -> D6 (GPIO12 / HSPI MISO - opcional)
 */

#define TFT_CS   D8
#define TFT_DC   D3
#define TFT_RST  D4

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Control de tiempo y estado Wi-Fi
bool wasConnected = false;
unsigned long connectionStartTime = 0;
unsigned long lastStatusPrint = 0;
unsigned long lastScreenUpdate = 0;
constexpr unsigned long STATUS_PRINT_INTERVAL = 30000; // 30 s por serial
constexpr unsigned long SCREEN_UPDATE_INTERVAL = 1000; // 1 s en display

// Dibuja la cabecera fija de la interfaz
void drawHeader() {
    tft.fillRect(0, 0, 320, 34, ILI9341_NAVY);
    tft.drawFastHLine(0, 34, 320, ILI9341_CYAN);

    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(12, 9);
    tft.print("ESP8266 - TFT 2.8\"");

    tft.setTextSize(1);
    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(240, 13);
    tft.print("ILI9341");
}

// Pantalla inicial antes de conectar
void showInitialScreen() {
    tft.fillScreen(ILI9341_BLACK);
    drawHeader();

    // Marco de estado
    tft.drawRoundRect(10, 45, 300, 185, 8, ILI9341_DARKGREY);

    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(25, 60);
    tft.print("Estado de Red");

    tft.setTextSize(1);
    tft.setTextColor(ILI9341_LIGHTGREY);
    tft.setCursor(25, 95);
    tft.print("SSID Objetivo:");

    tft.setTextSize(2);
    tft.setTextColor(ILI9341_CYAN);
    tft.setCursor(25, 112);
    tft.print(WIFI_SSID);

    tft.setTextSize(1);
    tft.setTextColor(ILI9341_ORANGE);
    tft.setCursor(25, 145);
    tft.print("Conectando...");
}

// Actualiza animación o estado de intento de conexión
void updateConnectingScreen(uint16_t step) {
    tft.fillRect(25, 170, 270, 20, ILI9341_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(25, 175);
    tft.print("Esperando enlace");

    uint8_t dots = (step % 5) + 1;
    for (uint8_t i = 0; i < dots; i++) {
        tft.print(".");
    }
}

// Muestra la tarjeta con datos completos de la conexión
void drawConnectedScreen() {
    // Limpiar área de contenido principal
    tft.fillRect(10, 45, 300, 185, ILI9341_BLACK);
    tft.drawRoundRect(10, 45, 300, 185, 8, ILI9341_GREEN);

    // Título de estado
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_GREEN);
    tft.setCursor(25, 58);
    tft.print("WI-FI CONECTADO");

    tft.drawFastHLine(20, 80, 280, ILI9341_DARKGREY);

    // Filas de información
    tft.setTextSize(1);

    // SSID
    tft.setTextColor(ILI9341_LIGHTGREY);
    tft.setCursor(25, 92);
    tft.print("SSID:");
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(110, 92);
    tft.print(WiFi.SSID());

    // IP
    tft.setTextColor(ILI9341_LIGHTGREY);
    tft.setCursor(25, 110);
    tft.print("IP Asignada:");
    tft.setTextColor(ILI9341_GREENYELLOW);
    tft.setCursor(110, 110);
    tft.print(WiFi.localIP().toString());

    // Gateway
    tft.setTextColor(ILI9341_LIGHTGREY);
    tft.setCursor(25, 128);
    tft.print("Gateway:");
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(110, 128);
    tft.print(WiFi.gatewayIP().toString());

    // MAC
    tft.setTextColor(ILI9341_LIGHTGREY);
    tft.setCursor(25, 146);
    tft.print("MAC:");
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(110, 146);
    tft.print(WiFi.macAddress());

    // Señal (RSSI)
    tft.setTextColor(ILI9341_LIGHTGREY);
    tft.setCursor(25, 164);
    tft.print("Senal (RSSI):");
    tft.setTextColor(ILI9341_CYAN);
    tft.setCursor(110, 164);
    tft.printf("%d dBm", WiFi.RSSI());

    // Tiempo de enlace
    tft.setTextColor(ILI9341_LIGHTGREY);
    tft.setCursor(25, 182);
    tft.print("Tiempo enlace:");
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(110, 182);
    tft.printf("%lu ms", millis() - connectionStartTime);

    // Barra de estado inferior
    tft.fillRect(10, 203, 280, 20, ILI9341_NAVY);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(20, 209);
    tft.printf("RAM Libre: %u B | Ch: %d", ESP.getFreeHeap(), WiFi.channel());
}

// Muestra alerta si se desconecta
void drawDisconnectedScreen() {
    tft.fillRect(10, 45, 300, 185, ILI9341_BLACK);
    tft.drawRoundRect(10, 45, 300, 185, 8, ILI9341_RED);

    tft.setTextSize(2);
    tft.setTextColor(ILI9341_RED);
    tft.setCursor(25, 65);
    tft.print("CONEXION PERDIDA");

    tft.setTextSize(1);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(25, 105);
    tft.print("Reintentando reconexion automatica...");

    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(25, 130);
    tft.printf("SSID: %s", WIFI_SSID);
}

// Refresca la telemetría dinámica periódicamente en pantalla
void refreshTelemetry() {
    tft.fillRect(110, 164, 80, 12, ILI9341_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_CYAN);
    tft.setCursor(110, 164);
    tft.printf("%d dBm", WiFi.RSSI());

    tft.fillRect(10, 203, 280, 20, ILI9341_NAVY);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(20, 209);
    tft.printf("RAM Libre: %u B | Ch: %d", ESP.getFreeHeap(), WiFi.channel());
}

void printChipInfo() {
    Serial.println(F("\n--- Lolin NodeMCU V3 (ESP8266) + TFT ILI9341 PoC ---"));
    Serial.printf("Chip ID:         0x%08X\n", ESP.getChipId());
    Serial.printf("Frecuencia CPU:  %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Versión SDK:     %s\n", ESP.getSdkVersion());
    Serial.printf("Flash Size:      %u bytes\n", ESP.getFlashChipRealSize());
    Serial.printf("Memoria Libre:   %u bytes\n", ESP.getFreeHeap());
    Serial.printf("Dirección MAC:   %s\n", WiFi.macAddress().c_str());
    Serial.println(F("-----------------------------------------------------"));
}

void printWifiDetails() {
    Serial.println(F("\n========================================"));
    Serial.println(F("         CONEXIÓN WI-FI EXITOSA         "));
    Serial.println(F("========================================"));
    Serial.printf("SSID:            %s\n", WiFi.SSID().c_str());
    Serial.printf("IP Asignada:     %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Máscara de red:  %s\n", WiFi.subnetMask().toString().c_str());
    Serial.printf("Puerta de enlace:%s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("DNS Primario:    %s\n", WiFi.dnsIP().toString().c_str());
    Serial.printf("Dirección MAC:   %s\n", WiFi.macAddress().c_str());
    Serial.printf("Potencia (RSSI): %d dBm\n", WiFi.RSSI());
    Serial.printf("Canal Wi-Fi:     %d\n", WiFi.channel());
    Serial.printf("Tiempo conex.:   %lu ms\n", millis() - connectionStartTime);
    Serial.println(F("========================================\n"));
}

void startWifiConnection() {
    Serial.println(F("\nIniciando conexión Wi-Fi..."));
    Serial.printf("Conectando a SSID: \"%s\"\n", WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    connectionStartTime = millis();
    Serial.print(F("Esperando enlace"));
}

void setup() {
    Serial.begin(115200);
    delay(200);

    printChipInfo();

    // Inicializar pantalla TFT ILI9341
    Serial.println(F("Inicializando pantalla TFT ILI9341 SPI..."));
    tft.begin();
    tft.setRotation(3); // Orientación horizontal (320x240)
    showInitialScreen();

    // Iniciar Wi-Fi
    startWifiConnection();
}

void loop() {
    bool isConnected = (WiFi.status() == WL_CONNECTED);
    static uint16_t connectingStep = 0;

    // Detección de cambios de estado
    if (isConnected && !wasConnected) {
        wasConnected = true;
        printWifiDetails();
        drawConnectedScreen();
    } else if (!isConnected && wasConnected) {
        wasConnected = false;
        Serial.println(F("\n[ALERTA] ¡Conexión Wi-Fi perdida!"));
        Serial.println(F("Reintentando conexión automáticamente..."));
        connectionStartTime = millis();
        drawDisconnectedScreen();
    }

    // Progreso mientras conecta
    if (!isConnected) {
        static unsigned long lastStep = 0;
        if (millis() - lastStep >= 500) {
            lastStep = millis();
            Serial.print(F("."));
            updateConnectingScreen(connectingStep++);
        }
    } else {
        // Telemetría periódica en pantalla y serial
        if (millis() - lastScreenUpdate >= SCREEN_UPDATE_INTERVAL) {
            lastScreenUpdate = millis();
            refreshTelemetry();
        }

        if (millis() - lastStatusPrint >= STATUS_PRINT_INTERVAL) {
            lastStatusPrint = millis();
            Serial.printf("[Heartbeat] Wi-Fi OK | IP: %s | RSSI: %d dBm | RAM libre: %u bytes\n",
                          WiFi.localIP().toString().c_str(),
                          WiFi.RSSI(),
                          ESP.getFreeHeap());
        }
    }

    delay(10);
}
