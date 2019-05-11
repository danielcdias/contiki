import 'package:flutter/material.dart';
import 'package:mqtt_app/ui/home_page.dart';

void main() => runApp(
      MaterialApp(
        home: HomePage(),
        title: "CEIOT - INC4",
        debugShowCheckedModeBanner: false,
        theme: ThemeData.light(),
      ),
    );
