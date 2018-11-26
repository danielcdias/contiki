from django.contrib import admin

from .models import BoardModel, ControlBoard, Device, DeviceModel, Manufacturer


class BoardModelAdmin(admin.ModelAdmin):
    model = BoardModel
    extra = 0
    ordering = ('name',)


class ControlBoardAdmin(admin.ModelAdmin):
    model = ControlBoard
    extra = 0
    ordering = ('nickname',)


class DeviceAdmin(admin.ModelAdmin):
    model = Device
    extra = 0
    ordering = ('device_id',)


class DeviceModelAdmin(admin.ModelAdmin):
    model = DeviceModel
    extra = 0
    ordering = ('name',)


class ManufacturerAdmin(admin.ModelAdmin):
    model = Manufacturer
    extra = 0
    ordering = ('name',)


admin.site.register(BoardModel, BoardModelAdmin)
admin.site.register(ControlBoard, ControlBoardAdmin)
admin.site.register(Device, DeviceAdmin)
admin.site.register(DeviceModel, DeviceModelAdmin)
admin.site.register(Manufacturer, ManufacturerAdmin)
