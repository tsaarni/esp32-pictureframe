#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <AutoConnect.h>
#include <HTTPClient.h>

#include <GxEPD.h>
#include <GxGDEW042T2/GxGDEW042T2.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include "upng.h"

WebServer Server(80);
AutoConnect Portal(Server);
AutoConnectConfig Config("", "pictureframe");

// buffer for image download
uint8_t image[7*1024];

// e-paper display driver https://github.com/ZinggJM/GxEPD
// BUSY -> 4, RST -> 16, DC -> 17, CS -> SS(5), CLK -> SCK(18), DIN -> MOSI(23), GND -> GND, 3.3V -> 3.3V
GxIO_Class io(SPI, /*CS*/ 5, /*DC*/ 17, /*RST*/ 16);
GxEPD_Class display(io, /*RST*/ 16, /*BUSY*/ 4);

#include <Fonts/FreeMonoBold9pt7b.h>


size_t download_image(uint8_t* buf)
{
    Serial.println("Downloading image");

    HTTPClient http;
    http.begin("INSERT DOWNLOAD URL HERE");

    if (http.GET() == 200)
    {
        WiFiClient stream = http.getStream();
        int len = http.getSize();

        while (http.connected() && (len > 0))
        {
            size_t bytes_to_read = stream.available();

            if (bytes_to_read)
            {
                Serial.print("Reading ");
                Serial.print(bytes_to_read);
                Serial.println(" bytes...");
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
        Serial.print("Failed to download:");
        Serial.println(http.GET());
    }

    return 0;
}


const uint8_t* decode_image(uint8_t* buf, size_t len)
{
    Serial.print("Decoding image: ");
    Serial.print(len);
    Serial.println(" bytes");

    upng_t* upng;
    upng = upng_new_from_bytes(buf, len);

    if (upng)
    {
        upng_header(upng);

        int width  = upng_get_width(upng);
        int height = upng_get_height(upng);

        Serial.print("Image dimensions: width=");
        Serial.print(width);
        Serial.print(", height=");
        Serial.print(height);
        Serial.print(", bpp=");
        Serial.print(upng_get_bpp(upng));

        upng_decode(upng);

        if (upng_get_error(upng) == UPNG_EOK)
        {
            Serial.println("Decode ok");
            return upng_get_buffer(upng);
        }
        else
        {
            Serial.print("Decoding failed, line_number: ");
            Serial.println(upng_get_error_line(upng));
        }
    }
    return 0;
}


void setup() {
    Serial.begin(115200);

    Config.autoReconnect = true;
    Portal.config(Config);
    Portal.begin();

    size_t len = download_image(image);
    if (len == 0)
    {
        Serial.println("Download failed");
    }
 
    const uint8_t* buf = decode_image(image, len);

    delay(100);
    SPI.begin(/*SCK*/18, /*MISO*/-1, /*MOSI*/23, /*SS*/5);
    display.init(115200);
    display.drawBitmap(buf, 15000);
}


void loop() {
    Portal.handleClient();
    // esp_sleep_enable_timer_wakeup(5 * 1000000);
    // esp_deep_sleep_start();
}

