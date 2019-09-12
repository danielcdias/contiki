import requests
import time
import json

from encryption_tools import decrypt
import message_queue as queue
from prefs import log_factory, prefs
from threading import Thread, Event

TOKEN_KEY = "Authorization"
TOKEN_VALUE_FORMAT = "Token {}"
CONTENTTYPE_KEY = "Content-Type"
CONTENTTYPE_VALUE = "application/json"

logger = None


def get_logger():
    global logger
    if not logger:
        logger = log_factory.get_new_logger("rest_client")
    return logger


class RESTClient(Thread):

    def __init__(self):
        super(RESTClient, self).__init__()
        self._token = ""

    def retrieve_token(self) -> bool:
        result = False
        data = {'username': decrypt(prefs['web-service']['username']),
                'password': decrypt(prefs['web-service']['password'])}
        url = prefs['web-service']['base_url'] + prefs['web-service']['get_token_service']
        resp = requests.post(url, data=data)
        if resp.status_code == 200:
            self._token = resp.json()['token']
            result = True
        else:
            get_logger().error("Token for REST API could not be retrieved. Status code = {}, Content = {}".format(
                resp.status_code, resp.text))
        return result

    def send_message(self, message: dict):
        url = prefs['web-service']['base_url'] + prefs['web-service']['message_receiver_service']
        headers = {
            CONTENTTYPE_KEY: CONTENTTYPE_VALUE,
            TOKEN_KEY: TOKEN_VALUE_FORMAT.format(self._token)
        }
        data = {"topic": message['topic'], "message": message['payload']}
        resp = requests.post(url, headers=headers, data=json.dumps(data))
        if resp.status_code == 201:
            if queue.update_message_as_sent(message['id']):
                get_logger().debug("Message {} sent with success!".format(message))
            else:
                get_logger().error("Sent flag could not be updated in the queue db.")
        elif resp.status_code == 401:
            self.retrieve_token()
        else:
            get_logger().error("Message could not be sent. Status code = {}, Content = {}".format(
                resp.status_code, resp.text))

    def run(self):
        get_logger().info("Starting REST client...")
        if self.retrieve_token():
            while True:
                for message in queue.get_all_not_sent():
                    self.send_message(message)
                time.sleep(1)
        else:
            get_logger().error("Abandoning thread...")
