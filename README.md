# Raspberry Pi FLIR Lepton Thermal Imaging Camera
A Thermal Imaging camera built on Raspberry Pi and FLIR Lepton module

<table>
  <tr style="border: 0;">
    <td width="50%" valign="top" style="vertical-align: top;">
      <img src="https://raw.githubusercontent.com/sorinbotirla/Raspberry-Pi-FLIR-Lepton-Thermal-Imaging-Camera/refs/heads/main/images/inside.gif" />
      <div>Indoor thermal imaging (person waving hand) close distance</div>
    </td>
    <td width="50%" valign="top" style="vertical-align: top;">
      <img src="https://raw.githubusercontent.com/sorinbotirla/Raspberry-Pi-FLIR-Lepton-Thermal-Imaging-Camera/refs/heads/main/images/outside.gif" />
      <div>Outdoor thermal imaging (city buildings, streets and cars) medium-long distance</div>
    </td>
  </tr>
</table>


## Bill of Materials (BOM / Components required)
You will need:
<ul>
  <li>Raspberry Pi (zero, zero 2w, pi2, pi3, pi4, pi5, cm)</li>
  <li>FLIR Lepton module 2.X, 3.X (2.0, 2.5, 3.0, 3.5)</li>
  <li>FLIR Lepton Breakout Board (V2 used in this project but others might work too - untested)</li>
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
Upload the <strong>setupleptonrpi.sh</strong> script in <strong>/home/youruser/</strong><br />
<br />
Run:<br />
```bash
chmod +xr setupleptonrpi.sh
sudo ./setupleptonrpi.sh
```
Wait for the script to install everything and it will reboot itself. If you connect everything right, you will see the thermal image video on your screen in about 13-15s after the boot.

## Pinout

<table>
  <thead>
    <tr>
      <th>Raspberry Pi pin (physical / signal)</th>
      <th>Lepton Breakout V2 pin</th>
      <th>RCA Connector</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>3.3V</td>
      <td>VIN (3.3V)</td>
      <td></td>
    </tr>
    <tr>
      <td>GND</td>
      <td>GND</td>
      <td></td>
    </tr>
    <tr>
      <td>GPIO11</td>
      <td>CLK (SCK)</td>
      <td></td>
    </tr>
    <tr>
      <td>GPIO9</td>
      <td>MISO (DO)</td>
      <td></td>
    </tr>
    <tr>
      <td>GPIO10</td>
      <td>MOSI (DI)</td>
      <td></td>
    </tr>
    <tr>
      <td>GPIO8</td>
      <td>CS</td>
      <td></td>
    </tr>
    <tr>
      <td>GPIO2</td>
      <td>SDA</td>
      <td></td>
    </tr>
    <tr>
      <td>GPIO3</td>
      <td>SCL</td>
      <td></td>
    </tr>
    <tr>
      <td>GPIO17</td>
      <td>VSYNC</td>
      <td></td>
    </tr>
    <tr>
      <td>TV OUT</td>
      <td></td>
      <td>RCA center pin = composite video signal</td>
    </tr>
    <tr>
      <td>GND</td>
      <td></td>
      <td>RCA shield = ground</td>
    </tr>
  </tbody>
</table>

