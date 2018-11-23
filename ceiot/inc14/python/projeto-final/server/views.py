from django.http import HttpResponseRedirect
from django.urls import reverse
from django.views import generic
from django.contrib import messages

from projetofinal import board_bridge
from .models import ControlBoard


class IndexView(generic.ListView):
    template_name = 'server/index.html'
    context_object_name = 'boards_list'
    queryset = ControlBoard.objects.all()


def set_led(request):
    boards = ControlBoard.objects.all()
    nothing_to_do = 0
    for board in boards:
        command = int(request.POST['led_value_{}'.format(board.nickname)])
        if board.last_led_level != command:
            board.last_led_level = command
            board.save()
            if not board_bridge.send_command(board, command):
                messages.error(request, 'Message could not be sent to device {}!'.format(board.nickname))
        else:
            nothing_to_do += 1
    if nothing_to_do == len(boards):
        messages.info(request, 'Nothing to do!')

    return HttpResponseRedirect(reverse('server:index'))
