import logging
import time

from django.core.mail import EmailMessage
from threading import Thread, Event
from datetime import datetime
from django.utils import timezone
from config import settings
from pytz import timezone as tz

import paho.mqtt.client as mqtt

from model.models import ControlBoard, MQTTConnection, ConnectionStatus

MQTT_DISCONNECTION_EMAIL_NOTIFY_TIMEOUT = 120
CLIENT_ID = "TV-CWB-EstudoModelo"

MQTT_TOPIC_STATUS = "/tvcwb1299/mmm/sta/#"
MQTT_TOPIC_COMMAND = "/tvcwb1299/mmm/cmd/"

MQTT_CMD_SERVER_TIMESTAMP = "T"

MQTT_BOARD_STARTUP_STATUS = "STT"
MQTT_BOARD_TIMESTAMP_UPDATE_REQUEST = "TUR"

LOG_OUTPUT_FORMAT = "%(asctime)s %(levelname)s [%(name)s] %(message)s"
LOG_DATETIME_FORMAT = "%Y/%m/%d %H:%M:%S"

EMAIL_SUBJECT = '[Django Server] *** ERRO *** Terraço Verde IoT'

logger = logging.getLogger("tvcwb.mqtt_client")

stop_timer_flag = Event()  # Flag to stop timer if reconnection occurs before MQTT_DISCONNECTION_EMAIL_NOTIFY_TIMEOUT


class DisconnectionTimer(Thread):
    def __init__(self, event):
        Thread.__init__(self)
        self.stopped = event
        self.sent = False

    def run(self):
        while (not self.stopped.wait(MQTT_DISCONNECTION_EMAIL_NOTIFY_TIMEOUT)) and (not self.sent):
            email = EmailMessage(EMAIL_SUBJECT, 'Desconexão ocorrida com o broker MQTT.',
                                 to=['daniel.dias@gmail.com'])
            email.send()
            self.sent = True


class MQTTBridge:

    def __init__(self):
        self.mqttc_cli = mqtt.Client(client_id=CLIENT_ID)
        self.is_running = False

    def on_connect(self, client, userdata, flags, rc):
        stop_timer_flag.set()
        logger.info("Connection started with {}, {}, {}, {}.".format(client, userdata, flags, rc))
        MQTTBridge.__update_connection_status(ConnectionStatus.CONNECTED)

    def on_disconnect(self, client, userdata, rc):
        logger.warning("Connection finished with {}, {}, {}.".format(client, userdata, rc))
        MQTTBridge.__update_connection_status(ConnectionStatus.DISCONNECTED)
        stop_timer_flag.clear()
        thread = DisconnectionTimer(stop_timer_flag)
        thread.start()

    def on_message(self, client, userdata, message):
        logger.debug("Message received from {}, {}, message: {}, {}, {}, {}.".format(client, userdata, message.topic,
                                                                                     message.payload, message.qos,
                                                                                     message.retain))
        mac_end = (message.topic[-8:-6] + ":" + message.topic[-6:-4]
                   if len(message.topic) > 24 else message.topic[-4:-2] + ":" + message.topic[-2:])
        sensor_id = (message.topic[-3:] if len(message.topic) > 24 else None)
        query = ControlBoard.objects.filter(mac_address__endswith=mac_end)
        if query:
            board = query[0]
            payload = message.payload.decode()
            if MQTTBridge.is_valid_payload(payload):
                value_str = payload
                timestamp = timezone.now()
                if (len(payload) >= 12) and (payload.find(MQTT_CMD_SERVER_TIMESTAMP) > -1):
                    value_str = payload[0:payload.find(MQTT_CMD_SERVER_TIMESTAMP)]
                    timestamp = timezone.localtime(
                        datetime.fromtimestamp(int(payload[payload.find(MQTT_CMD_SERVER_TIMESTAMP) + 1:]),
                                               tz=tz(settings.TIME_ZONE)))
                if sensor_id:
                    subquery = board.sensor_set.all().filter(sensor_id=sensor_id)
                    if subquery:
                        sensor = subquery[0]
                        try:
                            value = float(value_str)
                            if sensor.sensor_type.precision > 0:
                                value = value / (10 ** sensor.sensor_type.precision)
                            sensor_read_event = sensor.sensorreadevent_set.create(timestamp=timestamp, value_read=value)
                            sensor_read_event.save()
                        except ValueError:
                            logger.warning("Value received is not a valid float: {}".format(value_str))
                    else:
                        logger.warning(
                            "No sensor was found with ID {} for the control board {}.".format(sensor_id,
                                                                                              board.nickname))
                else:
                    board_event_received = board.controlboardevent_set.create(timestamp=timestamp,
                                                                              status_received=value_str[:10])
                    board_event_received.save()
                    if value_str[0:3] in (MQTT_BOARD_STARTUP_STATUS, MQTT_BOARD_TIMESTAMP_UPDATE_REQUEST):
                        self.send_command(board, MQTT_CMD_SERVER_TIMESTAMP, MQTTBridge.get_time_in_seconds())
            else:
                logger.warning("Payload received is not well formatted: {}.".format(payload))
        else:
            logger.warning("No control board was found with mac address ending with {}.".format(mac_end))

    def on_publish(self, client, userdata, mid):
        logger.debug("Message published to {}, {}, {}.".format(client, userdata, mid))

    def on_subscribe(self, client, userdata, mid, granted_qos):
        logger.debug("Subscribed {}, {}, {}, {}.".format(client, userdata, mid, granted_qos))

    def on_log(self, mqttc, obj, level, string):
        pass

    def is_running(self):
        return self.is_running

    def start_bridge(self):
        connected = False
        email_sent = False
        while True:
            if not connected:
                MQTTBridge.__update_connection_status(ConnectionStatus.DISCONNECTED)
                connection_info = MQTTConnection.objects.get(id=1)
                if connection_info:
                    try:
                        self.mqttc_cli.on_connect = self.on_connect
                        self.mqttc_cli.on_subscribe = self.on_subscribe
                        self.mqttc_cli.on_publish = self.on_publish
                        self.mqttc_cli.on_message = self.on_message
                        self.mqttc_cli.on_disconnect = self.on_disconnect
                        self.mqttc_cli.on_log = self.on_log
                        self.mqttc_cli.reconnect_delay_set(min_delay=1, max_delay=30)
                        self.mqttc_cli.connect(connection_info.hostname, connection_info.port,
                                               connection_info.connection_timeout)
                        self.mqttc_cli.subscribe(MQTT_TOPIC_STATUS, 0)
                        self.is_running = True
                        connected = True
                        email_sent = False
                        self.mqttc_cli.loop_start()
                    except (ConnectionRefusedError, TimeoutError) as ex:
                        if not email_sent:
                            email = EmailMessage(EMAIL_SUBJECT,
                                                 'Não foi possível conectar servidor ao broker MQTT.',
                                                 to=['daniel.dias@gmail.com'])
                            email.send()
                            email_sent = True
                        logger.error("Cannot connect to MQTT broker! Exception: {}".format(ex))
                else:
                    connected = False
                    logger.error("No connection info found!")
            time.sleep(5)

    def send_command(self, board: ControlBoard, cmd: str, value: int) -> bool:
        result = False
        try:
            buf = cmd + str(value)
            queue_cmd = MQTT_TOPIC_COMMAND + board.mac_address[12:14] + board.mac_address[15:17]
            logger.debug("Sending to queue {} - command {}".format(queue_cmd, buf))
            self.mqttc_cli.publish(queue_cmd, buf)
            result = True
        except (ValueError, ConnectionError, ConnectionRefusedError) as ex:
            logger.exception("Cannot publish to command topic! Exception: {}".format(ex))
        return result

    @staticmethod
    def __update_connection_status(status: int):
        if (status == ConnectionStatus.CONNECTED) or (status == ConnectionStatus.DISCONNECTED):
            connection_info = MQTTConnection.objects.get(id=1)
            if connection_info:
                connection_status = connection_info.connectionstatus_set.create(host_status=status)
                connection_status.save()
            else:
                logger.error("No connection info found!")
        else:
            logger.error("Invalid value for connection status.")
            raise ValueError("Invalid value for connection status.")

    @staticmethod
    def get_time_in_seconds() -> int:
        return int(round(time.time()))

    @staticmethod
    def is_valid_payload(payload: str) -> bool:
        result = (payload in (MQTT_BOARD_STARTUP_STATUS, MQTT_BOARD_TIMESTAMP_UPDATE_REQUEST)) or \
                 ((len(payload) >= 12) and (payload.find(MQTT_CMD_SERVER_TIMESTAMP) > -1))
        return result


bridge = MQTTBridge()


def run():
    if not bridge.is_running:
        thread = Thread(target=bridge.start_bridge)
        thread.start()