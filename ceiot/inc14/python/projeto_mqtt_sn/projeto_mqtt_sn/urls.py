
import sys

from django.contrib import admin
from django.urls import include, path

from projeto_mqtt_sn import bridge_mqttsn

urlpatterns = [
    path('', include('webapp.urls')),
    path('admin/', admin.site.urls),
]

if sys.argv[1] == "runserver":
    bridge_mqttsn.run()
