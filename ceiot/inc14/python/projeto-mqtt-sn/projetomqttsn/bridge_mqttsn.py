import logging
from threading import Thread

import paho.mqtt.client as mqtt

MQTT_SERVER_HOST = "10.3.2.41"
MQTT_SERVER_PORT = 1883
MQTT_SERVER_TIMEOUT = 60
CLIENT_ID = "CEIOT-INC14-DCD"

LOG_OUTPUT_FORMAT = "%(asctime)s %(levelname)s [%(name)s] %(message)s"
LOG_DATETIME_FORMAT = "%Y/%m/%d %H:%M:%S"

logging.basicConfig(format=LOG_OUTPUT_FORMAT, level=logging.DEBUG,
                    datefmt=LOG_DATETIME_FORMAT)
logger = logging.getLogger("MQTTBridge")


class MQTTBridge:

    def __init__(self):
        self.mqttc_cli = mqtt.Client(client_id=CLIENT_ID)
        self.is_running = False

    def on_connect(self, mqttc, obj, flags, rc):
        pass

    def on_message(mqttc, obj, msg):
        pass

    def on_publish(self, mqttc, obj, mid):
        pass

    def on_subscribe(self, mqttc, obj, mid, granted_qos):
        pass

    def on_log(self, mqttc, obj, level, string):
        pass

    def is_running(self):
        return self.is_running

    def start_bridge(self):
        try:
            self.mqttc_cli.on_connect = self.on_connect
            self.mqttc_cli.on_subscribe = self.on_subscribe
            self.mqttc_cli.on_publish = self.on_publish
            self.mqttc_cli.on_message = self.on_message
            self.mqttc_cli.on_log = self.on_log
            self.mqttc_cli.connect(MQTT_SERVER_HOST, MQTT_SERVER_PORT, MQTT_SERVER_TIMEOUT)
            self.mqttc_cli.subscribe("$SIS/#", 0)
            self.is_running = True
            self.mqttc_cli.loop_forever()
        except ConnectionRefusedError as ex:
            self.log_error("Cannot connect to MQTT broker! Exception: {}".format(ex))

    @staticmethod
    def log_error(message: str, exception: Exception = None):
        import inspect
        # Get the previous frame in the stack, otherwise it would
        # be this function!!!
        func = inspect.currentframe().f_back.f_code
        # Dump the message + the name of this function to the log.
        if exception:
            logger.error("{} - {}".format(func.co_name, message))
        else:
            logger.error("{} - {} Exception: {}".format(func.co_name, message, exception))


bridge = MQTTBridge()


def run():
    thread = Thread(target=bridge.start_bridge)
    thread.start()
