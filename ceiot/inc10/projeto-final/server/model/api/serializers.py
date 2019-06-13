from django.contrib.auth.models import User
from rest_framework.serializers import ModelSerializer, CharField

from model.models import BoardVendor, BoardModel, ControlBoard, SensorType, Sensor, SensorReadEvent, NotificationUser, \
    ErrorReport, ControlBoardEvent, MQTTConnection


class MQTTConnectionSerializer(ModelSerializer):
    class Meta:
        model = MQTTConnection
        fields = ('id', 'hostname', 'port', 'last_status')


class BoardVendorSerializer(ModelSerializer):
    class Meta:
        model = BoardVendor
        fields = ('id', 'description',)


class BoardModelSerializer(ModelSerializer):
    board_vendor = BoardVendorSerializer()

    class Meta:
        model = BoardModel
        fields = ('id', 'description', 'board_vendor',)


class ControlBoardSerializer(ModelSerializer):
    board_model = BoardModelSerializer()

    class Meta:
        model = ControlBoard
        fields = ('id', 'nickname', 'mac_address', 'board_model', 'short_mac_id')


class ControlBoardEventSerializer(ModelSerializer):
    short_mac_id = CharField(source='control_board.short_mac_id', read_only=True)

    class Meta:
        model = ControlBoardEvent
        fields = ('id', 'timestamp', 'status_received', 'short_mac_id')


class SensorTypeSerializer(ModelSerializer):
    class Meta:
        model = SensorType
        fields = ('id', 'sensor_type', 'description', 'data_sheet', 'precision',)


class SensorSerializer(ModelSerializer):
    sensor_type = SensorTypeSerializer()
    control_board = ControlBoardSerializer()

    class Meta:
        model = Sensor
        fields = ('id', 'sensor_id', 'description', 'sensor_type', 'control_board',)


class SensorReadEventSerializer(ModelSerializer):
    sensor_id = CharField(source='sensor.sensor_id', read_only=True)

    class Meta:
        model = SensorReadEvent
        fields = ('id', 'timestamp', 'value_read', 'sensor_id',)


class UserSerializer(ModelSerializer):
    class Meta:
        model = User
        fields = ('id', 'username', 'email',)


class NotificationUserSerializer(ModelSerializer):
    user = UserSerializer(read_only=True)

    class Meta:
        model = NotificationUser
        fields = ('id', 'notify_errors', 'user',)


class ErrorReportSerializer(ModelSerializer):
    class Meta:
        model = ErrorReport
        fields = ('id', 'timestamp', 'error_type', 'error_type_description', 'details',)
