from django.urls import path
from model import views

app_name = 'model'
urlpatterns = [
    path('', views.IndexView.as_view(), name='index'),
    path('csv/sensorreadevents/', views.get_sensors_read_event_in_csv, name='cav_sensorreadevents'),
]
