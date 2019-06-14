import csv

from django.utils import timezone
from django.http import StreamingHttpResponse
from django.views import generic

from .models import MQTTConnection, SensorReadEvent


class IndexView(generic.ListView):
    template_name = 'model/index.html'
    context_object_name = 'hosts_list'
    queryset = MQTTConnection.objects.all()


class Echo:
    """An object that implements just the write method of the file-like
    interface.
    """

    def write(self, value):
        """Write the value by returning it, instead of storing in a buffer."""
        return value


def get_sensors_read_event_in_csv(request):
    sensors_read_events = SensorReadEvent.objects.all().order_by('timestamp')
    rows = [['Placa Controladora', 'ID do Sensor', 'Timestamp', 'Timestamp', 'Valor Lido']]
    for sensor_read in sensors_read_events:
        dt = timezone.localtime(sensor_read.timestamp).strftime('%d/%m/%Y %H:%M:%S')
        rows.append(
            [sensor_read.sensor.control_board.nickname, sensor_read.sensor.sensor_id, dt, sensor_read.value_read])

    pseudo_buffer = Echo()
    writer = csv.writer(pseudo_buffer, delimiter=',', quotechar='"', quoting=csv.QUOTE_ALL)
    response = StreamingHttpResponse((writer.writerow(row) for row in rows),
                                     content_type="text/csv")
    response['Content-Disposition'] = 'attachment; filename="senso_read_events.csv"'
    return __generate_csv('senso_read_events.csv', rows, request)


def __generate_csv(csv_filname: str, rows, request):
    sep = ';' if request.GET.get('sep') == 'sc' else ','
    pseudo_buffer = Echo()
    writer = csv.writer(pseudo_buffer, delimiter=sep, quotechar='"', quoting=csv.QUOTE_ALL)
    response = StreamingHttpResponse((writer.writerow(row) for row in rows),
                                     content_type="text/csv")
    response['Content-Disposition'] = 'attachment; filename="' + csv_filname + '"'
    return response
