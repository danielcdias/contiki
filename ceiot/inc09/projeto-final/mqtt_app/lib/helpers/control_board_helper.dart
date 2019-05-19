final String controlBoardTable = "ControlBoard";
final String idColumn = "idColumn";
final String nicknameColumn = "nicknameColumn";
final String boardModelColumn = "boardModelColumn";
final String macAddressColumn = "macAddressColumn";
final String lastLedLevelColumn = "lastLedLevelColumn";
final String activeColumn = "activeColumn";

class ControlBoardHelper {
  static final ControlBoardHelper _instance = ControlBoardHelper.internal();

  factory ControlBoardHelper() => _instance;

  ControlBoardHelper.internal();

  Future<List> getAllBoards() async {
    List<ControlBoard> list = List();
    list.add(ControlBoard.fromMap({
      idColumn: 1,
      nicknameColumn: "DCD02",
      boardModelColumn: "TI LAUNCHXL CC2650",
      macAddressColumn: "00:12:4B:82:89:04",
      lastLedLevelColumn: 0,
      activeColumn: false,
    }));
    list.add(ControlBoard.fromMap({
      idColumn: 2,
      nicknameColumn: "DCD06",
      boardModelColumn: "TI LAUNCHXL CC2650",
      macAddressColumn: "00:12:4B:82:AE:97",
      lastLedLevelColumn: 0,
      activeColumn: false,
    }));
    return list;
  }
}

class ControlBoard {
  int id;
  String nickname;
  String boardModel;
  String macAddress;
  int lastLedLevel;
  bool active;

  ControlBoard.fromMap(Map map) {
    id = map[idColumn];
    nickname = map[nicknameColumn];
    boardModel = map[boardModelColumn];
    macAddress = map[macAddressColumn];
    lastLedLevel = map[lastLedLevelColumn];
    active = map[activeColumn];
  }

  Map toMap() {
    Map<String, dynamic> map = {
      nicknameColumn: nickname,
      boardModelColumn: boardModel,
      macAddressColumn: macAddress,
      lastLedLevelColumn: lastLedLevel,
      activeColumn: active,
    };
    if (id != null) {
      map[idColumn] = id;
    }
    return map;
  }

  @override
  String toString() {
    return "ControlBoard("
        "id: $id, "
        "nickname: $nickname, "
        "boardModel: $boardModel, "
        "macAddress: $macAddress, "
        "lastLedLevel: $lastLedLevel,"
        "active: $active)";
  }
}
