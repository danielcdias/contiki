import 'package:flutter/material.dart';
import 'package:mobile/helpers/control_board_helper.dart';

class HomePage extends StatefulWidget {
  @override
  _HomePageState createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  ControlBoardHelper _helper = ControlBoardHelper();
  List<ControlBoard> _boards = List();

  @override
  void initState() {
    super.initState();
    _getAllContacts();
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

  void _getAllContacts() {
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
