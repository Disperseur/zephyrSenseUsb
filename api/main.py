"""
Script de test de la classe
"""

from SenseUSB import *
from time import sleep

sensor_env_motion = SenseUSB("/dev/ttyACM1")
sensor_location = serial.Serial("/dev/ttyACM0", baudrate=115200)

while(1):
    print(sensor_env_motion.get_data_oneshot())
    print(sensor_location.readline().decode())
    sleep(1)