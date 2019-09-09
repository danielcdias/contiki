from mqtt_client import MQTTClient
from prefs import log_factory
from rest_client import RESTClient

logger = log_factory.get_new_logger("main")


def main():
    mqtt = MQTTClient()
    mqtt.start()
    rest = RESTClient()
    rest.start()


if __name__ == '__main__':
    logger.info("TV-CWB-IOT Forwarder starting...")
    main()
