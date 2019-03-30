from django.urls import path

from dbmodel import views

app_name = 'dbmodel'
urlpatterns = [
    path('', views.IndexView.as_view(), name='index'),
    path('board/<int:pk>/', views.ControlBoardDetailView.as_view(), name='board_detail'),
    path('device/<int:pk>/', views.DeviceDetailView.as_view(), name='device_detail'),
    path('device_update/<int:pk>/', views.device_update, name='device_update'),
    path('device_command/<int:pk>/', views.device_command, name='device_command'),
]
