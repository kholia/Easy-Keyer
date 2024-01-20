# https://github.com/arduino/arduino-cli/releases

port := $(shell python3 board_detect.py)

default:
	arduino-cli compile --fqbn=esp8266:esp8266:d1_mini_clone code

upload:
	@# echo $(port)
	arduino-cli compile --fqbn=esp8266:esp8266:d1_mini_clone code

install_platform:
	arduino-cli config init --overwrite
	arduino-cli core update-index
	arduino-cli core install esp8266:esp8266

deps:
	arduino-cli lib install "EspSoftwareSerial"
	arduino-cli lib install "RTClib"
	arduino-cli lib install "Time"

install_arduino_cli:
	curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=~/.local/bin sh
