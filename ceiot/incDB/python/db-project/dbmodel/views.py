import random

from django.http import HttpResponseRedirect
from django.shortcuts import get_object_or_404
from django.urls import reverse
from django.views import generic

from .models import ControlBoard, Device


class IndexView(generic.ListView):
    template_name = 'dbmodel/index.html'
    context_object_name = 'board_list'
    queryset = ControlBoard.objects.all()


class ControlBoardDetailView(generic.DetailView):
    model = ControlBoard
    context_object_name = 'board'
    template_name = 'dbmodel/board_detail.html'


class DeviceDetailView(generic.DetailView):
    model = Device
    context_object_name = 'device'
    template_name = 'dbmodel/device_detail.html'


def device_update(request, pk):
    device = get_object_or_404(Device, pk=pk)
    status = device.devicestatus_set.create()
    status.value = generate_value(device)
    status.health_state = generate_health_state()
    status.save()
    return HttpResponseRedirect(reverse('dbmodel:device_detail', args=(device.pk,)))


def device_command(request, pk):
    device = get_object_or_404(Device, pk=pk)
    command = device.devicecommand_set.create()
    command.value = generate_on_off()
    command.exec_code = generate_exec_code()
    command.save()
    return HttpResponseRedirect(reverse('dbmodel:device_detail', args=(device.pk,)))


def generate_value(device: Device):
    min_value = float(str(device.device_model.min_value)) if device.device_model.min_value else 0.0
    max_value = float(str(device.device_model.max_value)) if device.device_model.max_value else 0.0
    return random.uniform(min_value, max_value)


def generate_health_state():
    return random.randint(0, 3)


def generate_exec_code():
    return random.randint(1, 5)


def generate_on_off():
    return random.randint(0, 1)
