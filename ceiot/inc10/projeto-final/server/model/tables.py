import django_tables2 as tables
from .models import SensorReadEvent


class SensorsReadingTable(tables.Table):
    class Meta:
        model = SensorReadEvent
        template_name = "django_tables2/bootstrap.html"
        fields = ("sensor.sensor_id", "sensor.description", "timestamp", "value_read",
                  "sensor.control_board.prototype_side",)
