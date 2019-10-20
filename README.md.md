# Microcontroller part for the serial to mqtt communication
### LM4F120 Launchpad 

 - Framework = Arduino
- IDE = Visual Code + PlatformIO
<img src="https://github.com/vortex314/mqtt2serial/raw/master/doc/stellaris.jpeg" width="200" title="Stellaris Launchpad 5$">
The Launchpad board only needs one USB connector as the LM4F120 used as programmer exposes an ICDI port and a CDC serial port.
```
$ dmesg
[Do Okt 17 19:59:28 2019] usb 1-1.2.3: new full-speed USB device number 26 using ehci-pci
[Do Okt 17 19:59:28 2019] usb 1-1.2.3: New USB device found, idVendor=0483, idProduct=5740
[Do Okt 17 19:59:28 2019] usb 1-1.2.3: New USB device strings: Mfr=1, Product=2, SerialNumber=3
[Do Okt 17 19:59:28 2019] usb 1-1.2.3: Product: Maple Mini CDC in FS Mode
[Do Okt 17 19:59:28 2019] usb 1-1.2.3: Manufacturer: STMicroelectronics
[Do Okt 17 19:59:28 2019] usb 1-1.2.3: SerialNumber: 8D8134785748
[Do Okt 17 19:59:28 2019] cdc_acm 1-1.2.3:1.0: ttyACM2: USB ACM device
```

### maple mini LeafLabs 
- Framework = Arduino
- IDE = Visual Code + PlatformIO
This is a maple leaflabs board, programmed through a STLINKV2 clone. Parameters need to be changed in platformio.ini if the maple board is used with the inbuild bootloader. 
The USB connection will present itself as an USB CDC device : 
```
Bus 001 Device 021: ID 0483:5740 STMicroelectronics STM32F407
```
<img src="https://github.com/vortex314/mqtt2serial/raw/master/doc/maple.jpg" width="200" title="Maple mini 4$">
### nrf51822

<img src="https://github.com/vortex314/mqtt2serial/raw/master/doc/nrf51822.jpeg" width="200" >

### MQTT SPY screenshot
![maple mini](https://github.com/vortex314/mqtt2serial/raw/master/doc/mqtt-spy.png)

### Test setup 

5 different devices and 1 serial2mqtt handling all  communication.
<img src="https://github.com/vortex314/mqtt2serial/raw/master/doc/ucs.jpg" width="300" title="Stellaris Launchpad 5$">

Top to bottom :
- nrf51822
- esp32 - flashing a stm32 maple
- esp8266
- Stellaris Launchpas lm4f120h5qr
- maple - stm32f103

