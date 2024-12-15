# cncjs-pendant

## Installation

```bash
sudo apt install libusb-1.0-0-dev libudev-dev
npm install node-gyp node-pre-gyp
npm install serialport@9.0.6
npm install node-hid --driver=hidraw --build-from-source --unsafe-perm
# cd in folder
npm install

```

## Usage

```bash
# cd bin and make them executable
chmod +x *
# test with:
./start-pendant.sh
```

create buttons in cncjs to start it and stop it  manually
```bash
/home/pi/cncjs-pendant/bin/pendant.sh start
/home/pi/cncjs-pendant/bin/pendant.sh stop
```
