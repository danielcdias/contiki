@ECHO OFF
CALL venv\Scripts\activate.bat
DEL dist\*.* /F /Q
RMDIR dist
REM pyinstaller -i ".\terraco_verde.ico" -w -F .\main.py -n "calibration-tool.exe" --hidden-import=PyQt5
pyinstaller calibration-tool.spec
COPY sensors-calibration-prefs.json dist
COPY calibration-tool.ico dist
