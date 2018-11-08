from django.contrib import admin
from django.urls import include, path

from projetofinal import board_bridge

urlpatterns = [
    path('', include('server.urls')),
    path('admin/', admin.site.urls),
]

board_bridge.run_server()
