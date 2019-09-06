import logging
import os

from logging.handlers import RotatingFileHandler

#: Log line format
LOG_FORMAT = '%(asctime)-15s [%(levelname)s] %(message)s'


class Logger:

    initialized = False

    def __init__(self, name: str = 'application', folder: str = '.', filename: str = 'application.log',
                 console_level: str = "INFO", file_level: str = "INFO"):
        if not self.initialized:
            self.initialized = True
            self._log_name = name
            self._log_folder = folder
            self._log_filename = folder + os.sep + filename
            self._console_level = Logger._translate_level(console_level)
            self._file_level = Logger._translate_level(file_level)
            if not os.path.exists(self._log_folder):
                os.mkdir(self._log_folder)
            should_roll_over = os.path.isfile(self._log_filename)
            self._logger = logging.getLogger(self._log_name)
            self._logger.setLevel(logging.DEBUG)
            formatter = logging.Formatter(LOG_FORMAT)
            ch = logging.StreamHandler()
            ch.setLevel(self._console_level)
            ch.setFormatter(formatter)
            fh = RotatingFileHandler(
                self._log_filename, mode='a', backupCount=50)
            fh.setLevel(self._file_level)
            fh.setFormatter(formatter)
            if should_roll_over:
                fh.doRollover()
            self._logger.addHandler(ch)
            self._logger.addHandler(fh)

    def __new__(cls, *args, **kwargs):
        if not hasattr(cls, '_inst'):
            cls._inst = super(Logger, cls).__new__(cls)
        return cls._inst

    @staticmethod
    def _translate_level(level):
        lv = logging.INFO
        if level == "DEBUG":
            lv = logging.DEBUG
        elif level == "WARNING":
            lv = logging.WARNING
        elif level == "ERROR":
            lv = logging.ERROR
        elif level == "CRITICAL":
            lv = logging.CRITICAL
        return lv

    def get_logger(self):
        return self._logger
