from django import template
from django.template.defaultfilters import stringfilter

from dbmodel.models import DeviceStatus, DeviceCommand

register = template.Library()


@register.filter
@stringfilter
def health_state_filter(value):
    result = ''
    int_value = int(value) if value != "None" else 0
    if int_value == DeviceStatus.HEALTH_UNKNOWN:
        result = DeviceStatus.HealthState[DeviceStatus.HEALTH_UNKNOWN][1]
    elif int_value == DeviceStatus.HEALTH_HEALTHY:
        result = DeviceStatus.HealthState[DeviceStatus.HEALTH_HEALTHY][1]
    elif int_value == DeviceStatus.HEALTH_ERROR:
        result = DeviceStatus.HealthState[DeviceStatus.HEALTH_ERROR][1]
    elif int_value == DeviceStatus.HEALTH_DEVICE_NOT_FOUND:
        result = DeviceStatus.HealthState[DeviceStatus.HEALTH_DEVICE_NOT_FOUND][1]
    return result


@register.filter
@stringfilter
def exec_code_filter(value):
    result = ''
    int_value = int(value) if value != "None" else 0
    if int_value == DeviceCommand.EXEC_NOT_SUPPORTED:
        result = DeviceCommand.ExecutionCode[DeviceCommand.EXEC_NOT_SUPPORTED][1]
    elif int_value == DeviceCommand.EXEC_SUCCESSFUL:
        result = DeviceCommand.ExecutionCode[DeviceCommand.EXEC_SUCCESSFUL][1]
    elif int_value == DeviceCommand.EXEC_ERROR:
        result = DeviceCommand.ExecutionCode[DeviceCommand.EXEC_ERROR][1]
    elif int_value == DeviceCommand.EXEC_COMM_ERROR:
        result = DeviceCommand.ExecutionCode[DeviceCommand.EXEC_COMM_ERROR][1]
    elif int_value == DeviceCommand.EXEC_TIMEOUT:
        result = DeviceCommand.ExecutionCode[DeviceCommand.EXEC_TIMEOUT][1]
    return result


@register.filter
@stringfilter
def on_off_filter(value):
    return "ON" if str(value) != "None" else "OFF"
