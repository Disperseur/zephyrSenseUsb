# Projet capteur USB

## TODO

## Remarques diverses

Temps min pour avoir les valeurs de tout les capteurs : 55ms (environ). Donc frequence max : 18Hz
Ce qui prends le plus de temps dans le noeud de mesures: les mesures. Donc mettre le post traitement des donnees ailleurs n'améliorerait rien. Il faut revoir l'aquisition sans utiliser l'api de zephyr et utiliser directement les commandes i2c.

## Commandes

### Compiler (sous WSL)
````bash
west build -p -b arduino_nano_33_ble/nrf52840/sense
````


### Flasher (sous Windows)
````powershell
"C:\Users\chass\AppData\Local\Arduino15\packages\arduino\tools\bossac\1.9.1-arduino2/bossac.exe" -d --port=COM4 -U -i -e -w "\\wsl.localhost\Ubuntu\home\antoine\zephyrSenseUsb\build\zephyr\zephyr.bin" -R
````