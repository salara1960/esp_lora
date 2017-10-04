# esp_lora

esp32 + ssd1306 + lora module (HM-TRLR-S-868)

# Файлы пакета:

* sdkconfing	- файл конфигурации проекта

* Makefile	- make файл (файл сценария компиляции проекта)

* version	- файл версии ПО

* README.md	- файл справки

* main/		- папка исходников и сертификатов


Требуемые компоненты:
```
- Cross compiler xtensa-esp32-elf (http://esp-idf-fork.readthedocs.io/en/stable/linux-setup.html#step-0-prerequisites)
- SDK esp-idf (https://github.com/espressif/esp-idf)
- Python2 (https://www.python.org/)
```

# Компиляция и загрузка

make menuconfig - конфигурация проекта

make app	- компиляция проекта

make flash	- запись бинарного кода проекта в dataflash


