FROM python:3.7
ENV PYTHONUNBUFFERED 1
RUN mkdir /app
RUN mkdir /app/static
COPY ./code/ /app/
WORKDIR /app
RUN pip install -r requirements.txt
ADD . /app
# python manage.py makemigrations && \
# python manage.py migrate
# ENV DJANGO_SU_PASSWORD=$IOT_DJANGO_SU_PASS
# RUN python -c "import django; import os; os.environ['DJANGO_SETTINGS_MODULE'] = 'config.settings'; django.setup(); \
#    from django.contrib.auth.management.commands.createsuperuser import get_user_model; \
#    get_user_model()._default_manager.db_manager('default').create_superuser( \
#    username='admin', \
#    email='daniel.dias@gmail.com', \
#    password='$DJANGO_SU_PASSWORD')"
