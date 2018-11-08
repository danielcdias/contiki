from django.contrib import admin

from .models import ControlBoard


class ControlBoardAdmin(admin.ModelAdmin):
    model = ControlBoard
    extra = 0
    ordering = ('nickname',)


admin.site.register(ControlBoard, ControlBoardAdmin)
