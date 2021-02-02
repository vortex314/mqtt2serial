# Bluetooth serial
A small attempt at using Bluetooth as serial comm port for MQTT
## Linux serial port over bluetooth
https://medium.com/@18218004/devlog-6-bluetooth-and-esp32-ba076a8e207d
## serial2mqtt over bluetooth
Config file. 
```
{
    "mqtt": {
        "connection": "tcp://limero.ddns.net:1883"
    },
    "serial": {
        "baudrate": 115200,
        "ports": [
            "/dev/rfcomm0"
        ],
        "protocol": "jsonArray"
    },
    "log": {
        "protocol": true,
        "debug": true,
        "useColors": true,
        "mqtt": false,
        "program": false,
        "level": "I",
        "file": "log.serial2mqtt.",
        "console": true
    }
}
```