# LM4F120
needed some tweeking in include files for sleep() function
not stdc++11
needed to include printf.h
# Recommended
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core/master/scripts/99-platformio-udev.rules | sudo tee /etc/udev/rules.d/99-platformio-udev.rules

# OR, manually download and copy this file to destination folder
sudo cp 99-platformio-udev.rules /etc/udev/rules.d/99-platformio-udev.rules
sudo service udev restart

https://docs.platformio.org/en/latest/faq.html#platformio-udev-rules