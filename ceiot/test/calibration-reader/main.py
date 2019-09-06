import csv
import json
import pickle
import serial
import os
import sqlite3 as sqlite
import sys
import time
import fix_qt_import_error

from copy import deepcopy

from config_util.prefs_loader import Preferences
from jsonschema import validate
from logging_util.logging_tool import Logger
from PyQt5 import QtGui
from PyQt5.QtCore import *
from PyQt5.QtWidgets import *
from PyQt5.QtGui import *
from serial.tools import list_ports
from threading import Thread, Event

PREFS_JSON_FILENAME = "sensors-calibration-prefs.json"

CC2650_USB_DESCRIPTION = "XDS110 Class Application/User UART"

DB_FOLDER = "." + os.sep + "db"
DB_FILE = DB_FOLDER + os.sep + "sensor-calibration.db"

DB_TABLE_CREATE = "CREATE TABLE calibration(id INTEGER PRIMARY KEY AUTOINCREMENT, sensor INTEGER NOT NULL, " \
                  "sample INTEGER NOT NULL, scale INTEGER NOT NULL, value INTEGER NOT NULL)"

DB_TABLE_CHECK = "SELECT name FROM sqlite_master WHERE type='table' AND name='calibration'"

DB_TABLE_INSERT = "INSERT INTO calibration(sensor, sample, scale, value) VALUES (?, ?, ?, ?)"

DB_TABLE_SELECT = "SELECT * FROM calibration"

DB_TABLE_SELECT_COUNT = "SELECT COUNT(*) FROM calibration"

DB_TABLE_CLEAR = "DELETE FROM calibration"

REPOSITORY_FOLDER = "." + os.sep + "data"

REPOSITORY_FILENAME = REPOSITORY_FOLDER + os.sep + "sensor-calibration.dat"

REPOSITORY_OBJECT = {
    "num_sensors": 0,
    "sample_count": 0,
    "scale_interval": 0,
    "i": 0,
    "j": 1,
    "finished": True
}

QGROUPBOX_STYLE = """
QGroupBox {
    font-weight: bold;
    font-size: 13px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
}
QLabel {
    font-size: 12px;
    padding-right: 0px;
    padding-left: 10px;
}
QLineEdit {
    font-size: 12px;
    padding-right: 0px;
    padding-left: 0px;
}
QPushButton {
    font-size: 12px;
    font-weight: bold;
}
QHBoxLayout {
    padding: 20px;
}
QTableView {
    font-size: 12px;
}
QTextEdit  {
    font-size: 12px;
}
QMessageBox QLabel {
    font-size: 13px;
}
"""

JSON_SCHEMA_FOR_DATA_READ = {
    "type": "object",
    "patternProperties": {
        "^[s][1]?[0-9]$": {
            "type": "integer"
        }
    },
    "minProperties": 10,
    "maxProperties": 10,
    "additionalProperties": False
}

_repository = {}

logger = None
prefs = None


def save_repository() -> bool:
    logger.debug("Saving repository...")
    result = False
    try:
        with open(REPOSITORY_FILENAME, 'wb') as f:
            pickle.dump(_repository, f, pickle.HIGHEST_PROTOCOL)
        result = True
    except Exception as e:
        logger.error("Error trying to create data repository Exception: {}".format(e))
    return result


def init_repository() -> bool:
    logger.debug("Initializing repository...")
    result = False
    global _repository
    if not os.path.exists(REPOSITORY_FOLDER):
        os.mkdir(REPOSITORY_FOLDER)
    if os.path.exists(REPOSITORY_FILENAME):
        try:
            with open(REPOSITORY_FILENAME, 'rb') as f:
                _repository = pickle.load(f)
            result = True
        except Exception as e:
            logger.error("Could not ready data repository. Exception: {}".format(e))
    else:
        _repository = deepcopy(REPOSITORY_OBJECT)
        result = save_repository()
    return result


def check_db():
    logger.debug("Checking database...")
    if not os.path.exists(DB_FILE):
        os.mkdir(DB_FOLDER)
    if not os.path.exists(DB_FILE):
        try:
            conn = sqlite.connect(DB_FILE)
            try:
                conn = sqlite.connect(DB_FILE)
                cursor = conn.cursor()
                cursor.execute(DB_TABLE_CHECK)
                row = cursor.fetchone()
                if not row:
                    cursor.execute(DB_TABLE_CREATE)
                cursor.close()
            except Exception as ex:
                if "no such table: calibration" in str(ex):
                    cursor = conn.cursor()
                    cursor.execute(DB_TABLE_CREATE)
                    cursor.close()
                else:
                    logger.error(
                        "Could not create the database (1). Exception: {}".format(ex))
            finally:
                conn.close()
        except Exception as ex:
            logger.error(
                "Could not create the database (2). Exception: {}".format(ex))


def connect() -> sqlite.Connection:
    logger.debug("Connecting to database...")
    result = None
    check_db()
    try:
        result = sqlite.connect(DB_FILE)
    except Exception as ex:
        logger.error(
            "Could not connect to the database. Exception: {}".format(ex))
    return result


def save_reading(sample: int, scale: int, reading: dict, num_sensors: int) -> bool:
    logger.debug("Saving to database...")
    result = False
    conn = connect()
    try:
        if conn:
            cursor = conn.cursor()
            for i in range(1, (num_sensors + 1)):
                parms = [i, sample, scale, reading['s{}'.format(i)]]
                cursor.execute(DB_TABLE_INSERT, parms)
            conn.commit()
            cursor.close()
            result = True
        else:
            logger.error(
                "Could not insert data in the database.")
    except Exception as ex:
        logger.error(
            "Could not insert data in the database. Exception: {}".format(ex))
        conn.rollback()
    finally:
        if conn:
            conn.close()
    return result


def get_all_readings() -> list:
    logger.debug("Getting all from database...")
    result = []
    conn = connect()
    try:
        if conn:
            cursor = conn.cursor()
            cursor.execute(DB_TABLE_SELECT)
            rows = cursor.fetchall()
            columns = [column[0] for column in cursor.description]
            for row in rows:
                row_as_dict = dict(zip(columns, row))
                result.append(row_as_dict)
            cursor.close()
    except Exception as ex:
        logger.error(
            "Could not read data from the database. Exception: {}".format(ex))
    finally:
        if conn:
            conn.close()
    return result


def get_count_db() -> int:
    logger.debug("Getting count from database...")
    result = 0
    conn = connect()
    try:
        if conn:
            cursor = conn.cursor()
            cursor.execute(DB_TABLE_SELECT_COUNT)
            result = cursor.fetchone()[0]
            cursor.close()
    except Exception as ex:
        logger.error(
            "Could not read data from the database. Exception: {}".format(ex))
    finally:
        if conn:
            conn.close()
    return result


def clear_db() -> bool:
    logger.debug("Clearing database...")
    result = False
    conn = connect()
    try:
        if conn:
            cursor = conn.cursor()
            cursor.execute(DB_TABLE_CLEAR)
            conn.commit()
            cursor.close()
            result = True
        else:
            logger.error(
                "Could not clear data in the database.")
    except Exception as ex:
        logger.error(
            "Could not clear data in the database. Exception: {}".format(ex))
        conn.rollback()
    finally:
        if conn:
            conn.close()
    return result


def get_cc2650_port() -> str:
    logger.debug("Getting serial port...")
    port = ""
    for port_listed in list_ports.comports():
        logger.debug("Serial port listed: {} - {}".format(port_listed.device, port_listed.description))
        port_description_starts_with = CC2650_USB_DESCRIPTION
        if 'control-board' in prefs and 'usb-port-description-start-with' in prefs['control-board']:
            port_description_starts_with = prefs['control-board']['usb-port-description-start-with']
        if port_description_starts_with in port_listed.description:
            port = port_listed.device
    return port


class SerialPortReader(Thread):

    def __init__(self, port: str):
        logger.debug("[SerialPortReader] Thread created.")
        super(SerialPortReader, self).__init__()
        self._stop_event = Event()
        self._port = port
        self.data_read = bytearray()

    def stop(self):
        logger.debug("[SerialPortReader] Stop requested.")
        self._stop_event.set()

    def is_stopped(self):
        return self._stop_event.is_set()

    def run(self) -> None:
        logger.debug("[SerialPortReader] Thread started.")
        while not self.is_stopped():
            with serial.Serial(self._port, 115200, timeout=1) as ser:
                self.data_read = ser.readline()
                if not self.data_read:
                    continue
            while self.data_read:
                time.sleep(0.1)
        logger.debug("[SerialPortReader] Thread ended.")

    def get_data_read(self) -> str:
        data = self.data_read.decode()
        self.data_read = bytearray()
        return data


class CalibrationTool(QWidget):

    def __init__(self):
        logger.debug("[CalibrationTool] QWidget created.")
        super().__init__()
        self.setStyleSheet(QGROUPBOX_STYLE)
        self.setWindowIcon(QIcon('calibration-tool.ico'))
        # self.move(10, 10)
        self.location_on_the_screen()
        self._reading_thread = None
        self._gb_parameters = QGroupBox()
        self._lb_num_sensors = QLabel()
        self._lb_samples = QLabel()
        self._lb_scale = QLabel()
        self._ti_num_sensors = QLineEdit()
        self._ti_samples = QLineEdit()
        self._ti_scale = QLineEdit()
        self._lb_port = QLabel()
        self._ti_port = QLineEdit()
        self._gb_actions = QGroupBox()
        self._bt_start = QPushButton('Iniciar', self)
        self._bt_save = QPushButton('Salvar', self)
        self._bt_cancel = QPushButton('Cancelar', self)
        self._gb_table = QGroupBox()
        self._table = QTableWidget()
        self._gb_messages = QGroupBox()
        self._messages = QTextEdit()
        self._build_window()
        self._cancelling_requested = False

    def location_on_the_screen(self):
        ag = QDesktopWidget().availableGeometry()

        widget = self.geometry()
        x = int((ag.width() / 2) - widget.width())
        self.move(x, 10)

    def _create_reading_thread(self):
        self._reading_thread = SerialPortReader(self._ti_port.text())

    def _build_window(self):
        self.setWindowTitle("Calibração de Sensores Capacitivos")
        layout = QGridLayout()
        # GroupBox Parameters
        self._gb_parameters.setTitle("Parâmetros")
        layout_gb_parameters = QHBoxLayout()
        self._lb_num_sensors.setText("Número de sensores:")
        self._lb_num_sensors.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self._ti_num_sensors.setValidator(QIntValidator())
        self._ti_num_sensors.setMaxLength(2)
        self._ti_num_sensors.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self._ti_num_sensors.setFixedSize(50, 20)
        self._lb_num_sensors.setBuddy(self._ti_num_sensors)
        layout_gb_parameters.addWidget(self._lb_num_sensors)
        layout_gb_parameters.addWidget(self._ti_num_sensors)
        self._lb_samples.setText("Quantidade de amostras:")
        self._lb_samples.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self._ti_samples.setValidator(QIntValidator())
        self._ti_samples.setMaxLength(2)
        self._ti_samples.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self._ti_samples.setFixedSize(50, 20)
        self._lb_samples.setBuddy(self._ti_samples)
        layout_gb_parameters.addWidget(self._lb_samples)
        layout_gb_parameters.addWidget(self._ti_samples)
        self._lb_scale.setText("Intervalo escala de umidade:")
        self._lb_scale.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self._ti_scale.setValidator(QIntValidator())
        self._ti_scale.setMaxLength(2)
        self._ti_scale.setFixedSize(50, 20)
        self._ti_scale.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self._lb_scale.setBuddy(self._ti_scale)
        layout_gb_parameters.addWidget(self._lb_scale)
        layout_gb_parameters.addWidget(self._ti_scale)
        self._lb_port.setText("Porta conexão:")
        self._lb_port.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self._ti_port.setMaxLength(5)
        self._ti_port.setFixedSize(50, 20)
        self._ti_port.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self._ti_port.setDisabled(True)
        self._lb_port.setBuddy(self._ti_port)
        layout_gb_parameters.addWidget(self._lb_port)
        layout_gb_parameters.addWidget(self._ti_port)
        self._gb_parameters.setLayout(layout_gb_parameters)
        layout.addWidget(self._gb_parameters, 1, 1, 1, 3)
        # GroupBox Actions
        self._gb_actions.setTitle("Ações")
        layout_gb_actions = QHBoxLayout()
        self._bt_start.setToolTip("Iniciar calibração")
        self._bt_start.clicked.connect(self.on_click_start)
        layout_gb_actions.addWidget(self._bt_start)
        self._bt_save.setToolTip("Salvar resultados")
        self._bt_save.setDisabled(True)
        self._bt_save.clicked.connect(self.on_click_save_results)
        layout_gb_actions.addWidget(self._bt_save)
        self._bt_cancel.setToolTip("Cancelar calibragem")
        self._bt_cancel.setDisabled(True)
        self._bt_cancel.clicked.connect(self.on_click_cancel)
        layout_gb_actions.addWidget(self._bt_cancel)
        self._gb_actions.setLayout(layout_gb_actions)
        layout.addWidget(self._gb_actions, 2, 1, 1, 3)
        # GroupBox Table
        self._gb_table.setTitle("Valores lidos dos sensores")
        layout_gb_table = QHBoxLayout()
        self._table.setColumnCount(4)
        self._table.setAlternatingRowColors(True)
        self._table.setCornerButtonEnabled(False)
        self._table.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        self._table.setSelectionMode(QAbstractItemView.NoSelection)
        self._table.setHorizontalHeaderLabels(["Sensor", "Amostra", "Escala de umidade", "Valor lido"])
        self._table.resizeColumnsToContents()
        self._table.setFixedHeight(200)
        # self._table.resizeRowsToContents()
        layout_gb_table.addWidget(self._table)
        self._gb_table.setLayout(layout_gb_table)
        layout.addWidget(self._gb_table, 3, 1, 8, 3)
        # GroupBox Messages
        self._gb_messages.setTitle("Mensagens")
        layout_gb_messages = QHBoxLayout()
        self._messages.setFixedHeight(200)
        self._messages.setReadOnly(True)
        layout_gb_messages.addWidget(self._messages)
        self._gb_messages.setLayout(layout_gb_messages)
        layout.addWidget(self._gb_messages, 11, 1, 8, 3)
        # Window configuration
        self.setLayout(layout)
        self.setWindowFlags(Qt.WindowFlags() | Qt.WindowStaysOnTopHint | Qt.WindowMinimizeButtonHint |
                            Qt.WindowCloseButtonHint)
        self._ti_num_sensors.setFocus()

    def show_error(self, message: str):
        logger.error("[ErrorMessage] {}".format(message))
        msg = QMessageBox(self)
        msg.setIcon(QMessageBox.Critical)
        msg.setText(message)
        msg.setWindowTitle("Erro!")
        msg.exec_()

    def show_info(self, message: str):
        logger.info("[InfoMessage] {}".format(message))
        msg = QMessageBox(self)
        msg.setIcon(QMessageBox.Information)
        msg.setText(message)
        msg.setWindowTitle("Aviso!")
        msg.exec_()

    def togle_start_widgets(self, disabled: bool):
        self._bt_start.setDisabled(disabled)
        self._ti_num_sensors.setDisabled(disabled)
        self._ti_samples.setDisabled(disabled)
        self._ti_scale.setDisabled(disabled)

    def validate_input(self, lineedit: QLineEdit, input_name: str, min_val: int, max_val: int) -> bool:
        result = True
        if not lineedit.text().strip():
            result = False
        else:
            try:
                value = int(lineedit.text())
                if (value < min_val) or (value > max_val):
                    result = False
            except ValueError:
                result = False
        if not result:
            self.show_error(
                "Por favor, informe um valor inteiro entre {} e {} para campo {}.".format(min_val, max_val,
                                                                                          input_name.replace(':', '')))
            self.togle_start_widgets(False)
            lineedit.selectAll()
            lineedit.setFocus()
        return result

    @pyqtSlot()
    def on_click_cancel(self):
        logger.debug("[CalibrationTool] Cancelling calibration...")
        qm = QMessageBox(self)
        qm.setIcon(QMessageBox.Warning)
        qm.setText("Em caso de cancelamento, todos oa valores lidos até o momento "
                   "serão perdidos. Tem certeza que deseja cancelar a calibragem?")
        qm.setWindowTitle("Cancelar calibragem")
        qm.setStandardButtons(QMessageBox.Yes | QMessageBox.No)
        ret = qm.exec_()
        if ret == qm.Yes:
            self._cancelling_requested = True
            if self._reading_thread and self._reading_thread.is_alive():
                self._reading_thread.stop()
                while self._reading_thread.is_alive():
                    time.sleep(1)

    @pyqtSlot()
    def on_click_start(self):
        logger.debug("[CalibrationTool] Start process...")
        logger.debug("[CalibrationTool] num_sensors = '{}'".format(self._ti_num_sensors.text()))
        self.togle_start_widgets(True)
        if self.validate_input(self._ti_num_sensors, self._lb_num_sensors.text(), 1, 10):
            logger.debug("[CalibrationTool] samples = '{}'".format(self._ti_samples.text()))
            if self.validate_input(self._ti_samples, self._lb_samples.text(), 1, 50):
                logger.debug("[CalibrationTool] scales = '{}'".format(self._ti_scale.text()))
                if self.validate_input(self._ti_scale, self._lb_scale.text(), 1, 50):
                    port = get_cc2650_port()
                    if port:
                        self._ti_port.setText(port)
                    while not self._ti_port.text():
                        qm = QMessageBox()
                        ret = qm.question(self, "Ação necessária",
                                          "Por favor, conecte a controladora CC2650 em uma porta USB, aguarde 10 "
                                          "segundos e clique no botão OK abaixo.", qm.Ok | qm.Cancel)
                        if ret == qm.Ok:
                            port = get_cc2650_port()
                            if port:
                                self._ti_port.setText(port)
                        else:
                            break
                    if self._ti_port.text():
                        logger.debug("[CalibrationTool] Serial port set to {}.".format(self._ti_port.text()))
                        global _repository
                        db_ready = False
                        if _repository['finished'] and get_count_db() > 0:
                            qm = QMessageBox()
                            ret = qm.question(self, "Ação necessária",
                                              "Esta ferramenta já foi executada e o banco de dados já contem "
                                              "informação. Deseja sobrescrever estes dados?", qm.Yes | qm.No)
                            if ret == qm.Yes:
                                if clear_db():
                                    db_ready = True
                                else:
                                    self.show_error("Não foi possível apagar o banco de dados. "
                                                    "Por favor, verifique os logs da aplicação.")
                        else:
                            db_ready = True
                        if db_ready:
                            _repository['num_sensors'] = self._ti_num_sensors.text()
                            _repository['sample_count'] = self._ti_samples.text()
                            _repository['scale_interval'] = self._ti_scale.text()
                            if not save_repository():
                                self.show_error("Não foi possível salvar o repositório de dados. "
                                                "Por favor, verifique os logs da aplicação.")
                                self.togle_start_widgets(False)
                            else:
                                QApplication.processEvents()
                                self._bt_save.setDisabled(True)
                                if self._table.rowCount() > 0 and _repository['finished']:
                                    self._table.setRowCount(0)
                                self._messages.clear()
                                self.perform_calibration()
                        else:
                            self.togle_start_widgets(False)
                    else:
                        self.togle_start_widgets(False)

    @pyqtSlot()
    def on_click_save_results(self):
        logger.debug("[CalibrationTool] Saving results to CSV file...")
        csv_filename = "calibration-results.csv"
        try:
            with open('./' + csv_filename, 'w', newline='') as csvfile:
                spamwriter = csv.writer(csvfile, delimiter=';',
                                        quotechar='"', quoting=csv.QUOTE_ALL)
                header = ["id", "sensor", "amostra", "escala", "valor"]
                spamwriter.writerow(header)
                all_readings = get_all_readings()
                if all_readings:
                    for reading in all_readings:
                        reading_to_list = [reading[elem] for elem in reading]
                        spamwriter.writerow(reading_to_list)
                    self.show_info("Arquivo CSV {} gerado com sucesso na pasta do aplicativo.".format(csv_filename))
                    logger.info("[CalibrationTool] CSV file generated with success.")
                else:
                    self.show_error("Não há dados no banco de dados para gerar o arquivo CSV.")
                    logger.error("[CalibrationTool] Database is empty.")
        except Exception as ex:
            logger.error("[CalibrationTool] Could not generate CSV file. Exception: {}".format(ex))
            self.show_error(
                "Ocorreu um erro tentando salvar os resultados em CSV. Por favor, consulte os logs da aplicação.")
        self._bt_save.setDisabled(True)
        self.togle_start_widgets(False)

    def set_config(self, num_sensors: int, samples: int, scales: int, disable_inputs: bool = True):
        self._ti_num_sensors.setText(str(num_sensors))
        self._ti_samples.setText(str(samples))
        self._ti_scale.setText(str(scales))
        if disable_inputs:
            self._ti_num_sensors.setDisabled(True)
            self._ti_samples.setDisabled(True)
            self._ti_scale.setDisabled(True)

    def add_message(self, message: str):
        self._messages.append(message)
        self._messages.moveCursor(QTextCursor.End)

    def add_line_to_table(self, sensor: str, sample: str, scale: str, value: str):
        r = self._table.rowCount()
        self._table.insertRow(r)
        self._table.setRowHeight(r, 12)
        sensor_item = QTableWidgetItem(sensor)
        sample_item = QTableWidgetItem(sample)
        scale_item = QTableWidgetItem(scale + " %")
        value_item = QTableWidgetItem(value)
        sensor_item.setTextAlignment(Qt.AlignCenter | Qt.AlignVCenter)
        sample_item.setTextAlignment(Qt.AlignRight | Qt.AlignVCenter)
        scale_item.setTextAlignment(Qt.AlignRight | Qt.AlignVCenter)
        value_item.setTextAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self._table.setItem(r, 0, sensor_item)
        self._table.setItem(r, 1, sample_item)
        self._table.setItem(r, 2, scale_item)
        self._table.setItem(r, 3, value_item)
        item = self._table.item(r, 0)
        self._table.scrollToItem(item, QAbstractItemView.PositionAtBottom)

    def load_table(self):
        all_readings = get_all_readings()
        if all_readings:
            for reading in all_readings:
                self.add_line_to_table("Sensor " + str(reading['sensor']), str(reading['sample']),
                                       str(reading['scale']), str(reading['value']))
            self._bt_save.setDisabled(False)
            item = self._table.item(0, 0)
            self._table.scrollToItem(item, QAbstractItemView.PositionAtTop)

    def add_reading_to_table(self, sensors_reading: dict, num_sensors: int, sample: int, scale: int):
        for i in range(1, (num_sensors + 1)):
            key = "s{}".format(i)
            sensor_name = "Sensor {}".format(i)
            self.add_line_to_table(sensor_name, str(sample), str(scale), str(sensors_reading[key]))

    def confirm_reading(self, sensors_reading: dict) -> bool:
        result = False
        message = "Por favor, confirme se a leitura efetuada está correta:\n"
        for key in sensors_reading:
            message += "Sensor {}: {}\n".format(key[1:], sensors_reading[key])
        qm = QMessageBox()
        ret = qm.question(self, "Ação necessária", message, qm.Yes | qm.No)
        if ret == qm.Yes:
            result = True
        return result

    def validate_data_read(self, data_read: str) -> dict:
        result = {}
        if data_read:
            try:
                num_sensors = int(self._ti_num_sensors.text())
                schema = deepcopy(JSON_SCHEMA_FOR_DATA_READ)
                schema['minProperties'] = num_sensors
                schema['maxProperties'] = num_sensors
                result = json.loads(data_read.replace("'", "\""))
                validate(instance=result, schema=schema)
            except ValueError:
                logger.error("[CalibrationTool] The value read is invalid: {}".format(data_read.strip('\n')))
                self.show_error(
                    "O valor lido é inválido: {}. Tente novamente.".format(data_read.strip('\n')))
                result = {}
            except Exception as ex:
                if "Failed validating 'maxProperties' in schema" in str(ex) or \
                        "Failed validating 'minProperties' in schema" in str(ex):
                    logger.error("[CalibrationTool] Error validating JSON read from serial. "
                                 "Wrong number of properties (sensors). Exception: {}".format(ex))
                    self.show_error(
                        "O valor lido não tem o número correto de sensores: {}. "
                        "Por favor, corrija o número de sensores ou verifique se a controladora "
                        "está programada corretamente.".format(data_read.strip('\n')))
                else:
                    logger.error("[CalibrationTool] Error validating JSON read from serial. "
                                 "Exception: {}".format(ex))
                    self.show_error(
                        "O valor lido não está corretamente formatado: {}.".format(data_read.strip('\n')))
                result = {}
        return result

    def perform_calibration(self):
        logger.debug("[CalibrationTool] Starting calibration...")
        self._bt_cancel.setDisabled(False)
        self._cancelling_requested = False
        global _repository
        self._messages.setText("")
        start_i = _repository['i']
        start_j = _repository['j']
        self._create_reading_thread()
        error_found = False
        sensors_reading = {}
        for i in range(start_i, (100 + int(self._ti_scale.text())), int(self._ti_scale.text())):
            for j in range(start_j, (int(self._ti_samples.text()) + 1)):
                while not self._cancelling_requested:
                    sensors_reading = {}
                    while not sensors_reading and (not self._cancelling_requested):
                        if not self._reading_thread.is_alive():
                            self._reading_thread.start()
                        logger.debug(
                            "[CalibrationTool] Waiting reading for sample {} with {}% humidity...".format(j, i))
                        self.add_message("Aguardando leitura para amostra {} com {}% de umidade...".format(j, i))
                        self._messages.setFocus()
                        reading = ""
                        while not reading and (not self._cancelling_requested):
                            QApplication.processEvents()
                            reading = self._reading_thread.get_data_read()
                            time.sleep(0.1)
                        sensors_reading = self.validate_data_read(reading)
                        if self._cancelling_requested:
                            break
                    if self._cancelling_requested:
                        break
                    logger.debug("[CalibrationTool] Value read: {}".format(sensors_reading))
                    self.add_message("Leitura: {}".format(sensors_reading))
                    if self.confirm_reading(sensors_reading):
                        logger.debug("[CalibrationTool] Value accepted!")
                        break
                    else:
                        logger.debug("[CalibrationTool] Value rejected!")
                if self._cancelling_requested:
                    break
                if save_reading(j, i, sensors_reading, int(self._ti_num_sensors.text())):
                    _repository['finished'] = False
                    _repository['i'] = i
                    _repository['j'] = (j + 1)
                    self.add_reading_to_table(sensors_reading, int(self._ti_num_sensors.text()), j, i)
                    if not save_repository():
                        self.show_error("Não foi possível salvar o repositório de dados. "
                                        "Por favor, verifique os logs da aplicação.")
                        error_found = True
                        break
                else:
                    self.show_error("Não foi possível salvar no banco dados. "
                                    "Por favor, verifique os logs da aplicação.")
                    error_found = True
                    break
            if self._cancelling_requested:
                break
            if error_found:
                break
            start_j = 1
        if self._reading_thread and self._reading_thread.is_alive():
            self._reading_thread.stop()
        self._bt_cancel.setDisabled(True)
        if not error_found and (not self._cancelling_requested):
            self._bt_save.setDisabled(False)
            logger.debug("[CalibrationTool] Calibration completed with success!")
            self.add_message("***** Calibragem de sensores finalizada com sucesso! *****")
            self.show_info("Calibragem de sensores finalizada com sucesso!")
            _repository['finished'] = True
            _repository['i'] = 0
            _repository['j'] = 1
            if not save_repository():
                self.show_error("Não foi possível salvar o repositório de dados. "
                                "Por favor, verifique os logs da aplicação.")
        if self._cancelling_requested:
            logger.debug("[CalibrationTool] Calibration cancelled!")
            self.add_message("***** Calibragem cancelada! *****")
            self.show_info("A calibragem de sensores foi cancelada!")
            self._bt_start.setDisabled(False)
            self.togle_start_widgets(False)
            _repository['finished'] = True
            _repository['i'] = 0
            _repository['j'] = 1
            if not save_repository():
                self.show_error("Não foi possível salvar o repositório de dados. "
                                "Por favor, verifique os logs da aplicação.")

    def closeEvent(self, event: QtGui.QCloseEvent) -> None:
        super().closeEvent(event)
        logger.debug("[CalibrationTool] closeEvent from QWidget called.")
        if self._reading_thread and self._reading_thread.is_alive():
            self._reading_thread.stop()
            while self._reading_thread.is_alive():
                time.sleep(1)
            sys.exit(0)


def main():
    global prefs, logger, _repository
    prefs = Preferences("." + os.sep + PREFS_JSON_FILENAME).get_preferences()
    logger = Logger(prefs['logging']['name'], prefs['logging']['folder'],
                    prefs['logging']['filename'], prefs['logging']['console_level'],
                    prefs['logging']['file_level']).get_logger()
    logger.debug("Starting app...")
    if init_repository():
        app = QApplication([])
        app.setStyle('Fusion')
        window = CalibrationTool()
        if not _repository['finished']:
            window.set_config(_repository['num_sensors'], _repository['sample_count'],
                              _repository['scale_interval'])
        else:
            window.set_config(prefs['application']['default_num_sensors'], prefs['application']['default_samples'],
                              prefs['application']['default_scale'], disable_inputs=False)
        window.load_table()
        window.show()
        window.setFixedSize(window.width(), window.height())
        sys.exit(app.exec_())
    else:
        sys.exit(-1)


if __name__ == "__main__":
    main()
