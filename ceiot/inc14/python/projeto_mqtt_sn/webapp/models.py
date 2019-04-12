import re

from django.core.exceptions import ValidationError
from django.db import models
from django.utils.translation import gettext_lazy as _


def check_mac_address(value):
    if not re.match("[0-9a-f]{2}([-:]?)[0-9a-f]{2}(\\1[0-9a-f]{2}){4}$", value.lower()):
        raise ValidationError(
            _('Invalid MAC address '),
        )


class ControlBoard(models.Model):
    nickname = models.CharField(max_length=100, unique=True)
    board_model = models.CharField(max_length=100, verbose_name='board model')
    mac_address = models.CharField(max_length=17, verbose_name='MAC address', unique=True,
                                   validators=[check_mac_address])
    last_led_level = models.IntegerField(default=0, editable=False)

    def __str__(self):
        return "%s - %s" % (self.nickname, self.mac_address)
