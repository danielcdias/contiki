final String controlBoardTable = "ControlBoard";
final String idColumn = "idColumn";
final String nicknameColumn = "nicknameColumn";
final String boardModelColumn = "boardModelColumn";
final String macAddressColumn = "macAddressColumn";
final String lastLedLevelColumn = "lastLedLevelColumn";

class ControlBoardHelper {
  static final ControlBoardHelper _instance = ControlBoardHelper.internal();

  factory ControlBoardHelper() => _instance;

  ControlBoardHelper.internal();

  Future<List> getAllBoards() async {
    List<ControlBoard> list = List();
    list.add(ControlBoard.fromMap({
      nicknameColumn: "DCD02",
      boardModelColumn: "TI LAUNCHXL CC2650",
      macAddressColumn: "00:12:4B:82:89:04",
      lastLedLevelColumn: 0,
    }));
    list.add(ControlBoard.fromMap({
      nicknameColumn: "DCD03",
      boardModelColumn: "TI LAUNCHXL CC2650",
      macAddressColumn: "00:12:4B:82:AA:02",
      lastLedLevelColumn: 0,
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

  ControlBoard.fromMap(Map map) {
    id = map[idColumn];
    nickname = map[nicknameColumn];
    boardModel = map[boardModelColumn];
    macAddress = map[macAddressColumn];
    lastLedLevel = map[lastLedLevelColumn];
  }

  Map toMap() {
    Map<String, dynamic> map = {
      nicknameColumn: nickname,
      boardModelColumn: boardModel,
      macAddressColumn: macAddress,
      lastLedLevelColumn: lastLedLevel,
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
        "lastLedLevel: $lastLedLevel";
  }
}
