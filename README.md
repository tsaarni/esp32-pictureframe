
# ESP32 picture-frame

Downloads PNG image from the network and displays it on e-ink /
e-paper display.
An example of a service that generates suitable image is [here](https://github.com/tsaarni/weather-pictureframe-functions).

The project uses ESP32 and WaveShare 4.2" 400x300 display described
[here](https://www.waveshare.com/wiki/4.2inch_e-Paper_Module).
The display can be purchased
[here](https://waveshare.aliexpress.com/store/407494).

![image](https://i.imgur.com/RZB5srZ.jpg)


## Building

The project uses [PlatformIO](https://platformio.org/) as the development environment.


### Configure

Create configuration file under `data` subdirectory of the the source
code repository and configuration file

    mkdir -p data
    cat > data/config.json <<EOF
    {
        "wifi_ap_mode": {
            "ssid": "pictureframe",
            "pass": "esp32frame"
        },
        "image_url": "IMAGE_DOWNLOAD_URL",
        "refresh_period_sec": 600
    }
    EOF

Note that passphrase must be 8 characters or longer, otherwise SoftAP
will fail to initialize correctly and the SSID and PSK for the AP will
be corrupted.

Next, generate and upload SPIFF filesystem out of the contents of
`data` directory

    pio run -t uploadfs


### Compile and flash

Run following commands to compile and flash the program:

    pio run -t upload

    # run to see debug messages
    pio device monitor -b 115200


## Credits

* PNG decoding uses [uPNG](https://github.com/elanthis/upng)
* [GxEPD](https://github.com/ZinggJM/GxEPD) e-paper display driver, depends on [Adafruit_GFX](https://github.com/adafruit/Adafruit-GFX-Library)
* Web interface for initial WLAN configuration using [AutoConnect](https://github.com/Hieromon/AutoConnect)
* JSON deserialization with [ArduinoJSON](https://github.com/bblanchon/ArduinoJson)
