from django.contrib.auth.models import User
from rest_framework.serializers import ModelSerializer

from model.models import BoardVendor, BoardModel, ControlBoard, SensorType, Sensor, SensorReadEvent, NotificationUser, \
    ErrorReport


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
        fields = ('id', 'nickname', 'mac_address', 'board_model',)


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
    sensor = SensorSerializer()

    class Meta:
        model = SensorReadEvent
        fields = ('id', 'timestamp', 'value_read', 'sensor',)


class UserSerializer(ModelSerializer):
    class Meta:
        model = User
        fields = ('id', 'username', 'email',)


class NotificationUserSerializer(ModelSerializer):
    user = UserSerializer()

    class Meta:
        model = NotificationUser
        fields = ('id', 'notify_errors', 'user',)


class ErrorReportSerializer(ModelSerializer):
    class Meta:
        model = ErrorReport
        fields = ('id', 'timestamp', 'error_type', 'error_type_description', 'details',)
