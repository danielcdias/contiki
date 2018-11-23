from django.urls import path
from server import views

app_name = 'server'
urlpatterns = [
    path('', views.IndexView.as_view(), name='index'),
    path('board/setled/', views.set_led, name='set_led'),
]
