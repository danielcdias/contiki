from rest_framework.authentication import TokenAuthentication
from rest_framework.decorators import action
from rest_framework.filters import SearchFilter
from rest_framework.permissions import IsAuthenticated, IsAuthenticatedOrReadOnly
from rest_framework.response import Response
from rest_framework.viewsets import ModelViewSet

from model.api.serializers import BoardVendorSerializer, BoardModelSerializer, ControlBoardSerializer, \
    SensorTypeSerializer, SensorSerializer, SensorReadEventSerializer, NotificationUserSerializer, ErrorReportSerializer
from model.models import BoardVendor, BoardModel, ControlBoard, SensorType, Sensor, SensorReadEvent, NotificationUser, \
    ErrorReport


class BoardVendorViewSet(ModelViewSet):
    queryset = BoardVendor.objects.all()
    serializer_class = BoardVendorSerializer
    filter_backends = (SearchFilter,)
    permission_classes = (IsAuthenticated,)
    authentication_classes = (TokenAuthentication,)


class BoardModelViewSet(ModelViewSet):
    queryset = BoardModel.objects.all()
    serializer_class = BoardModelSerializer
    filter_backends = (SearchFilter,)
    permission_classes = (IsAuthenticated,)
    authentication_classes = (TokenAuthentication,)


class ControlBoardViewSet(ModelViewSet):
    queryset = ControlBoard.objects.all()
    serializer_class = ControlBoardSerializer
    filter_backends = (SearchFilter,)
    permission_classes = (IsAuthenticated,)
    authentication_classes = (TokenAuthentication,)


class SensorTypeViewSet(ModelViewSet):
    queryset = SensorType.objects.all()
    serializer_class = SensorTypeSerializer
    filter_backends = (SearchFilter,)
    permission_classes = (IsAuthenticated,)
    authentication_classes = (TokenAuthentication,)


class SensorViewSet(ModelViewSet):
    queryset = Sensor.objects.all()
    serializer_class = SensorSerializer
    filter_backends = (SearchFilter,)
    permission_classes = (IsAuthenticated,)
    authentication_classes = (TokenAuthentication,)


class SensorReadEventViewSet(ModelViewSet):
    queryset = SensorReadEvent.objects.all()
    serializer_class = SensorReadEventSerializer
    filter_backends = (SearchFilter,)
    permission_classes = (IsAuthenticatedOrReadOnly,)
    authentication_classes = (TokenAuthentication,)


class NotificationUserViewSet(ModelViewSet):
    queryset = NotificationUser.objects.all()
    serializer_class = NotificationUserSerializer
    filter_backends = (SearchFilter,)
    permission_classes = (IsAuthenticated,)
    authentication_classes = (TokenAuthentication,)


class ErrorReportViewSet(ModelViewSet):
    queryset = ErrorReport.objects.all()
    serializer_class = ErrorReportSerializer
    filter_backends = (SearchFilter,)
    permission_classes = (IsAuthenticated,)
    authentication_classes = (TokenAuthentication,)
