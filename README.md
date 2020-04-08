# Arduino code mqtt over serial

## Synopsis
This code enables Arduino based microcontrollers to use a serial port communication to send MQTT publish information to topics and subscribe to MQTT topics.
This requires to have serial2mqtt (Linux X86 or ARM -Raspberry Pi ) run on the host side.
Only QOS level 0 used.
### Conventions used in the code 

 - MQTT Publish happens on **src/device/object/property** 
 - MQTT Subscribe happens on **dst/device/#** 
 - The **device** part is also the internal hostname. It can be configured via the options (-DHOSTNAME ) in platformio.ini.
```c++
ValueFlow<uint32_t> x;
x.pass(true);
x == mqtt.topic<uint32_t>("object1/x")
x=1; // will publish the value on MQTT 
```

Publishing topic=dst/device/object1/x , message=123 will result that x becomes 123 in the Arduino.

Most basic type are supported : string, bool, float, integer types as it uses ArduinoJson variants.
Complex JSON object types need to be composed by the application itself.
See also : https://github.com/vortex314/nanoAkka/blob/master/README.md


## Build it
 - Framework = Arduino
- IDE = Visual Code + PlatformIO

```Shell
git clone https://github.com/vortex314/mqtt2serial.git
git clone https://github.com/vortex314/nanoAkka.git
git clone https://github.com/vortex314/Common.git
git clone https://github.com/bblanchon/ArduinoJson
cd mqtt2serial
```
- use visual code with platformio extension to open any of the projects within mqtt2serial
 - compile and download to controller, check out the ports used in the platformio.ini. Could differ on your system
## Run it
- check if the serial port is sending out JSON arrays to subscribe and do loopback tests. Use minicom or the in-build terminal of platformio
- then activate serial2mqtt , adapt the serial2mqtt.json file for the correct ports. Don't forget to close the connection from the previous step.
```
cd ../serial2mqtt
cd build
unzip serial2mqtt.x86_64.zip # for Linux 64bit
cd ..
# check serial2mqtt.json for the correct settings
build/Debug/serial2mqtt
```
Have fun.
## Implementation details
- The Arduino sends out continously every x seconds a loopback message on which it also subscribes : *dst/device/system/loopback:true*
- This enables to detect if the there is an E2E connection with the broker.
- If no response is received it will send out also repetitively the subscribe command to *dst/device/#*
- As long as there is no loopback and subscribe established, it will not publish any further topics. 
- The LED of the board is used to indicate this connection status, it blinks slow if there is connection to MQTT, it blinks fast when the connection is not established or gone. 
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
eyJoaXN0b3J5IjpbMjI2Mzg0NzY1LC0xODc5NTUyNzgsLTkxMj
YzNzI0MSwxNjMzODExMzc4LDE4MDk4MjkzNzksLTgxNDM1MzMx
LC00OTQzNTY2MjYsMTQzNzI5NTM1NiwtODEwNzAwMTQ5LDE2Mj
g5ODY1MTUsMjA5MTYyMDE3OF19
-->