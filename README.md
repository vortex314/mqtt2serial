# Arduino code mqtt over serial

## Synopsis

## Build process ( draft )
 - Framework = Arduino
- IDE = Visual Code + PlatformIO

```Shell
git clone https://github.com/vortex314/mqtt2serial.git
git clone https://github.com/vortex314/nanoAkka.git
git clone https://github.com/vortex314/Common.git
git clone https://github.com/bblanchon/ArduinoJson
cd mqtt2serial
```
- use visual code with platformio extension to open any of the projects
 - compile and dowload to controller, check out the ports used in the platformio.ini. Could differ on your system
- check if the serial port is sending out JSON arrays to subscribe and do loopback tests. Use minciom or the in-build terminal of platformio
- then activate serial2mqtt , adapt the serial2mqtt.json file for the correct ports
```
cd ..\serial2mqtt
cd build
unzip serial2mqtt.x86_64.zip

unzip
cd ..
### LM4F120 Launchpad 
<img src="https://github.com/vortex314/mqtt2serial/raw/master/doc/stellaris.jpeg" width="200" title="Stellaris Launchpad 5$">

The Launchpad board only needs one USB connector as the LM4F120 used as programmer exposes an ICDI port and a CDC serial port.

### maple mini LeafLabs 
This is a maple leaflabs board, programmed through a STLINKV2 clone. Parameters need to be changed in platformio.ini if the maple board is used with the inbuild bootloader. 
The USB connection will present itself as an USB CDC device : 
```
Bus 001 Device 021: ID 0483:5740 STMicroelectronics STM32F407
```
<img src="https://github.com/vortex314/mqtt2serial/raw/master/doc/maple.jpg" width="200" title="Maple mini 4$">

### nrf51822
<img src="https://github.com/vortex314/mqtt2serial/raw/master/doc/nrf51822.jpeg" width="200" >

### ESP32
<img src="https://github.com/vortex314/mqtt2serial/raw/master/doc/esp32.jpg" width="200" >

### ESP8266
<img src="https://github.com/vortex314/mqtt2serial/raw/master/doc/esp8266.jpeg" width="200" >

### MQTT SPY screenshot

<img src="https://github.com/vortex314/mqtt2serial/raw/master/doc/mqtt-spy.png" width="200" >

### Test setup 

5 different devices and 1 serial2mqtt handling all  communication.

<img src="https://github.com/vortex314/mqtt2serial/raw/master/doc/ucs.jpg" width="300" >

Top to bottom :
- nrf51822 on a nrf51822 evaluation kit board with stlink clone
- esp32 - flashing a stm32 maple
- esp8266
- Stellaris Launchpas lm4f120h5qr
- maple - stm32f103 with stlink clone

### Folder structure 
- simple : most Basic Arduino example. Just a serial println publishing a topic. 
- subscribe ( TBC ) : subscribe to a topic and handle messages. 
- Serial Mqtt full example , it has more features :
  * it will detect a failing connection
  * it sends out a loopback message to itself, to check the E2E connectivity with MQTT
  * it automatically resubscribes to his own destination based on hostname
  * it stops all publishes until a connection is reliably present
  *  it's reactive stream driven based on [nanoAkka](https://github.com/vortex314/nanoAkka/blob/master/README.md) 
  * it should work on any Arduino having a serial or USB CDC connection. Anyway on those in my possession

<!--stackedit_data:
eyJoaXN0b3J5IjpbLTEzMzQ3MTY1NzIsLTgxMDcwMDE0OSwxNj
I4OTg2NTE1LDIwOTE2MjAxNzhdfQ==
-->