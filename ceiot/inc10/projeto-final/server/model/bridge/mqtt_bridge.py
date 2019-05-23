import logging
import struct

from threading import Thread

import paho.mqtt.client as mqtt

from model.models import ControlBoard, SensorReadEvent

MQTT_SERVER_HOST = "2804:7f4:3b80:d4af:ccdc:fc4f:a892:6ee"
MQTT_SERVER_PORT = 1883
MQTT_SERVER_TIMEOUT = 60
CLIENT_ID = "TV-CWB-EstudoModelo"

MQTT_TOPIC_STATUS = "/tvcwb/modelo/sta/#"
MQTT_TOPIC_COMMAND = "/tvcwb/modelo/cmd/"

LOG_OUTPUT_FORMAT = "%(asctime)s %(levelname)s [%(name)s] %(message)s"
LOG_DATETIME_FORMAT = "%Y/%m/%d %H:%M:%S"

logger = logging.getLogger("tvcwb.mqtt_client")


class MQTTBridge:

    def __init__(self):
        self.mqttc_cli = mqtt.Client(client_id=CLIENT_ID)
        self.is_running = False

    @staticmethod
    def on_connect(client, userdata, flags, rc):
        logger.info("Connection started with {}, {}, {}, {}.".format(client, userdata, flags, rc))

    @staticmethod
    def on_disconnect(client, userdata, rc):
        logger.warning("Connection finished with {}, {}, {}.".format(client, userdata, rc))

    @staticmethod
    def on_message(client, userdata, message):
        logger.debug("Message received from {}, {}, message: {}, {}, {}, {}.".format(client, userdata, message.topic,
                                                                                     message.payload, message.qos,
                                                                                     message.retain))
        mac_end = message.topic[-8:-6] + ":" + message.topic[-6:-4]
        sensor_id = message.topic[-3:]
        query = ControlBoard.objects.filter(mac_address__endswith=mac_end)
        if query:
            board = query[0]
            subquery = board.sensor_set.all().filter(sensor_id=sensor_id)
            if subquery:
                sensor = subquery[0]
                value_str = message.payload.decode()
                try:
                    value = float(value_str)
                    sensor_read_event = sensor.sensorreadevent_set.create(value_read=value)
                    sensor_read_event.save()
                except ValueError:
                    logger.warning("Value received is not a valid float: {}".format(value_str))
            else:
                logger.warning(
                    "No sensor was found with ID {} for the control board{}.".format(sensor_id, board.nickname))
        else:
            logger.warning("No control board was found with mac address ending with {}.".format(mac_end))

    @staticmethod
    def on_publish(client, userdata, mid):
        logger.debug("Message published from {}, {}, {}.".format(client, userdata, mid))

    @staticmethod
    def on_subscribe(client, userdata, mid, granted_qos):
        logger.debug("Subscribed {}, {}, {}, {}.".format(client, userdata, mid, granted_qos))

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
            self.mqttc_cli.subscribe(MQTT_TOPIC_STATUS, 0)
            self.is_running = True
            self.mqttc_cli.loop_start()
        except (ConnectionRefusedError, TimeoutError) as ex:
            logger.error("Cannot connect to MQTT broker! Exception: {}".format(ex))

    def send_command(self, board: ControlBoard, value: int):
        result = False
        try:
            buf = str(value)
            queue_cmd = MQTT_TOPIC_COMMAND + board.mac_address[12:14] + board.mac_address[15:17]
            logger.debug("Sending to queue {} - command {}".format(queue_cmd, buf))
            self.mqttc_cli.publish(queue_cmd, buf)
            result = True
        except (ValueError, ConnectionError, ConnectionRefusedError) as ex:
            logger.exception("Cannot publish to command topic! Exception: {}".format(ex))
        return result


bridge = MQTTBridge()


def run():
    thread = Thread(target=bridge.start_bridge)
    thread.start()
