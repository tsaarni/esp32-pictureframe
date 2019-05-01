
# ESP32 picture-frame

Downloads PNG image from the network and displays it on e-ink / e-paper display.
An example of a service that generates suitable image is [here](https://github.com/tsaarni/weather-pictureframe-functions).  

The project uses WaveShare 4.2" 400x300 display described [here](https://www.waveshare.com/wiki/4.2inch_e-Paper_Module). 
It can be purchased [here](https://waveshare.aliexpress.com/store/407494).

![image](https://i.imgur.com/RZB5srZ.jpg)

## Credits

* PNG decoding uses [uPNG](https://github.com/elanthis/upng)
* [GxEPD](https://github.com/ZinggJM/GxEPD) e-paper display driver, depends on [Adafruit_GFX](https://github.com/adafruit/Adafruit-GFX-Library)
* Web interface for initial WLAN configuration using [AutoConnect](https://github.com/Hieromon/AutoConnect)
