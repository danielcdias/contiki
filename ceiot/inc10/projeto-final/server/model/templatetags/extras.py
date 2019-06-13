from django import template
from django.template.defaultfilters import stringfilter

from model.models import ConnectionStatus

register = template.Library()

ONLINE_ICON_FILE = "online_24.png"
OFFLINE_ICON_FILE = "offline_24.png"


@register.filter
@stringfilter
def host_status_icon(value):
    int_value = int(value) if (not value) or (value != "None") else ConnectionStatus.DISCONNECTED
    return ONLINE_ICON_FILE if int_value == ConnectionStatus.CONNECTED else OFFLINE_ICON_FILE


@register.filter
@stringfilter
def host_status_str(value):
    int_value = int(value) if (not value) or (value != "None") else ConnectionStatus.DISCONNECTED
    return "conectado" if int_value == ConnectionStatus.CONNECTED else "desconectado"
