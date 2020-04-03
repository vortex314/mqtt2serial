# Arduino code mqtt over serial

 - Framework = Arduino
- IDE = Visual Code + PlatformIO

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
eyJoaXN0b3J5IjpbMjA5MTYyMDE3OCwtMzQ0ODg0MzM4LDk0Mj
AxNjc3LC02MTgzNzQyNTAsLTEyOTM5MDg3NjAsMTk3NzkzNDY3
OV19
-->