# Sonoff Reprogram
Reprogram Sonoff Basic to run your own web server, so that you do not need 3rd party app to install on your mobile and not to add another Alexa skill.

**Please note. Make sure your Sonoff is not connected to main power line while you open it. Also once overwritten, you cannot get original firmware to your Sonoff and you no longer can use eWeLink app on your mobile to control this Sonoff. Take extreme care on all the steps, so that you do not brick your Sonoff.**

We have to do following things

1. reprogram your sonoff

2. setup a raspberry pi for node-red (to emulate wemo switch)

3. set up Alexa with devices

4. connect the Sonoff with real electrical device


## Reprogram sonoff
Before you can reprogram you have to setup your FTDI and Sonoff to program mode. Please see this blog, [Random Nerd Tutorials](https://randomnerdtutorials.com/reprogram-sonoff-smart-switch-with-web-server/), it gives step by step process to reprogram your Sonoff. For simple operation, you can use the program given in the tutorial, else get the program "Sonoff-HA.ino" from this project and flash it. This project gives you option to save Sonoff state and set your SSID/Password to any custom one you want.

I have followed step by step process from above page and it works. 

#### Web UI

Upon first boot, the sonoff starts up in AP mode with SSID RelayBoard and the Web-UI is accessible at http://relay.local/ OR http://192.168.4.1 from a computer or mobile phone connected to the RelayBoard Wi-Fi network. The home router SSID and password and other details can be entered and saved (persisted on the Wemos). Upon restarting, the Wemos should connect to the home router and get an IP address from there. Now the Web-UI should still be accessible at http://<IP_Address_of_Sonoff>/

![Web](https://github.com/dipdawiz/sonoff-reprogram/raw/master/ui.jpg)


#### HTTP Control API

To turn ON the switch, the URL is:
http://<IP_Address_of_Sonoff>/on

To turn OFF the switch, the URL is:
http://<IP_Address_of_Sonoff>/off

http://<IP_Address_of_Sonoff>/getconfig will return a response with savedstate as 0 or 1

## Setup a Wemo emulator

Setup your Raspberry PI and enable node-red (a library that helps you do your home automation [Node Red](https://nodered.org/docs/getting-started/). Once you are done with, this is the easiest part. Install wemo emulator by this command
```
$ npm install node-red-contrib-wemo-emulator
```
For more info how to configure this node, here is the repo for this node - [Wemo Emulator](https://github.com/biddster/node-red-contrib-wemo-emulator). With default configuration of your node-red on your PI, you can now access the node-red url from
```
http://<raspberry-pi-ip>:1880
```
Now drag and drop your wemo emulator on the blank flow and set up along with a http request node in node-red. 
You need to give unique id across your devices, a unique port across your devices and a friendly name.

![emulator](https://github.com/dipdawiz/sonoff-reprogram/raw/master/wemo-node-red-tv.png)

Configure HTTP node, that makes http call to your sonoff
![http-node](https://github.com/dipdawiz/sonoff-reprogram/raw/master/http-node.png)

## Setup Amazon Echo

As soon as you deploy your node-red flow in previous step, you are ready to set up your Echo (Dot/Plus). On your mobile, open Alexa app. From the menu, click on Smart Home. And add a device. it would search and find out as many wemo-emu node you added in your flow. Add them and you are good to give voice command to Alexa/Echo/Computer. Like in my example, if I say, "Alexa, switch on TV", it switch on sonoff.

## Connect Sonoff

In this step you will work with live electricity, so please be cautious. Connect Sonoff to the device you want to control. Two ways it can be connected, they are shown below

![sonoff-connection](https://github.com/dipdawiz/sonoff-reprogram/raw/master/sonoff-connection.png)
