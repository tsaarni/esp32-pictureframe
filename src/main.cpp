//
// Copyright 2019 tero.saarni@gmail.com
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <SPIFFS.h>

// https://github.com/Hieromon/AutoConnect
#include <AutoConnect.h>

// https://github.com/ZinggJM/GxEPD
#include <GxEPD.h>
#include <GxGDEW042T2/GxGDEW042T2.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

// https://github.com/elanthis/upng
#include <upng.h>

// https://github.com/bblanchon/ArduinoJson
#include <ArduinoJson.h>


// AutoConnect portal
WebServer Server(80);
AutoConnect Portal(Server);
AutoConnectConfig PortalConfig;


// configuration file
const char* config_file = "/config.json";
StaticJsonDocument<512> config;


// buffer for image download
uint8_t download_buf[7*1024];


// e-paper display driver https://github.com/ZinggJM/GxEPD
// BUSY -> 4, RST -> 16, DC -> 17, CS -> SS(5), CLK -> SCK(18), DIN -> MOSI(23), GND -> GND, 3.3V -> 3.3V
GxIO_Class io(SPI, /*CS*/ 5, /*DC*/ 17, /*RST*/ 16);
GxEPD_Class display(io, /*RST*/ 16, /*BUSY*/ 4);
#include <Fonts/FreeMonoBold12pt7b.h>


size_t download_image(const char* url, uint8_t* buf)
{
    Serial.printf("Downloading image from %s\n", url);

    HTTPClient http;
    http.begin(url);

    if (http.GET() == 200)
    {
        WiFiClient stream = http.getStream();
        int len = http.getSize();

        while (http.connected() && (len > 0))
        {
            size_t bytes_to_read = stream.available();

            if (bytes_to_read)
            {
                Serial.printf("Reading %d bytes\n", bytes_to_read);
                int c = stream.readBytes(buf, bytes_to_read);

                if (len > 0)
                {
                    len -= c;
                    buf += c;
                }
            }
        }
        return http.getSize();
    }
    else
    {
        Serial.printf("HTTP error code=%d\n", http.GET());
    }

    return 0;
}


const uint8_t* decode_image(const uint8_t* buf, size_t len)
{
    Serial.printf("Decoding image of %d bytes\n", len);

    upng_t* upng;
    upng = upng_new_from_bytes(buf, len);

    if (upng)
    {
        upng_header(upng);
        Serial.printf("Image dimensions: width=%d, height=%d, bpp=%d\n", upng_get_height(upng), upng_get_height(upng), upng_get_bpp(upng));

        upng_decode(upng);

        if (upng_get_error(upng) == UPNG_EOK)
        {
            Serial.println("Decode ok");
            return upng_get_buffer(upng);
        }
        else
        {
            Serial.printf("Decoding failed, line_number=%d\n", upng_get_error_line(upng));
        }
    }
    return 0;
}


bool display_captive_portal_instructions(IPAddress softapIP) {
    Serial.println("Captive portal started, SoftAP IP:" + softapIP.toString());

    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(0, 50);
    display.println("ERROR:");
    display.println("Cannot connect to WIFI");
    display.println("");
    display.println("Connect to portal to");
    display.println("configure");
    display.println("");
    display.print("SSID: ");
    display.println(static_cast<const char*>(config["wifi_ap_mode"]["ssid"]));
    display.print("Passphrase: ");
    display.println(static_cast<const char*>(config["wifi_ap_mode"]["pass"]));
    display.update();

    return true;
}


bool read_config()
{
    if (!SPIFFS.begin(true)){
        Serial.println("Cannot mount SPIFFS");
        return false;
    }

    File file = SPIFFS.open(config_file);
    if (!file)
    {
        Serial.printf("Could not open %s\n", config_file);
        return false;
    }

    DeserializationError error = deserializeJson(config, file);
    if (error)
    {
        Serial.printf("Failed to deserialize JSON config file: %s\n", error.c_str());
    }

    return !error;
}


bool wifi_config()
{
    // start wifi or if we cannot connect, start wifi in AP mode and
    // initalize captive portal for dynamic wifi configuration
    PortalConfig.autoReconnect = true;
    PortalConfig.portalTimeout = 5 * 60 * 1000; // 5 min
    PortalConfig.apid = static_cast<const char*>(config["wifi_ap_mode"]["ssid"]);
    PortalConfig.psk = static_cast<const char*>(config["wifi_ap_mode"]["pass"]);
    Portal.config(PortalConfig);
    Portal.onDetect(display_captive_portal_instructions);
    return Portal.begin();
}


void setup() {

    Serial.begin(115200);

    // initialize SPI for the display
    SPI.begin(/*SCK*/18, /*MISO*/-1, /*MOSI*/23, /*SS*/5);
    display.init(115200);

    if (read_config() == false)
    {
        Serial.println("Could not read configuration");
        return;
    }

    if (wifi_config() == false)
    {
        Serial.println("Wifi was not configured during timeout period of autoconfig portal, shutting down");
        esp_deep_sleep_start();
    }


    Serial.println("Wifi started, got IP:" + WiFi.localIP().toString());

    // download and decode image to be displayed
    size_t len = download_image(config["image_url"], download_buf);
    if (len == 0)
    {
        Serial.println("Download failed");
        return;
    }

    const uint8_t* image_buf = decode_image(download_buf, len);
    if (!image_buf)
    {
        Serial.println("Got nothing to display");
        return;
    }

    Serial.println("Start drawing image on display");
    delay(100);
    display.drawBitmap(image_buf, 400*300/8); // size of the bitmap is hardcoded for 400*300 image, 1 bit per pixel

    Serial.printf("Entering deep sleep for %d seconds", static_cast<int>(config["refresh_period_sec"]));
    delay(100);
    esp_sleep_enable_timer_wakeup(static_cast<int>(config["refresh_period_sec"]) * 1000000);
    esp_deep_sleep_start();
}


void loop() {
    // we should never get up to here
    sleep(1000);
}
