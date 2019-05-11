import 'package:flutter/material.dart';
import 'package:mqtt_app/helpers/control_board_helper.dart';
import 'package:mqtt_client/mqtt_client.dart';

const MQTT_SERVER_HOST = "danieldias.mooo.com";
const MQTT_SERVER_PORT = 1883;
const MQTT_SERVER_TIMEOUT = 60;
const CLIENT_ID = "CEIOT-INC14-DCD77";

const MQTT_TOPIC_STATUS = "/projetofinal/sta/#";
const MQTT_TOPIC_COMMAND = "/projetofinal/cmd/";

class HomePage extends StatefulWidget {
  @override
  _HomePageState createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  ControlBoardHelper _helper = ControlBoardHelper();
  List<ControlBoard> _boards = List();
  MqttClient mqtt_client;

  void _startMqttClient() async {
    mqtt_client = new MqttClient.withPort(MQTT_SERVER_HOST, CLIENT_ID, MQTT_SERVER_PORT);
    mqtt_client.logging(on: false);
    mqtt_client.keepAlivePeriod = MQTT_SERVER_TIMEOUT;
    mqtt_client.onDisconnected = _onDisconnected;
    mqtt_client.onSubscribed = _onSubscribed;
    mqtt_client.onConnected = _onConnected;
    try {
      await mqtt_client.connect();
    } catch (e) {
      print("######### EXAMPLE::client exception - $e");
      mqtt_client.disconnect();
    }
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
  }

  @override
  void initState() {
    super.initState();
    print("######### initState");
    _getAllBoards();
    _startMqttClient();
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
                      onChanged: (newValue) {
                        setState(() => _boards[index].lastLedLevel = newValue.toInt());
                      },
                      onChangeEnd: (newValue) {
                        print("${_boards[index].nickname} - value: ${_boards[index].lastLedLevel}");
                      },
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
