import serial

# Reponses capteur
SENSOR_ACK  = "ACK\r\n"
SENSOR_DONE = "DONE\r\n"


STATUS = ["RUNNING", "STOPPED", "FAULT"]
MODES = ["STREAMING", "ONESHOT", "RINGBUFFER"]
SENSORS = ["NONE", "TEMPERATURE", "PRESSURE", "HUMIDITY", "ALTITUDE", "ACCELERATION", "GYROSCOPE", "ALL", "ENV", "RINGBUFFER"]


class SenseUSB:
    def __init__(self, portname, baudrate=115200):
        self.port = serial.Serial(portname, baudrate)
        
        self.status = "STOPPED"
        self.mode = "STREAMING"
        self.sensors = "ALL"
        

    def set_mode(self, mode):
        assert(mode in MODES) # le mode choisi doit faire partie des modes possibles
        assert(self.status == "STOPPED") # bloque le changement de mode en cours de mesure
        
        self.port.write(f"SET MODE {mode}\n".encode())
        assert(self.port.readline().decode() == SENSOR_ACK) # hanshake avec le capteur
        self.mode = mode

    def set_speed(self, speed):
        """
        Etant donne la quantite de traitements a realiser dans le mode run, il y a une resolution temporelle minimale de environ 100 ms
        """
        assert(type(speed) in [float, int])
        assert(speed > 0)
        assert(self.status == "STOPPED") # bloque le changement de speed en cours de mesure

        self.port.write(f"SET SPEED {speed}\n".encode())
        assert(self.port.readline().decode() == SENSOR_ACK)
        
        
    def start(self):
        self.port.write("START\n".encode())
        assert(self.port.readline().decode() == SENSOR_ACK)
        assert(self.port.readline().decode() == INIT_SENSOR_INIT)
        assert(self.port.readline().decode() == INIT_SENSOR_OK)
        self.status = "RUNNING"

    def stop(self):
        self.port.write("STOP\n".encode())
        assert(self.port.readline().decode() == SENSOR_ACK)
        assert(self.port.readline().decode() == SENSOR_DONE)
        self.status = "STANDBY"

    def get_mode(self):
        self.port.write("GET MODE\n".encode())
        return self.port.readline().decode()[:-1]
    
    def get_status(self):
        self.port.write("GET STATUS\n".encode())
        return self.port.readline().decode()[:-1]
    
    def get_speed(self):
        self.port.write("GET SPEED\n".encode())
        return self.port.readline().decode()[:-1]
        

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



    def get_data_oneshot(self, sensor, n=1):
        assert(sensor in SENSORS)
        assert(n>=1)
        assert(self.status == "RUNNING") # sensor must be started using .start() in order to get data
        assert(self.mode == "ONESHOT") # sensor must be in oneshot mode using .set_mode("ONESHOT")

        # self.set_mode("ONESHOT")

        output = []

        for i in range(n):
            self.port.write(f"GET {sensor}\n".encode())
            k = 1

            if(sensor == "ALL"): k = 7
            elif(sensor == "ENV"): k = 3
            elif(sensor == "MOTION"): k = 3
            else: k = 1


            for j in range(k):
                raw_values_list = self.port.readline().decode().split()
                values_list = [float(raw_values_list[0]), raw_values_list[1], float(raw_values_list[2])]

                output.append(values_list)

        return output
    

    # def start_record(self):
    #     self.port.write("SET MODE LOG\n".encode())
    #     assert(self.port.readline().decode() == SENSOR_ACK)
    #     self.start()
    
    # def stop_record(self):
    #     self.stop()

    def get_record(self):
        self.port.write("GET LOG\n".encode())
        entete = self.port.readline().decode()
        assert(entete[0:3] == "LOG") # assert sensor is giving back log data

        size = int(entete[4:])
        output = []

        for i in range(size*7):
            raw_values_list = self.port.readline().decode().split()
            values_list = [float(raw_values_list[0]), raw_values_list[1], float(raw_values_list[2])]

            output.append(values_list)

        assert(len(output) == size*7)

        return output