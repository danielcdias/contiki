from django.views import generic

from .models import MQTTConnection


class IndexView(generic.ListView):
    template_name = 'model/index.html'
    context_object_name = 'hosts_list'
    queryset = MQTTConnection.objects.all()
