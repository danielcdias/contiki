# Generated by Django 2.2.5 on 2019-09-19 13:44

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('model', '0005_auto_20190918_2348'),
    ]

    operations = [
        migrations.AlterField(
            model_name='controlboardevent',
            name='status_received',
            field=models.CharField(max_length=20, verbose_name='Valor lido'),
        ),
    ]
