import re

from django.core.exceptions import ValidationError
from django.db import models
from django.db.models import Count, Avg
from django.utils.translation import gettext_lazy as _


def check_mac_address(value):
    if not re.match("[0-9a-f]{2}([-:]?)[0-9a-f]{2}(\\1[0-9a-f]{2}){4}$", value.lower()):
        raise ValidationError(
            _('Invalid MAC address '),
        )


class Manufacturer(models.Model):
    name = models.CharField(max_length=250)
    web_site = models.CharField(max_length=250)

    def __str__(self):
        return self.name


class BoardModel(models.Model):
    name = models.CharField(max_length=250)
    manufacturer = models.ForeignKey(Manufacturer, on_delete=models.CASCADE)

    def __str__(self):
        return self.name


class ControlBoard(models.Model):
    nickname = models.CharField(max_length=25)
    mac_address = models.CharField(max_length=17, verbose_name='MAC address', unique=True,
                                   validators=[check_mac_address])
    ipv6_address = models.GenericIPAddressField(protocol='IPv6', verbose_name='IPv6 address', editable=False, null=True)
    port_number = models.IntegerField(verbose_name='port number', default=8802)
    board_model = models.ForeignKey(BoardModel, verbose_name='board model', on_delete=models.CASCADE)

    def __str__(self):
        return "%s (%s)" % (self.nickname, self.mac_address)


class DeviceModel(models.Model):
    name = models.CharField(max_length=250)
    min_value = models.FloatField(verbose_name='minimum value', default=0.0)
    max_value = models.FloatField(verbose_name='maximum value', default=0.0)
    precision = models.IntegerField(default=0)
    manufacturer = models.ForeignKey(Manufacturer, on_delete=models.CASCADE)

    def __str__(self):
        return self.name


class Device(models.Model):
    DT_SENSOR = 0
    DT_ACTUATOR = 1
    DT_SENSOR_ACTUATOR = 2

    DeviceType = (
        (DT_SENSOR, 'Sensor'),
        (DT_ACTUATOR, 'Atuador'),
        (DT_SENSOR_ACTUATOR, 'Sensor/Atuador'),
    )

    device_id = models.CharField(max_length=250)
    device_model = models.ForeignKey(DeviceModel, verbose_name='device model', on_delete=models.CASCADE)
    device_type = models.IntegerField(verbose_name='device type', choices=DeviceType, default=DT_SENSOR)
    control_board = models.ForeignKey(ControlBoard, verbose_name='control board', on_delete=models.CASCADE)

    def __str__(self):
        return "%s - %s in %s board" % (self.device_id, self.get_device_type_str(), self.control_board.nickname)

    def get_device_type_str(self):
        return self.DeviceType[int(str(self.device_type)) if self.device_type else 0][1]

    def get_last_status(self):
        last_read = None
        if self.devicestatus_set.count() != 0:
            last_read = self.devicestatus_set.all().order_by('-timestamp')[0]
        return last_read

    def list_device_status(self):
        return self.devicestatus_set.all().order_by('-timestamp')

    def list_device_command(self):
        return self.devicecommand_set.all().order_by('-timestamp')

    def get_form_number_step(self):
        result = '1'
        if self.device_model.precision == 1:
            result = '0.1'
        elif self.device_model.precision > 1:
            result = '0.' + result.rjust(self.device_model.precision, '0')
        return result

    def get_health_count(self):
        q = self.devicestatus_set.all().values('health_state').annotate(c=Count('health_state')).order_by('-c')
        return q

    def get_value_avg(self):
        return self.devicestatus_set.all().aggregate(Avg('value'))

    def is_sensor(self):
        dev_type = int(str(self.device_type)) if self.device_type else 0
        return dev_type == Device.DT_SENSOR or dev_type == Device.DT_SENSOR_ACTUATOR

    def is_actuator(self):
        dev_type = int(str(self.device_type)) if self.device_type else 0
        return dev_type == Device.DT_ACTUATOR or dev_type == Device.DT_SENSOR_ACTUATOR

    def get_exec_code_count(self):
        q = self.devicecommand_set.all().values('exec_code').annotate(c=Count('exec_code')).order_by('-c')
        return q


class DeviceStatus(models.Model):
    HEALTH_UNKNOWN = 0
    HEALTH_HEALTHY = 1
    HEALTH_ERROR = 2
    HEALTH_DEVICE_NOT_FOUND = 3

    HealthState = (
        (HEALTH_UNKNOWN, "Desconhecido"),
        (HEALTH_HEALTHY, "Saudável"),
        (HEALTH_ERROR, "Erro"),
        (HEALTH_DEVICE_NOT_FOUND, "Não encontrado"),
    )

    value = models.FloatField(default=0.0)
    timestamp = models.DateTimeField(auto_now=True)
    health_state = models.IntegerField(verbose_name='healthy state', choices=HealthState, default=HEALTH_UNKNOWN)
    device = models.ForeignKey(Device, on_delete=models.CASCADE)

    def get_health_state_str(self):
        return self.HealthState[int(str(self.health_state)) if self.health_state else 0][1]


class DeviceCommand(models.Model):
    EXEC_NOT_STARTED = 0
    EXEC_NOT_SUPPORTED = 1
    EXEC_SUCCESSFUL = 2
    EXEC_ERROR = 3
    EXEC_COMM_ERROR = 4
    EXEC_TIMEOUT = 5

    ExecutionCode = (
        (EXEC_NOT_STARTED, "Não iniciado"),
        (EXEC_NOT_SUPPORTED, "Não suportado"),
        (EXEC_SUCCESSFUL, "Sucesso"),
        (EXEC_ERROR, "Erro"),
        (EXEC_COMM_ERROR, "Erro comunicação"),
        (EXEC_TIMEOUT, "Timeout"),
    )

    exec_code = models.IntegerField(verbose_name='execution code', choices=ExecutionCode, default=EXEC_NOT_STARTED)
    value = models.FloatField(default=0.0)
    timestamp = models.DateTimeField(auto_now=True)
    device = models.ForeignKey(Device, on_delete=models.CASCADE)

    def get_exec_code_str(self):
        return self.ExecutionCode[int(str(self.exec_code)) if self.exec_code else 0][1]
