from django.urls import path
from model import views

app_name = 'model'
urlpatterns = [
    path('', views.IndexView.as_view(), name='index'),
]
