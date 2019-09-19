import django_filters

from .models import SensorReadEvent, ControlBoard, Sensor


class SensorReadEventFilter(django_filters.FilterSet):
    sensor__sensor_id = django_filters.CharFilter(label="ID do sensor", lookup_expr='contains')
    value_read = django_filters.NumberFilter(label="Valor lido")
    timestamp = django_filters.DateTimeFromToRangeFilter(label="Data/hora")
    sensor__control_board__prototype_side = django_filters.ChoiceFilter(choices=ControlBoard.PrototypeSide)

    class Meta:
        model = SensorReadEvent
        fields = ('sensor__sensor_id', 'timestamp', 'value_read', 'sensor__control_board__prototype_side')
