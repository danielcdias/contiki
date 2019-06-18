from rest_framework.filters import SearchFilter
from rest_framework.permissions import IsAuthenticated, IsAuthenticatedOrReadOnly
from rest_framework.viewsets import ReadOnlyModelViewSet

from config.expiring_token import ExpiringTokenAuthentication
from model.api.serializers import BoardVendorSerializer, BoardModelSerializer, ControlBoardSerializer, \
    SensorTypeSerializer, SensorSerializer, SensorReadEventSerializer, NotificationUserSerializer, \
    MQTTConnectionSerializer, ControlBoardEventSerializer, ConnectionStatusSerializer
from model.models import BoardVendor, BoardModel, ControlBoard, SensorType, Sensor, SensorReadEvent, NotificationUser, \
    MQTTConnection, ControlBoardEvent, ConnectionStatus


class MQTTConnectionViewSet(ReadOnlyModelViewSet):
    queryset = MQTTConnection.objects.all()
    serializer_class = MQTTConnectionSerializer
    filter_backends = (SearchFilter,)
    permission_classes = (IsAuthenticatedOrReadOnly,)
    authentication_classes = (ExpiringTokenAuthentication,)


class ConnectionStatusViewSet(ReadOnlyModelViewSet):
    queryset = ConnectionStatus.objects.all()
    serializer_class = ConnectionStatusSerializer
    filter_backends = (SearchFilter,)
    search_fields = ('host_connection__hostname', 'timestamp', 'host_status')
    permission_classes = (IsAuthenticatedOrReadOnly,)
    authentication_classes = (ExpiringTokenAuthentication,)


class BoardVendorViewSet(ReadOnlyModelViewSet):
    queryset = BoardVendor.objects.all()
    serializer_class = BoardVendorSerializer
    filter_backends = (SearchFilter,)
    permission_classes = (IsAuthenticatedOrReadOnly,)
    authentication_classes = (ExpiringTokenAuthentication,)


class BoardModelViewSet(ReadOnlyModelViewSet):
    queryset = BoardModel.objects.all()
    serializer_class = BoardModelSerializer
    filter_backends = (SearchFilter,)
    permission_classes = (IsAuthenticatedOrReadOnly,)
    authentication_classes = (ExpiringTokenAuthentication,)


class ControlBoardViewSet(ReadOnlyModelViewSet):
    queryset = ControlBoard.objects.all()
    serializer_class = ControlBoardSerializer
    filter_backends = (SearchFilter,)
    search_fields = 'nickname'
    permission_classes = (IsAuthenticatedOrReadOnly,)
    authentication_classes = (ExpiringTokenAuthentication,)


class ControlBoardEventViewSet(ReadOnlyModelViewSet):
    queryset = ControlBoardEvent.objects.all()
    serializer_class = ControlBoardEventSerializer
    filter_backends = (SearchFilter,)
    search_fields = ('control_board__nickname', 'timestamp', 'status_received')
    permission_classes = (IsAuthenticatedOrReadOnly,)
    authentication_classes = (ExpiringTokenAuthentication,)


class SensorTypeViewSet(ReadOnlyModelViewSet):
    queryset = SensorType.objects.all()
    serializer_class = SensorTypeSerializer
    filter_backends = (SearchFilter,)
    permission_classes = (IsAuthenticatedOrReadOnly,)
    authentication_classes = (ExpiringTokenAuthentication,)


class SensorViewSet(ReadOnlyModelViewSet):
    queryset = Sensor.objects.all()
    serializer_class = SensorSerializer
    filter_backends = (SearchFilter,)
    search_fields = ('control_board__nickname', 'sensor_id')
    permission_classes = (IsAuthenticatedOrReadOnly,)
    authentication_classes = (ExpiringTokenAuthentication,)


class SensorReadEventViewSet(ReadOnlyModelViewSet):
    queryset = SensorReadEvent.objects.all()
    serializer_class = SensorReadEventSerializer
    filter_backends = (SearchFilter,)
    search_fields = ('sensor__sensor_id', 'timestamp')
    permission_classes = (IsAuthenticatedOrReadOnly,)
    authentication_classes = (ExpiringTokenAuthentication,)


class NotificationUserViewSet(ReadOnlyModelViewSet):
    queryset = NotificationUser.objects.all()
    serializer_class = NotificationUserSerializer
    filter_backends = (SearchFilter,)
    permission_classes = (IsAuthenticated,)
    authentication_classes = (ExpiringTokenAuthentication,)
