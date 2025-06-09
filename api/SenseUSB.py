import serial
from time import sleep


# Reponses capteur
SENSOR_ACK  = "ACK\r\n"
SENSOR_DONE = "DONE\r\n"


STATUS =    ["RUNNING", "STOPPED", "FAULT"]
MODES =     ["STREAMING", "ONESHOT", "RINGBUFFER"]
SENSORS =   ["NONE", "TEMPERATURE", "PRESSURE", "HUMIDITY", "ALTITUDE", "ACCELERATION", "GYROSCOPE", "ALL", "ENV", "RINGBUFFER"]


class SenseUSB:
    def __init__(self, portname, baudrate=115200):
        print("Connecting...")
        self.port = serial.Serial(portname, baudrate)
        print("Successfully connected.")
        
        
        self.status = "STOPPED"
        self.mode = "STREAMING"
        self.sensors = "ALL"
        

    def wait(self, str, timeout=1000):
        """
        Waits for str to be given by the sensor
        Blocking function
        timeout in ms
        """
        output = ""
        counter = 0

        while(output != str):
            if(counter == timeout):
                return -1

            output = self.port.readline().decode()
            counter += 1
            sleep(0.001)
        
        return 1


    def set_mode(self, mode):
        assert(mode in MODES) # le mode choisi doit faire partie des modes possibles
        assert(self.status == "STOPPED") # bloque le changement de mode en cours de mesure. Une telle action est aussi bloque sur la carte
        
        self.port.write(f"SET MODE {mode}\n".encode())

        assert(self.port.readline().decode() == SENSOR_ACK) # hanshake avec le capteur
        assert(self.port.readline().decode() == SENSOR_DONE)
        self.mode = mode

    def set_sensors(self, sensors):
        assert(sensors in SENSORS)
        assert(self.status == "STOPPED")

        self.port.write(f"SET SENSORS {sensors}\n".encode())

        assert(self.port.readline().decode() == SENSOR_ACK) # hanshake avec le capteur
        assert(self.port.readline().decode() == SENSOR_DONE)
        self.sensors = sensors

    def set_speed(self, speed):
        assert(type(speed) == int)
        assert(speed > 55) # maximum speed is 55ms delay between each measure due to the time needed to get the sensor's measurments
        assert(self.status == "STOPPED") # bloque le changement de speed en cours de mesure

        self.port.write(f"SET SPEED {speed}\n".encode())

        assert(self.port.readline().decode() == SENSOR_ACK)
        assert(self.port.readline().decode() == SENSOR_DONE)

    def set_trigger1(self, sensor, comp, floor):
        """
        floor est la valeur en milliers. Exemple : TEMPERATURE SUP 30000 signifie temperature superieure a 30.000 deg
        """
        assert(self.status == "STOPPED")
        assert(sensor in SENSORS)
        assert(comp in ["SUP", "INF"])
        assert(type(floor) == int)
        assert(floor > 0) # a voir si on retire ou pas


        self.port.write(f"SET TRIGGER1 OFF\n".encode())
        assert(self.port.readline().decode() == SENSOR_ACK)
        assert(self.port.readline().decode() == SENSOR_DONE)

        self.port.write(f"SET TRIGGER1 SENSOR {sensor}\n".encode())
        assert(self.port.readline().decode() == SENSOR_ACK)
        assert(self.port.readline().decode() == SENSOR_DONE)

        self.port.write(f"SET TRIGGER1 COMP {comp}\n".encode())
        assert(self.port.readline().decode() == SENSOR_ACK)
        assert(self.port.readline().decode() == SENSOR_DONE)

        self.port.write(f"SET TRIGGER1 FLOOR {floor}\n".encode())
        assert(self.port.readline().decode() == SENSOR_ACK)
        assert(self.port.readline().decode() == SENSOR_DONE)

    def set_trigger1_on(self):
        assert(self.status == "STOPPED")

        self.port.write(f"SET TRIGGER1 ON\n".encode())
        assert(self.port.readline().decode() == SENSOR_ACK)
        assert(self.port.readline().decode() == SENSOR_DONE)

    def set_trigger1_off(self):
        assert(self.status == "STOPPED")

        self.port.write(f"SET TRIGGER1 OFF\n".encode())
        assert(self.port.readline().decode() == SENSOR_ACK)
        assert(self.port.readline().decode() == SENSOR_DONE)
    
        


    def start(self):
        self.port.write("START\n".encode())
        assert(self.port.readline().decode() == SENSOR_ACK)
        assert(self.port.readline().decode() == SENSOR_DONE)
        self.status = "RUNNING"

    def stop(self):
        assert(self.status == "RUNNING")
        self.port.write("STOP\n".encode())
        
        assert(self.wait(SENSOR_ACK) == 1)
        assert(self.wait(SENSOR_DONE) == 1)
        
        self.status = "STOPPED"



    def get_status(self):
        self.port.write("GET STATUS\n".encode())

        assert(self.port.readline().decode() == SENSOR_ACK)
        return self.port.readline().decode()[:-1].split()[1]

    def get_mode(self):
        self.port.write("GET MODE\n".encode())

        assert(self.port.readline().decode() == SENSOR_ACK)
        return self.port.readline().decode()[:-1].split()[1]
    
    def get_sensors(self):
        self.port.write("GET SENSORS\n".encode())
        assert(self.port.readline().decode() == SENSOR_ACK)
        return self.port.readline().decode()[:-1].split()[1]
    
    def get_speed(self):
        self.port.write("GET SPEED\n".encode())

        assert(self.port.readline().decode() == SENSOR_ACK)
        return int(self.port.readline().decode()[:-1].split()[1])
        
    def get_ringbuffer(self):
        self.port.write("GET RINGBUFFER\n".encode())
        assert(self.port.readline().decode() == SENSOR_ACK)

        # recuperation de la taille du buffer (non modifiable, constante du soft embarque)
        size = int(self.port.readline().decode()[:-1].split()[1]) * 6 # 6 capteurs differents

        ringbuffer = []
        # recuperation des mesures
        for i in range(size):
            measure = self.port.readline().decode()[:-1].split()
            # print(measure)

            if(measure[2] in ["ACCEL_LIN", "ACCEL_ROT"]):
                ringbuffer.append([ int(measure[0]),
                                    int(measure[1]),
                                    measure[2],
                                    int(measure[3]),
                                    int(measure[4]),
                                    int(measure[5])])
            else:
                ringbuffer.append([ int(measure[0]),
                                    int(measure[1]),
                                    measure[2],
                                    int(measure[3])])
        return ringbuffer
    

    # get trigger1
    def get_trigger1(self):
        self.port.write("GET TRIGGER1\n".encode())
        assert(self.port.readline().decode() == SENSOR_ACK)

        trigger1 = []

        for i in range(4):
            trigger1.append(self.port.readline().decode()[:-1].split()[-1])

        trigger1[-1] = int(trigger1[-1]) #conversion du seuil du trigger en entier

        return trigger1






















    # def print_data_oneshot(self, sensor, n=1):
    #     assert(sensor in SENSORS)
    #     assert(n>=1)

    #     self.set_mode("ONESHOT")

    #     for i in range(n):
    #         self.port.write(f"GET {sensor}\n".encode())
    #         k = 1

    #         if(sensor == "ALL"): k = 7
    #         elif(sensor == "ENV"): k = 3
    #         elif(sensor == "MOTION"): k = 3
    #         else: k = 1


    #         for j in range(k):
    #             print(self.port.readline().decode().split())



    # def get_data_oneshot(self, sensor, n=1):
    #     assert(sensor in SENSORS)
    #     assert(n>=1)
    #     assert(self.status == "RUNNING") # sensor must be started using .start() in order to get data
    #     assert(self.mode == "ONESHOT") # sensor must be in oneshot mode using .set_mode("ONESHOT")

    #     # self.set_mode("ONESHOT")

    #     output = []

    #     for i in range(n):
    #         self.port.write(f"GET {sensor}\n".encode())
    #         k = 1

    #         if(sensor == "ALL"): k = 7
    #         elif(sensor == "ENV"): k = 3
    #         elif(sensor == "MOTION"): k = 3
    #         else: k = 1


    #         for j in range(k):
    #             raw_values_list = self.port.readline().decode().split()
    #             values_list = [float(raw_values_list[0]), raw_values_list[1], float(raw_values_list[2])]

    #             output.append(values_list)

    #     return output
    

    # def start_record(self):
    #     self.port.write("SET MODE LOG\n".encode())
    #     assert(self.port.readline().decode() == SENSOR_ACK)
    #     self.start()
    
    # def stop_record(self):
    #     self.stop()

    #  