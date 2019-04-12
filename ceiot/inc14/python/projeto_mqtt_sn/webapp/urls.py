from django.urls import path
from webapp import views

app_name = 'webapp'
urlpatterns = [
    path('', views.IndexView.as_view(), name='index'),
    path('board/setled/', views.set_led, name='set_led'),
]
