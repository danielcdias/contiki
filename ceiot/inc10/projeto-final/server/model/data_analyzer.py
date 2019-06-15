from datetime import datetime

from django.utils import timezone
from django.db.models import Q

from model.models import SensorReadEvent, ControlBoard, Sensor


def get_diff_datetime(start_date, end_date):
    dt_start = timezone.localtime(start_date)
    dt_end = timezone.localtime(end_date)
    diff = dt_end - dt_start
    hours = ("0" + str((diff.days * 24) + ((diff.seconds // 60) // 60)))[-2:]
    minutes = ("0" + str((diff.seconds // 60) % 60))[-2:]
    seconds = ("0" + str(diff.seconds % 60))[-2:]
    result = hours + ":" + minutes + ":" + seconds
    return result


def get_peak_delay(start_date: str = None, end_date: str = None):
    query_res = SensorReadEvent.objects.all().order_by('timestamp')
    if start_date:
        init_dt = datetime.strptime(start_date, '%Y-%m-%dT%H-%M-%S')
        query_res = query_res.filter(timestamp_gte=init_dt)
    if end_date:
        end_dt = datetime.strptime(end_date, '%Y-%m-%dT%H-%M-%S')
        query_res = query_res.filter(timestamp_lte=end_dt)
    boards = ControlBoard.objects.all()
    result = []
    for board in boards:
        query_events = query_res.filter(Q(sensor__control_board_id=board.id) & (
                Q(sensor__sensor_role=Sensor.ROLE_RAIN_DETECTION_SURFACE) |
                Q(sensor__sensor_role=Sensor.ROLE_RAIN_DETECTION_DRAIN)))
        start_time = None
        for event in query_events:
            if not start_time and event.sensor.sensor_role == Sensor.ROLE_RAIN_DETECTION_SURFACE:
                start_time = event.timestamp
            if start_time and event.sensor.sensor_role == Sensor.ROLE_RAIN_DETECTION_DRAIN:
                diff = get_diff_datetime(start_time, event.timestamp)
                dt_start = timezone.localtime(start_time).strftime('%d/%m/%Y %H:%M:%S')
                dt_end = timezone.localtime(event.timestamp).strftime('%d/%m/%Y %H:%M:%S')
                result.append(
                    {"board": board.nickname, "start_datetime": dt_start, "end_datetime": dt_end, "diff": diff})
                start_time = None
    return result
