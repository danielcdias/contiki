import re

from django.core.exceptions import ValidationError
from django.db import models
from django.utils.translation import gettext_lazy as _
from django.contrib.auth.models import User


def check_mac_address(value):
    if not re.match("[0-9a-f]{2}([-:]?)[0-9a-f]{2}(\\1[0-9a-f]{2}){4}$", value.lower()):
        raise ValidationError(
            _('Invalid MAC address '),
        )


class MQTTConnection(models.Model):
    hostname = models.CharField(max_length=200, unique=True, verbose_name="Hostname")
    port = models.IntegerField(verbose_name="Port")
    connection_timeout = models.IntegerField(default=60, verbose_name="Connection timeout")

    objects = models.Manager()

    def __str__(self):
        return "{}:{}".format(self.hostname, self.port)

    @property
    def last_status(self):
        host = self.connectionstatus_set.all().order_by('-timestamp')[0]
        if not host:
            host = ConnectionStatus.DISCONNECTED
        return host.host_status


class ConnectionStatus(models.Model):
    CONNECTED = 0
    DISCONNECTED = 1

    HostStatus = (
        (CONNECTED, 'connected'),
        (DISCONNECTED, 'disconnected'),
    )

    timestamp = models.DateTimeField(auto_now=True)
    host_status = models.IntegerField(verbose_name='Connection status', choices=HostStatus)
    host_connection = models.ForeignKey(MQTTConnection, on_delete=models.CASCADE, verbose_name="MQTTConnection")

    objects = models.Manager()

    @property
    def host_connection_description(self):
        return "{}".format(self.HostStatus[self.host_status][1])

    def __str__(self):
        return "{} is {}".format(self.host_connection.hostname, self.host_connection_description)


class BoardVendor(models.Model):
    description = models.CharField(max_length=100, unique=True, verbose_name="Description")

    objects = models.Manager()

    def __str__(self):
        return self.description


class BoardModel(models.Model):
    description = models.CharField(max_length=100, unique=True, verbose_name="Description")
    board_vendor = models.ForeignKey(BoardVendor, on_delete=models.CASCADE, verbose_name="Vendor")

    objects = models.Manager()

    def __str__(self):
        return self.description


class ControlBoard(models.Model):
    PROTOTYPE_INTENSIVE_SIDE = 0
    PROTOTYPE_EXTENSIVE_SIDE = 1

    PrototypeSide = (
        (PROTOTYPE_INTENSIVE_SIDE, "Modelo Intensivo"),
        (PROTOTYPE_EXTENSIVE_SIDE, "Modelo Extensivo"),
    )

    nickname = models.CharField(max_length=50, unique=True, verbose_name="Nickname")
    mac_address = models.CharField(max_length=17, verbose_name='MAC address', unique=True,
                                   validators=[check_mac_address])
    prototype_side = models.IntegerField(verbose_name='Prototype side', choices=PrototypeSide)
    board_model = models.ForeignKey(BoardModel, on_delete=models.CASCADE, verbose_name="Model")

    objects = models.Manager()

    @property
    def prototype_side_description(self):
        return "{}".format(self.PrototypeSide[self.prototype_side][1])

    @property
    def short_mac_id(self):
        return self.mac_address[-5:-3] + self.mac_address[-2:]

    def __str__(self):
        return "{} - {}".format(self.nickname, self.prototype_side_description)


class ControlBoardEvent(models.Model):
    timestamp = models.DateTimeField(auto_now=True)
    status_received = models.CharField(max_length=10)
    control_board = models.ForeignKey(ControlBoard, on_delete=models.CASCADE, verbose_name="ControlBoard")

    objects = models.Manager()

    def __str__(self):
        return "ControlBoard {} event received - timestamp: {}, status: {}".format(self.control_board.nickname,
                                                                                   self.timestamp,
                                                                                   self.status_received)


class SensorType(models.Model):
    sensor_type = models.CharField(max_length=50, verbose_name="Type")
    description = models.CharField(max_length=100, verbose_name="Description")
    data_sheet = models.TextField(verbose_name="Data sheet")
    precision = models.IntegerField(default=0, verbose_name="Decimal precision")

    objects = models.Manager()

    def __str__(self):
        return self.sensor_type


class Sensor(models.Model):
    ROLE_RAIN_DETECTION_SURFACE = 0
    ROLE_RAIN_DETECTION_DRAIN = 1
    ROLE_RAIN_ABSORPTION = 2
    ROLE_RAIN_AMOUNT = 3

    SensorRole = (
        (ROLE_RAIN_DETECTION_SURFACE, "Detecção de chuva na supercífie"),
        (ROLE_RAIN_DETECTION_DRAIN, "Detecção de chuva no ralo"),
        (ROLE_RAIN_ABSORPTION, "Absorção de chuva"),
        (ROLE_RAIN_AMOUNT, "Quantidade de chuva"),
    )

    sensor_id = models.CharField(max_length=10, verbose_name="ID")
    description = models.CharField(max_length=100, verbose_name="Description")
    sensor_type = models.ForeignKey(SensorType, on_delete=models.CASCADE, verbose_name="Type")
    sensor_role = models.IntegerField(verbose_name='Sensor role', choices=SensorRole)
    sensor_reading_conversion = models.FloatField(verbose_name="Sensor reading conversion", default=0.0)
    control_board = models.ForeignKey(ControlBoard, on_delete=models.CASCADE, verbose_name="Control board")

    objects = models.Manager()

    @property
    def sensor_role_description(self):
        return "{}".format(self.SensorRole[self.sensor_role][1])

    def __str__(self):
        return "{} - {} - {}".format(self.control_board.nickname, self.sensor_id, self.sensor_role_description)


class SensorReadEvent(models.Model):
    timestamp = models.DateTimeField(auto_now=True)
    value_read = models.FloatField()
    sensor = models.ForeignKey(Sensor, on_delete=models.CASCADE, verbose_name="Sensor")

    objects = models.Manager()

    def __str__(self):
        return "Sensor {} read - timestamp: {}, value read: {}".format(self.sensor.sensor_id, self.timestamp,
                                                                       self.value_read)


class NotificationUser(models.Model):
    user = models.OneToOneField(User, on_delete=models.CASCADE)
    notify_errors = models.BooleanField(default=False, verbose_name="Send errors notifications")

    objects = models.Manager()
