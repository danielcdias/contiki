import logging
import struct

from threading import Thread

import paho.mqtt.client as mqtt

from webapp.models import ControlBoard

MQTT_SERVER_HOST = "danieldias.mooo.com"
MQTT_SERVER_PORT = 1883
MQTT_SERVER_TIMEOUT = 60
CLIENT_ID = "CEIOT-INC14-DCD77"

MQTT_TOPIC_STATUS = "/projetofinal/sta/#"
MQTT_TOPIC_COMMAND = "/projetofinal/cmd/"

LOG_OUTPUT_FORMAT = "%(asctime)s %(levelname)s [%(name)s] %(message)s"
LOG_DATETIME_FORMAT = "%Y/%m/%d %H:%M:%S"

logging.basicConfig(format=LOG_OUTPUT_FORMAT, level=logging.DEBUG,
                    datefmt=LOG_DATETIME_FORMAT)
logger = logging.getLogger("MQTTBridge")


class MQTTBridge:

    def __init__(self):
        self.mqttc_cli = mqtt.Client(client_id=CLIENT_ID)
        self.is_running = False

    def on_connect(self, client, userdata, flags, rc):
        logger.info("Connection started with {}, {}, {}, {}.".format(client, userdata, flags, rc))

    def on_disconnect(self, client, userdata, rc):
        logger.info("Connection finished with {}, {}, {}.".format(client, userdata, rc))

    def on_message(self, client, userdata, message):
        logger.info("Message received from {}, {}, message: {}, {}, {}, {}.".format(client, userdata, message.topic,
                                                                                    message.payload, message.qos,
                                                                                    message.retain))
        mac_end = message.topic[-4:-2] + ":" + message.topic[-2:]
        query = ControlBoard.objects.filter(mac_address__endswith=mac_end)
        if query:
            board = query[0]
            value_str = message.payload.decode()
            board.last_led_level = int(value_str)
            board.save()

    def on_publish(self, client, userdata, mid):
        logger.info("Message published from {}, {}, {}.".format(client, userdata, mid))
        pass

    def on_subscribe(self, client, userdata, mid, granted_qos):
        logger.info("Subscribed {}, {}, {}, {}.".format(client, userdata, mid, granted_qos))
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
            # self.mqttc_cli.subscribe("$SIS/#", 0)
            self.mqttc_cli.subscribe(MQTT_TOPIC_STATUS, 0)
            self.is_running = True
            self.mqttc_cli.loop_start()
        except ConnectionRefusedError as ex:
            self.log_error("Cannot connect to MQTT broker! Exception: {}".format(ex))

    def send_command(self, board: ControlBoard, value: int):
        result = False
        try:
            buf = str(value)
            queue_cmd = MQTT_TOPIC_COMMAND + board.mac_address[12:14] + board.mac_address[15:17]
            logger.info(">>>> sending queue {} - command {}".format(queue_cmd, buf))
            self.mqttc_cli.publish(queue_cmd, buf)
            result = True
        except (ValueError, ConnectionError, ConnectionRefusedError) as ex:
            self.log_error("Cannot publish to command topic! Exception: {}".format(ex))
        return result

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
