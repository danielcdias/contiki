from rest_framework.filters import SearchFilter
from rest_framework.permissions import IsAuthenticated, IsAuthenticatedOrReadOnly
from rest_framework.viewsets import ModelViewSet, ReadOnlyModelViewSet

from config.expiring_token import ExpiringTokenAuthentication
from model.api.serializers import BoardVendorSerializer, BoardModelSerializer, ControlBoardSerializer, \
    SensorTypeSerializer, SensorSerializer, SensorReadEventSerializer, NotificationUserSerializer, ErrorReportSerializer
from model.models import BoardVendor, BoardModel, ControlBoard, SensorType, Sensor, SensorReadEvent, NotificationUser, \
    ErrorReport


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
    permission_classes = (IsAuthenticatedOrReadOnly,)
    authentication_classes = (ExpiringTokenAuthentication,)


class SensorReadEventViewSet(ReadOnlyModelViewSet):
    queryset = SensorReadEvent.objects.all()
    serializer_class = SensorReadEventSerializer
    filter_backends = (SearchFilter,)
    permission_classes = (IsAuthenticatedOrReadOnly,)
    authentication_classes = (ExpiringTokenAuthentication,)


class NotificationUserViewSet(ReadOnlyModelViewSet):
    queryset = NotificationUser.objects.all()
    serializer_class = NotificationUserSerializer
    filter_backends = (SearchFilter,)
    permission_classes = (IsAuthenticated,)
    authentication_classes = (ExpiringTokenAuthentication,)


class ErrorReportViewSet(ReadOnlyModelViewSet):
    queryset = ErrorReport.objects.all()
    serializer_class = ErrorReportSerializer
    filter_backends = (SearchFilter,)
    permission_classes = (IsAuthenticated,)
    authentication_classes = (ExpiringTokenAuthentication,)
