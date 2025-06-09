"""
Script de test de la classe
"""

from SenseUSB import *
from time import sleep

sensor = SenseUSB("/dev/ttyACM0")


# # commandes ok
# sensor.set_mode("STREAMING")
# print(sensor.get_mode())

# print(sensor.get_status())

# sensor.set_speed(100)
# print(sensor.get_speed())

# sensor.set_sensors("TEMPERATURE")
# print(sensor.get_sensors())

# sensor.start()

# sleep(2)

# sensor.stop()

# print(sensor.get_ringbuffer())

# sensor.set_trigger1("HUMIDITY", "SUP", 50000)
# sensor.set_trigger1_on()
# sensor.set_trigger1_off()

# print(sensor.get_trigger1())



sensor.set_speed(100)
sensor.set_mode("RINGBUFFER")
sensor.start()
sleep(2)
sensor.stop()
buffer = sensor.get_ringbuffer()

for l in buffer:
    print(l)


# commandes en test

