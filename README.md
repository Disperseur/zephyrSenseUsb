# Projet capteur USB

## TODO
- ringbuffer mode:
	- enregistrement (v1 mais peut etre pb de poiteur lors du stockage)
	- get ringbuffer


## Compiler

````bash
west build -p -b arduino_nano_33_ble/nrf52840/sense
````


## Flasher
````powershell
"C:\Users\chass\AppData\Local\Arduino15\packages\arduino\tools\bossac\1.9.1-arduino2/bossac.exe" -d --port=COM4 -U -i -e -w "\\wsl.localhost\Ubuntu\home\antoine\zephyrSenseUsb\build\zephyr\zephyr.bin" -R
````





## Autres
Peut etre que a un moment tu auras besoin d'ajouter ce genre de ligne au .conf:

CONFIG_LPS22HB=y




Televerser sur la arduino_nano_33_ble sense rev2 :

- compiler normalemen sur wsl
- executer la commande suivante apres avoir:
	- pressé deux fois le reset pour se mettre en reception de binaire
	- verifié le nom du port COM
	- verifié le chemin du binaire




Probablement qu'un petit script ps1 serait le bienvenu...



calcul de l'altitude : 44330 * ( 1 - pow(data.pressure/101.325, 1/5.255) );