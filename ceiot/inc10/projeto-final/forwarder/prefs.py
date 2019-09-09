import os

from logging_tool import LoggerFactory
from prefs_loader import Preferences

PREFS_JSON_FILENAME = "forwarder.json"

prefs = None
log_factory = None


def _init():
    global prefs, log_factory
    prefs = Preferences("." + os.sep + PREFS_JSON_FILENAME).get_preferences()
    log_factory = LoggerFactory(prefs['logging']['folder'],
                                prefs['logging']['filename'], prefs['logging']['console_level'],
                                prefs['logging']['file_level'])


_init()
