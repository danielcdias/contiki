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


class BoardVendor(models.Model):
    description = models.CharField(max_length=100, unique=True, verbose_name="Description")

    def __str__(self):
        return self.description


class BoardModel(models.Model):
    description = models.CharField(max_length=100, unique=True, verbose_name="Description")
    board_vendor = models.ForeignKey(BoardVendor, on_delete=models.CASCADE, verbose_name="Vendor")

    def __str__(self):
        return self.description


class ControlBoard(models.Model):
    nickname = models.CharField(max_length=50, unique=True, verbose_name="Nickname")
    mac_address = models.CharField(max_length=17, verbose_name='MAC address', unique=True,
                                   validators=[check_mac_address])
    board_model = models.ForeignKey(BoardModel, on_delete=models.CASCADE, verbose_name="Model")

    def __str__(self):
        return self.nickname


class SensorType(models.Model):
    sensor_type = models.CharField(max_length=50, verbose_name="Type")
    description = models.CharField(max_length=100, verbose_name="Description")
    data_sheet = models.TextField(verbose_name="Data sheet")
    precision = models.IntegerField(default=0, verbose_name="Decimal precision")

    def __str__(self):
        return self.sensor_type


class Sensor(models.Model):
    sensor_id = models.CharField(max_length=10, verbose_name="ID")
    description = models.CharField(max_length=100, verbose_name="Description")
    sensor_type = models.ForeignKey(SensorType, on_delete=models.CASCADE, verbose_name="Type")
    control_board = models.ForeignKey(ControlBoard, on_delete=models.CASCADE, verbose_name="Control board")

    def __str__(self):
        return self.sensor_id


class SensorReadEvent(models.Model):
    timestamp = models.DateTimeField(auto_now=True)
    value_read = models.FloatField()
    sensor = models.ForeignKey(Sensor, on_delete=models.CASCADE, verbose_name="Sensor")

    def __str__(self):
        return "Sensor {} read - timestamp: {}, value read: {}".format(self.sensor.sensor_id, self.timestamp,
                                                                       self.value_read)


class NotificationUser(models.Model):
    user = models.OneToOneField(User, on_delete=models.CASCADE)
    notify_errors = models.BooleanField(default=False, verbose_name="Send errors notifications")


class ErrorReport(models.Model):
    ERROR_MQTT_CONNECTION = 0
    ERROR_DATABASE_CONNECTION = 1

    ErrorTypes = (
        (ERROR_MQTT_CONNECTION, 'Error in MQTT broker connection'),
        (ERROR_DATABASE_CONNECTION, 'Error in database connection'),
    )

    timestamp = models.DateTimeField(auto_now=True)
    error_type = models.IntegerField(verbose_name='Error type', choices=ErrorTypes)
    details = models.TextField(verbose_name="Details")

    def __str__(self):
        return "Error! Type: {}, Timestamp: {}, Details: {}".format(self.ErrorTypes[self.error_type][1], self.timestamp,
                                                                    self.details)
