from django.http import HttpResponseRedirect
from django.shortcuts import get_object_or_404
from django.urls import reverse
from django.views import generic

from projetofinal import board_bridge
from .models import ControlBoard


class IndexView(generic.ListView):
    template_name = 'server/index.html'
    context_object_name = 'boards_list'

    def get_queryset(self):
        return ControlBoard.objects.all()


def set_led(request, pk):
    board = get_object_or_404(ControlBoard, pk=pk)
    command = int(request.POST['led_value'])
    board_bridge.send_command(board, command)
    return HttpResponseRedirect(reverse('server:index'))
