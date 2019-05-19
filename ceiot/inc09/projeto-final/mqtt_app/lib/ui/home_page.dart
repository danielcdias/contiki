import 'package:flutter/material.dart';
import 'package:mqtt_app/helpers/control_board_helper.dart';
import 'package:mqtt_client/mqtt_client.dart';

const MQTT_SERVER_HOST = "danieldias.mooo.com";
const MQTT_SERVER_PORT = 1883;
const MQTT_SERVER_TIMEOUT = 60;
const CLIENT_ID = "CEIOT-INC14-DCD77";

const MQTT_TOPIC_STATUS = "/projetofinal/sta/";
const MQTT_TOPIC_COMMAND = "/projetofinal/cmd/";

class HomePage extends StatefulWidget {
  @override
  _HomePageState createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  ControlBoardHelper _helper = ControlBoardHelper();
  List<ControlBoard> _boards = List();
  MqttClient _mqtt_client;

  void _startMqttClient() async {
    _mqtt_client = new MqttClient.withPort(MQTT_SERVER_HOST, CLIENT_ID, MQTT_SERVER_PORT);
    _mqtt_client.logging(on: false);
    _mqtt_client.keepAlivePeriod = MQTT_SERVER_TIMEOUT;
    _mqtt_client.onDisconnected = _onDisconnected;
    _mqtt_client.onSubscribed = _onSubscribed;
    _mqtt_client.onConnected = _onConnected;
    try {
      await _mqtt_client.connect();
    } catch (e) {
      print("######### EXAMPLE::client exception - $e");
      _mqtt_client.disconnect();
    }
    for (ControlBoard m in _boards) {
      _mqtt_client.subscribe(
          MQTT_TOPIC_STATUS + m.macAddress.substring(12, 14) + m.macAddress.substring(15, 17), MqttQos.atLeastOnce);
    }
    _mqtt_client.published.listen((MqttPublishMessage message) {
      for (ControlBoard m in _boards) {
        if (message.variableHeader.topicName
            .endsWith(m.macAddress.substring(12, 14) + m.macAddress.substring(15, 17))) {
          setState(() {
            m.active = true;
            int newValue = int.parse(MqttPublishPayload.bytesToStringAsString(message.payload.message));
            if (newValue != m.lastLedLevel) {
              m.lastLedLevel = int.parse(MqttPublishPayload.bytesToStringAsString(message.payload.message));
            }
          });
        }
      }
      print('EXAMPLE::Published notification:: topic is ${message.variableHeader.topicName}, '
          'with Qos ${message.header.qos}. '
          'Message: ${MqttPublishPayload.bytesToStringAsString(message.payload.message)}');
    });
  }

  void _onConnected() {
    print("######### EXAMPLE::OnConnected client callback - Client connection");
  }

  /// The subscribed callback
  void _onSubscribed(String topic) {
    print("######### EXAMPLE::Subscription confirmed for topic $topic");
  }

  /// The unsolicited disconnect callback
  void _onDisconnected() {
    print("######### EXAMPLE::OnDisconnected client callback - Client disconnection");
    for (ControlBoard m in _boards) {
      m.active = false;
    }
  }

  @override
  void initState() {
    super.initState();
    print("######### initState");
    _getAllBoards();
    _startMqttClient();
  }

  @override
  void dispose() {
    _mqtt_client.disconnect();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text("CEIOT - Lista de controladoras"),
        centerTitle: true,
      ),
      body: ListView.builder(
        padding: EdgeInsets.all(10.0),
        itemCount: _boards.length,
        itemBuilder: (context, index) {
          return _controlBoard(context, index);
        },
      ),
    );
  }

  void _getAllBoards() {
    _helper.getAllBoards().then((list) {
      setState(() {
        _boards = list;
      });
    });
  }

  Widget _controlBoard(BuildContext context, int index) {
    return GestureDetector(
      child: Card(
        child: Padding(
          padding: EdgeInsets.all(10.0),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: <Widget>[
              Text(
                "${_boards[index].nickname}-${_boards[index].macAddress}",
                style: TextStyle(
                  color: Colors.black,
                  fontSize: 20.0,
                  fontWeight: FontWeight.bold,
                ),
              ),
              Row(
                crossAxisAlignment: CrossAxisAlignment.center,
                mainAxisAlignment: MainAxisAlignment.start,
                children: <Widget>[
                  ConstrainedBox(
                    constraints: BoxConstraints.tight(Size(230.0, 50.0)),
                    child: Slider(
                      key: Key(DateTime.now().millisecondsSinceEpoch.toString()),
                      min: 0,
                      max: 100,
                      divisions: 10,
                      value: _boards[index].lastLedLevel.toDouble(),
                      onChanged: _boards[index].active
                          ? (newValue) {
                              setState(() => _boards[index].lastLedLevel = newValue.toInt());
                            }
                          : null,
                      onChangeEnd: _boards[index].active
                          ? (newValue) {
                              setState(() => _boards[index].active= false);
                              String pubTopic = MQTT_TOPIC_COMMAND +
                                  _boards[index].macAddress.substring(12, 14) +
                                  _boards[index].macAddress.substring(15, 17);
                              final MqttClientPayloadBuilder builder = MqttClientPayloadBuilder();
                              builder.addString(newValue.toString());
                              _mqtt_client.publishMessage(pubTopic, MqttQos.atLeastOnce, builder.payload);
                            }
                          : null,
                    ),
                  ),
                  Container(
                    padding: EdgeInsets.only(left: 10.0),
                    child: Text(
                      "${_boards[index].lastLedLevel}% PWR",
                      style: TextStyle(
                        color: Colors.black,
                        fontSize: 15.0,
                      ),
                    ),
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    );
  }
}
