# MultiMon GUI

## Overview

MultiMon is designed to monitor up to four printers which are driven by either [OctoPrint](http://octoprint.org) or [Duet3D RepRap software](https://duet3d.dozuki.com/Wiki/Firmware_Overview). The monitor has both a GUI and a Web UI displayed on a color touch screen. The Web UI is primarily for configuration.

The GUI is organized around the concept of "Screens" of information that are navigated using touches. Touches can either be a press or a long press and typically result in moving to another screen. they may also cause an action to occur, but remain on the current screen.

There are a number of built-in screens (see below), but users can also define plugins which have their own screens. Plugins are accessed by tapping in the main area of the [Home Screen](#time-screen). This will take you to the first plugin. Tapping on a plugin screen takes you to the next plugin, or back to the Home Screen if there are no more. Plugin screens may also be accessed directly from the [Utility Screen](#utility-screen).

MultiMon regularly calls out to the printers to get their status. If a printer is offline, MultiMon checks it every 5-10 minutes to see if it has come back online. If a printer is online, but not printing, MultiMon checks it every 1-3 minutes to see if it has changed. If a printer is actually in the process of printing, MultiMon updates its status every 30 seconds. This is [configurable](../README.md#configure-printers). You can force a refresh sooner using either the Web UI or the [GUI](#utility-screen). You can also change the interval using the Web UI.

These calls can take a second or two depending  on how many printers are being checked and how responsive they are. During these periods the GUI is not responsive. To let the user know when this is happening, the GUI overlays a small info icon in the upper right hand corner of the screen. It is automatically removed when the calls are complete. When the info icon has a green border it means that MultiMon is talking to a printer. When it has an orange border it means that MultiMon is getting weather information.

*Note: The screen shots below are image captures from the device.*

## Screens

Index of Screens (alphabetical):

* [Calibration Screen](#calibration-screen)
* [Config Screen](#config-screen)
* [Utility Screen](#utility-screen)
* [Forecast Screen](#forecast-screen)
* [Printer Status Screen](#printer-status-screen)
* [Printer Detail Screen](#printer-detail-screen)
* [Reboot Screen](#reboot-screen)
* [Splash Screen](#splash-screen)
* [Time Screen (aka Home Screen)](#time-screen)
* [Weather Screen](#weather-screen)
* [Wifi Screen](#wifi-screen)

<a name="wifi-screen"></a>
### WiFi Screen

When MultiMon boots, the first thing it displays is the WiFi Screen. This will remain on the screen until a WiFi connection is established. Once the unit is connected to your WiFi network, the display will automatically navigate to the [Splash Screen](#splash-screen).

![](images/ss/WiFiScreen.png)

**Actions**:

* If MultiMon cannot connect to your WiFi network (e.g. it is new and hasn't been configured yet), it will automatically navigate to the [Config Screen](#config-screen).
* There are no actions available to the user.

<a name="splash-screen"></a>
### Splash Screen

The Splash Screen is displayed during the boot process while MultiMon is initializing. You will notice that the Info Icon is displayed almost the whole time since MultiMon is getting initial status from all configured printers, getting the current weather, and getting the forecast.

In the screen shots below you will see the info icon displayed in the upper right corner. A green border indicates that it is receiving printer status, while an orange border means it is getting weather data.

![](images/ss/SplashScreen_IIP.png)
![](images/ss/SplashScreen_IIW.png)

**Actions**:

* There are no actions available to the user. Once initialization is complete, MultiMon will automatically display the [Home Screen](#home-screen).

<a name="home-screen"></a>
<a name="time-screen"></a>
### Time Screen (aka Home Screen)

The time screen is the the primary/home screen. It provides a clock, overview status of the printers, and a single line of weather information. The screen shots below capture the home screen in a variety of situations. The elements of the scree are as follows:

* The current temperature and weather description for the selected city
* The next line shows the expected completion time of the next print across all configured printers. If there is no print in progress, then that line will not be displayed. If the print is scheduled to complete within 15 minutes, it will be displayed in a highlight color. Otherwise it will be displayed as normal text.
* The largest area shows the time. The format (24 hour or AM/PM) is configured in the Web UI.
* Across the bottom of the home screen are status indicators for each printer. To conserve space, this screen shows the nicknames in a very small font. My experience has been that after a short time, you know which printer is which by the location, so the small font isn't a real issue. The printers are displayed in the order in which they were configured in the Web UI. Each status indicator will show the current state which can be:
	* **Offline**: No connection to the printer
	* **Online**: Connected to the printer, but no print in progress
	* **Printing**: In which case the area displays a progress bar with percent complete
	* **Unused**: Meaning that printer has not been configured

![](images/ss/HomeScreen_3Prints.png)
![](images/ss/HomeScreen_Mixed.png)
![](images/ss/HomeScreen_UIW.png)
![](images/ss/HomeScreen_UIP.png)
![](images/ss/HomeScreen_Unused.png)

**Actions**:

* Pressing anywhere within the weather area navigates to the Weather Screen. Note that since this area is not very tall, the actual sensitivity area extends below the weather line to make it easier to press with a finger (rather than a stylus).
* Pressing in the clock area navigates to the [Printer Status Screen](#printer-status-screen).
* Pressing any of the printer status areas navigates to a detail screen for that printer.
* A long press anywhere on the screen navigates to the Utility Screen.

<a name="printer-status-screen"></a>
### Printer Status Screen

The Printer Status Screen gives an overview of the status of each printer. It is very similar to what is shown on the [Time Screen](#time-screen), but also shows each printer's time remaining and completion time (if a print is active). Below is an example of what this screen looks like:

![](images/ss/OverviewScreen.png)

This screen is actually a plugin that comes pre-configured with *MultiMon*. It can be removed or modified following the approach given in the [Plugin Guide](PluginGuide.md).

**Actions**:

* Pressing anywhere on the screen will move to the next plugin, or back to the [Home Screen](#home-screen) if there are no more plugins.

<a name="printer-detail-screen"></a>
### Printer Detail Screen

The Printer Detail Screen provides more detail about a print in progress. This screen is only available for printers that are either printing or have completed a print. The layout of the screen is:

* The nickname of the printer. If no nickname was specified, the server name will be used.
* The name of the file being printed. If the name won't fit on a single line, it will be truncated, [but can be scrolled](#scroll-filename). If it is shorter than a single line, it will be centered. 
* A progress bar shows the percent completion of the print and the time remaining in HH:MM:SS format.
* The line below the progress bar gives the bed actual and target temperatures as well as the target and actual temperature for tool 1 (labeled E0 for extruder 0). Each pair is given as Actual / Target (e.g. 60.1 / 60.0). These values are always in degrees Celsius.
* The next line shows the elapsed time of the print (labeled as "Done:") and the estimated completion time (labeled "Est:"). When a print is complete, this will show "Complete" rather than an estimate.

![](images/ss/DetailScreen_InProgress.png)
![](images/ss/DetailScreen_100Pct.png)

**Actions**:

* <a name="scroll-filename"></a> If the file name is too long to fit on a single line, it is truncated. To see the whole name, tap anywhere in the nickname area or the filename area (i.e. anywhere near the top of the screen) and the name will scroll to reveal the entire content. If you tap the area again while it is scrolling, the scrolling will stop and the name will be displayed from the beginning.
* Pressing anywhere on the screen navigates back to the [Home Screen](#home-screen).
* If the print is complete (100%), it will continue to show as 100%, here and on the home screen, until an action is taken.
	* Sometimes it is preferable to show the printer as Online and ready for a new print. I sometimes use this distinction to remind myself whether I have already collected the last print so the printer is ready for a new job.
	* To get into this state, long press anywhere on the screen. That will navigate to the [Home Screen](#home-screen) and set the print to show as Online rather than 100% complete.
	* The screen shots below show the home screen showing "100%" (as it would before a long press here) and "Online" as it would after a long press here.

![](images/ss/PD_HomeScreen_100Pct.png)
![](images/ss/PD_HomeScreen_Online.png)

<a name="weather-screen"></a>
### Weather Screen

The weather screen shows current weather information form [OpenWeatherMap.org](http://OpenWeatherMap.org) for the city that was configured using the Web UI. In particular it shows:

* The name of the City in the upper left corner
* The current time in the upper right corner
* An icon giving a visual representation of the current weather conditions
* The current temperature 
* An icon giving a feel for wind speed (none, light, heavy)
* A textual description of the current weather condition
* The humidity, barometric pressure, visibility, and "real feel" temperature.

The units (metric or imperial) are configured in the Web UI.

![](images/ss/WeatherScreen_II.png)  
*Note*: This shot shows the info icon in the upper right corner. It is occluding the seconds area of the time display.

**Actions**:

* Pressing anywhere on the screen navigates to the [Forecast](#forecast-screen).
* A long press anywhere on the screen navigates back to the [Home Screen](#home-screen).

<a name="forecast-screen"></a>
### Forecast Screen

The Forecast Screen uses [OpenWeatherMap.org](http://OpenWeatherMap.org) to get the 5-day forecast for the city that was configured using the Web UI. The screen is arranged as two columns with 3 forecasts each for a total of six cells. They are ordered chronologically from top to bottom, left to right. We use the first cell for the current temperature. The rest of the cells are the 5-day forecast.

Each cell shows an icon representing the weather condition (e.g. Sunny or Rain), the low/high temperature for the day, and the day/hour when the high will occur. Obviously the weather conditions can change throughout the day but there is only space to display one weather condition icon. To accommodate this I display the weather condition icon corresponding to the time of the high temperature. The two screenshots below show how the screen looks using [24-hour time or AM/PM](../README.md#configure-display) format.

Note: Since the first entry corresponds to the current conditions, it only shows one temperature (not high and low). The hour displayed is the hour when the reading was taken.

![](images/ss/ForecastScreen24hr.png)
![](images/ss/ForecastScreenAMPM.png)

**Actions**:

* Pressing anywhere on the screen navigates back to the [Home Screen](#home-screen).
* A long press anywhere on the screen navigates back to the [Weather Screen](#weather-screen).

<a name="utility-screen"></a>
### Utility Screen

The Utility Screen provides access to [plugins](), if any, as well as other commands and system information. The layout of the screen is:

* The header line just shows the the MultiMon version number
* Below that is the name of the monitor and its current IP address. If you use a computer/phone/tablet that has Bonjour support, you can access MultiMon by typing the unit's name followed by ".local" into the address bar of your browser. In the example below, the name of this unit is "QuadMon" so you could access it by entering `http://QuadMon` into your browser.
* To the right of those two lines is a WiFi signal strength meter (0 to 4 bars).
* Next is a grid of buttons that give access to up to 4 configured [plugins]().  In this example there are two plugins ("Blynk Weather" and "Custom Layout"). The other two slots are not in use.
* The next row of buttons allows you to change the brightness of the screen to Dim, Medium, or Bright. The brightness can also be changed from the [Web UI interactively](../README.md#home-page) or via a [schedule](../README.md#configure-display).
* The next row of buttons are:
  * Refresh: Pressing it will cause MultiMon to ask each printer for fresh status information. The Info Icon will flash on the screen while that is happening. 
  * Calibrate:This button takes you to the [Calibration Screen](#calibration-screen) where the touch screen can be calibrated.
  * Home: Navigates back to the home screen.

![](images/ss/UtilityScreen.png)

**Actions**:

* Pressing any of the buttons causes the actions described above (refreshing printer data or setting the brightness)

<a name="config-screen"></a>
### Config Screen

This screen is only displayed when MultiMon can't connect to the WiFi network. This can happen for a new installation, or if the unit is moved to a new location with a different WiFi base station, or if the name or credentials for your base station have changed. In any of these circumstances, MultiMon will become a WiFi base station and display the Config Screen which will show its name (SSID). At that point you need to use your phone, tablet, or computer to connect to this WiFi base station and then select your normal WiFi base station and enter your password.

In the image shown below, you would connect your phone/tablet/computer to the WiFi station named "MM-a82607". The rest of the configuration happens on your phone/tablet/computer. Once the MultiMon unit is properly configured, it will reboot and connect to the network you specified.

![](images/ss/ConfigScreen.png)

**Actions**:

* There are no additional actions available to the user from this screen.

<a name="calibration-screen"></a>
### Calibration Screen

The Calibration screen allows you to calibrate the touch hardware built into the display. You need never use this functionality unless you are either finding touch to be inaccurate *or* you have flipped the orientation of the display using the Web UI.

![](images/ss/CalibrationProcess.png)  

**Actions**:
You will be presented with a screen that asks you to touch the screen to begin the calibration process. Once you do, you will be presented with a series of four screens. Each will display an arrow pointing to one corner of the display and asking you to touch that corner. Do so. Once you've done that, it will ask you to touch the screen again to complete the calibration process. At that point you will be taken back to the [Home Screen](#home-screen).

<a name="reboot-screen"></a>
### Reboot Screen

An advanced user of the Web UI can request a reboot of the device. When they do so, the Reboot Screen will appear regardless of the current state of the GUI.

![](images/ss/RebootScreen.png)

**Actions**:

* A long press of the `Reboot` button will cause the device to reboot. A normal press will be ignored.
* Pressing the `Cancel` button will navigate back to the [Home Screen](#home-screen). In a future release, pressing `Cancel` may go back to the previously displayed screen rather than the Time Screen.
* If you take no action for one minute, it is the same as pressing the `Cancel` button.