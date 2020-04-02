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
- simple : most Basic Arduino example
- 
<!--stackedit_data:
eyJoaXN0b3J5IjpbLTk1Nzk1NjU0MV19
-->