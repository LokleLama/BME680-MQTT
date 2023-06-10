# Installing

## 1. Install Node RED

https://nodered.org/docs/getting-started/raspberrypi

``` bash
bash <(curl -sL https://raw.githubusercontent.com/node-red/linux-installers/master/deb/update-nodejs-and-nodered)
```

Install: ***node-red-dashboard*** & ***node-red-contrib-themes*** in the NodeRed Panel

## 2. Install Mosquitto Server

``` bash
sudo apt install mosquitto mosquitto-clients libmosquitto-dev
```

## 3. Enable & start servers

``` bash
sudo systemctl enable mosquitto
sudo systemctl enable nodered
sudo systemctl start mosquitto
sudo systemctl start nodered
```

## 3. Setup NodeRed Dashboard

Import *flow.json* in Node Red

## 4. Start app

``` bash
./bme680 -a localhost -i 5
```
