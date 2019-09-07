@ECHO OFF
CALL venv\Scripts\activate.bat
DEL __pycache__\*.* /F /Q
DEL build\*.* /F /Q
DEL dist\*.* /F /Q
RMDIR dist
pyinstaller calibration-tool.spec
COPY sensors-calibration-prefs.json dist
COPY calibration-tool.ico dist
CALL venv\Scripts\deactivate.bat
