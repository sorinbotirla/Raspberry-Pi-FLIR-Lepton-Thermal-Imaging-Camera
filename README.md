# Raspberry Pi FLIR Lepton Thermal Imaging Camera
A Thermal Imaging camera built on Raspberry Pi and FLIR Lepton module

<table>
  <tr style="border: 0;">
    <td width="33%" valign="top" style="vertical-align: top;">
      <img src="https://raw.githubusercontent.com/sorinbotirla/Raspberry-Pi-FLIR-Lepton-Thermal-Imaging-Camera/refs/heads/main/images/inside.gif" />
      <div>Indoor thermal imaging (person waving hand) close distance, grey background</div>
    </td>
    <td width="33%" valign="top" style="vertical-align: top;">
      <img src="https://raw.githubusercontent.com/sorinbotirla/Raspberry-Pi-FLIR-Lepton-Thermal-Imaging-Camera/refs/heads/main/images/inside_blackbg.gif" />
      <div>Indoor thermal imaging (multiple persons) close distance, black background</div>
    </td>
    <td width="33%" valign="top" style="vertical-align: top;">
      <img src="https://raw.githubusercontent.com/sorinbotirla/Raspberry-Pi-FLIR-Lepton-Thermal-Imaging-Camera/refs/heads/main/images/outside.gif" />
      <div>Outdoor thermal imaging (city buildings, streets and cars) medium-long distance, grey background</div>
    </td>
  </tr>
</table>

## Credits to the Original Authors of the code
This repository uses the raspberry_video from the <a href="https://github.com/groupgets/LeptonModule/tree/master/software/raspberrypi_video" target="_blank">LeptonModule repository - by groupgets</a>

## What does the <a href="https://github.com/sorinbotirla/Raspberry-Pi-FLIR-Lepton-Thermal-Imaging-Camera/blob/main/setupleptonrpi.sh">setupleptonrpi.sh</a> script do?
This script automatically prepares a Raspberry Pi to run a FLIR Lepton thermal camera without a desktop environment. It configures the system to use framebuffer video output instead of HDMI or a window manager. The script enables the required hardware interfaces and installs all needed software dependencies. It downloads the thermal camera software, applies stability fixes, and builds it for the current Raspberry Pi. The display is configured for stable composite video output without boot messages on screen. A background service is installed so the camera starts automatically on every boot. The script can be safely run on a fresh system or on an existing installation. After completion, the device boots directly into a stable thermal camera view.

## Bill of Materials (BOM / Components required)
You will need:
<ul>
  <li>Raspberry Pi (zero, zero 2w, pi2, pi3, pi4, pi5, cm)</li>
  <li>FLIR Lepton module 2.X, 3.X (2.0, 2.5, 3.0, 3.5) <a href="https://www.sparkfun.com/flir-radiometric-lepton-dev-kit-v2.html" target="_blank">Sparkfun</a></li>
  <li>FLIR Lepton Breakout Board (V2 used in this project but others might work too - untested) <a href="https://www.sparkfun.com/flir-lepton-breakout-board-v2.html" target="_blank">Sparkfun</a></li>
  <li>RCA Connector (for composite cvbs video) or HDMI cable</li>
  <li>Wires to solder</li>
  <li>40 Pin connector for Raspberry pi (or solder directly to the pins)</li>
  <li>Connector of your choice for the Breakout board (or solder directly to the pins)</li>
  <li>2A-5A 5V power adapter for the Raspberry Pi of your choice</li>
  <li>A TV or monitor that has either an HDMI or RCA composite video input</li>
</ul>

## Setup
Install the Raspberry Pi imager and choose your Raspberry Pi model. <br />
Choose <strong>Raspberry Pi OS (other) > Raspberry Pi OS (Legacy, 32-Bit) Lite</strong></br>
Set up your WiFi Connection if needed and enable SSH <strong color="red">(You won't get direct console login on the screeen, login will only be available on SSH if you enable it during this process)</strong>.<br />
Write the SDCard<br />
Boot the raspberry Pi and login trough SSH<br />
<br />
Run:<br />
```bash
sudo apt-get update
sudo apt-get install git -y
git clone --depth 1 https://github.com/sorinbotirla/Raspberry-Pi-FLIR-Lepton-Thermal-Imaging-Camera.git
cd Raspberry-Pi-FLIR-Lepton-Thermal-Imaging-Camera
chmod +xr setupleptonrpi.sh
# Choose between one of the following for black or grey background
sudo ./setupleptonrpi.sh --blackbackground
sudo ./setupleptonrpi.sh --greybackground
```
NOTE: The black background parameter is more suitable for detecting heat sources, while the grey background is more suitable for finer details, including more surrounding awareness.

Wait for the script to install everything and it will reboot itself. If you connect everything right, you will see the thermal image video on your screen in about 13-15s after the boot.

## Pinout

<table>
  <thead>
    <tr>
      <th>Lepton Breakout V2 pin</th>
      <th>Raspberry Pi GPIO pin</th>
      <th>RCA Connector</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>VIN</td>
      <td>3.3V</td>
      <td></td>
    </tr>
    <tr>
      <td>GND</td>
      <td>GND</td>
      <td></td>
    </tr>
    <tr>
      <td>CLK</td>
      <td>GPIO11</td>
      <td></td>
    </tr>
    <tr>
      <td>MISO</td>
      <td>GPIO9</td>
      <td></td>
    </tr>
    <tr>
      <td>MOSI</td>
      <td>GPIO10</td>
      <td></td>
    </tr>
    <tr>
      <td>CS</td>
      <td>GPIO8</td>
      <td></td>
    </tr>
    <tr>
      <td>SDA</td>
      <td>GPIO2</td>
      <td></td>
    </tr>
    <tr>
      <td>SCL</td>
      <td>GPIO3</td>
      <td></td>
    </tr>
    <tr>
      <td>VSYNC</td>
      <td>GPIO17</td>
      <td></td>
    </tr>
    <tr>
      <td></td>
      <td>TV OUT</td>
      <td>RCA center pin = composite video signal</td>
    </tr>
    <tr>
      <td></td>
      <td>GND</td>
      <td>RCA shield = ground</td>
    </tr>
  </tbody>
</table>

<br />
<table style="border: 0;">
  <tr>
    <td width="50%" style="vertical-align: top;">
      <img src="https://raw.githubusercontent.com/sorinbotirla/Raspberry-Pi-FLIR-Lepton-Thermal-Imaging-Camera/refs/heads/main/images/lepton-breakout-v2-pins.jpg" />
      LEPTON Breakout V2 Pins
    </td>
    <td width="50%" style="vertical-align: top;">
      <img src="https://raw.githubusercontent.com/sorinbotirla/Raspberry-Pi-FLIR-Lepton-Thermal-Imaging-Camera/refs/heads/main/images/rpi-gpio.png" />
      Raspberry Pi GPIO Pins
    </td>
  </tr>
</table>
<br />
Example of the TV pins which were used in this project (RPI Zero 2W)
<table>
  <tr>
    <td width="70%" valign="top">
      <img src="https://raw.githubusercontent.com/sorinbotirla/Raspberry-Pi-FLIR-Lepton-Thermal-Imaging-Camera/refs/heads/main/images/zero2-pad-diagram.png" />
    </td>
    <td width="30%" valign="top">
      <img src="https://raw.githubusercontent.com/sorinbotirla/Raspberry-Pi-FLIR-Lepton-Thermal-Imaging-Camera/refs/heads/main/images/20260104_061112.jpg" />
    </td>
  </tr>
</table>

## Why Composite video instead of HDMI?

Composite video requires only two wires, which allows it to operate reliably over long distances, often exceeding 10 to 20 meters without significant interference. Using a thin coaxial cable enables even longer runs, typically between 50 and 200 meters. In comparison, HDMI cables are more sensitive to interference and do not perform well over long distances. Composite CVBS connections can directly replace existing CCTV cameras or automotive front and rear camera systems.

## What this project doesn't do?

The most important thing, it doesn't measure the temperature, lol :)

## License

Open source under MIT

## Copyrights

FLIR, Teledyne, Lepton and their logos are trademarks registered and owned by Teledyne FLIR LLC<br />
Raspberry Pi is a trademark of Raspberry Pi Ltd<br />
HDMI (High-Definition Multimedia Interface) is a registered trademark of HDMI Licensing, LLC
