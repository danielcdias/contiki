import logging
import time

from django.core.mail import EmailMessage
from threading import Thread, Event

import paho.mqtt.client as mqtt

from model.models import ControlBoard, MQTTConnection, ConnectionStatus

# MQTT_SERVER_HOST = "danieldias.mooo.com"
# MQTT_SERVER_PORT = 1883
# MQTT_SERVER_TIMEOUT = 60
MQTT_DISCONNECTION_EMAIL_NOTIFY_TIMEOUT = 120
CLIENT_ID = "TV-CWB-EstudoModelo"

MQTT_TOPIC_STATUS = "/tvcwb1299/mmm/sta/#"
MQTT_TOPIC_COMMAND = "/tvcwb1299/mmm/cmd/"

LOG_OUTPUT_FORMAT = "%(asctime)s %(levelname)s [%(name)s] %(message)s"
LOG_DATETIME_FORMAT = "%Y/%m/%d %H:%M:%S"

logger = logging.getLogger("tvcwb.mqtt_client")

stop_timer_flag = Event()  # Flag to stop timer if reconnection occurs before MQTT_DISCONNECTION_EMAIL_NOTIFY_TIMEOUT


class DisconnectionTimer(Thread):
    def __init__(self, event):
        Thread.__init__(self)
        self.stopped = event
        self.sent = False

    def run(self):
        while (not self.stopped.wait(MQTT_DISCONNECTION_EMAIL_NOTIFY_TIMEOUT)) and (not self.sent):
            email = EmailMessage('*** ERRO *** Terraço Verde IoT', 'Desconexão ocorrida com o broker MQTT.',
                                 to=['daniel.dias@gmail.com'])
            email.send()
            self.sent = True


class MQTTBridge:

    def __init__(self):
        self.mqttc_cli = mqtt.Client(client_id=CLIENT_ID)
        self.is_running = False

    @staticmethod
    def on_connect(client, userdata, flags, rc):
        stop_timer_flag.set()
        logger.info("Connection started with {}, {}, {}, {}.".format(client, userdata, flags, rc))
        MQTTBridge.__update_connection_status(ConnectionStatus.CONNECTED)

    @staticmethod
    def on_disconnect(client, userdata, rc):
        logger.warning("Connection finished with {}, {}, {}.".format(client, userdata, rc))
        MQTTBridge.__update_connection_status(ConnectionStatus.DISCONNECTED)
        stop_timer_flag.clear()
        thread = DisconnectionTimer(stop_timer_flag)
        thread.start()

    @staticmethod
    def on_message(client, userdata, message):
        logger.debug("Message received from {}, {}, message: {}, {}, {}, {}.".format(client, userdata, message.topic,
                                                                                     message.payload, message.qos,
                                                                                     message.retain))
        mac_end = (message.topic[-8:-6] + ":" + message.topic[-6:-4]
                   if len(message.topic) > 24 else message.topic[-4:-2] + ":" + message.topic[-2:])
        sensor_id = (message.topic[-3:] if len(message.topic) > 24 else None)
        query = ControlBoard.objects.filter(mac_address__endswith=mac_end)
        if query:
            board = query[0]
            value_str = message.payload.decode()
            if sensor_id:
                subquery = board.sensor_set.all().filter(sensor_id=sensor_id)
                if subquery:
                    sensor = subquery[0]
                    try:
                        value = float(value_str)
                        sensor_read_event = sensor.sensorreadevent_set.create(value_read=value)
                        sensor_read_event.save()
                    except ValueError:
                        logger.warning("Value received is not a valid float: {}".format(value_str))
                else:
                    logger.warning(
                        "No sensor was found with ID {} for the control board {}.".format(sensor_id, board.nickname))
            else:
                board_event_received = board.controlboardevent_set.create(status_received=value_str[:10])
                board_event_received.save()
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
                            email = EmailMessage('*** ERRO *** Terraço Verde IoT',
                                                 'Não foi possível conectar servidor ao broker MQTT.',
                                                 to=['daniel.dias@gmail.com'])
                            email.send()
                            email_sent = True
                        logger.error("Cannot connect to MQTT broker! Exception: {}".format(ex))
                else:
                    # TODO Criar processo de reconexão
                    connected = False
                    logger.error("No connection info found!")
            time.sleep(5)

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


bridge = MQTTBridge()


def run():
    if not bridge.is_running:
        thread = Thread(target=bridge.start_bridge)
        thread.start()
