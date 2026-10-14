# BetterMouseYoke
A plugin that does away with X-Plane's idiotic centered little box for mouse steering and replaces it with a more sane system for those who, for whatever reason, want to or have to use the mouse for flying.

The plugin removes the centered + sign with the squared box around it and instead adds a new command to X-Plane 11 that lets you toggle mouse yoke control on or off. When toggled on, the mouse cursor changes to a <img src="yoke-mode.png?raw=true" style="position:relative; top:5px;"> symbol and you can

* move the mouse right to go right,
* move the mouse left to go left,
* move the mouse toward you to go up,
* and move the mouse away from you to go down.

The absolute mouse position on the screen represents the deviation from center of the yoke, i.e.

* move the mouse to the right edge of the screen to apply full-right yoke,
* move the mouse to the left edge of the screen to apply full-left yoke,
* move the mouse to the bottom edge of the screen to apply full-up yoke,
* and move the mouse to the top edge of the screen to apply full-down yoke.

Additionally, a magenta text indicator is displayed in the upper-left corner of the screen when mouse yoke control is active.

The yoke and rudder cursor symbols are X-Plane's own and require X-Plane 12.3 or newer. On older versions the cursor remains a normal arrow.

<p align="center">
  <a href="image-1.png?raw=true" target="_blank">
    <img title="Mouse Yoke Control" src="image-1.png?raw=true" width="860" height="503"/>
  </a>
</p>

While in mouse yoke mode, press and hold the left mouse button to switch to rudder mode. The cursor will change to a <img src="rudder-mode.png?raw=true"> symbol and the magenta text indicator will read *Mouse Rudder Control*. Two green little bars will appear to the left and to the right of the cursor that indicate the mouse movement range for rudder deflection.

<p align="center">
  <a href="image-2.png?raw=true" target="_blank">
    <img title="Mouse Rudder Control" src="image-2.png?raw=true" width="860" height="503"/>
  </a>
</p>

Release the left mouse button to transition back into mouse yoke mode. The rudder will then gradually return to the neutral position. 

While in mouse yoke mode, press and hold the right mouse button and move the mouse to look around in the 3D cockpit. The yoke and rudder stay where they are and the magenta text indicator will read *Mouse Look*. Release the right mouse button and the cursor returns to where it was, so the yoke continues from the same position.

Mouse clicks don't reach the cockpit while mouse yoke control is enabled, so holding the left mouse button for rudder control won't accidentally flip any switches. The mouse wheel still works as usual. To click cockpit controls, toggle mouse yoke control off first.

In outside views the cursor and the left mouse button work the same as in the cockpit. The right mouse button is left to X-Plane there, so dragging with it moves the camera as usual.

### Download
You can get the latest version [here](https://github.com/smiley22/XPPlugins/releases/tag/BetterMouseYoke).

### Installation
Simply extract the .zip archive into the *plugins* directory of your *X Plane 11* installation, e.g. *X Plane 11\Resources\plugins*.

### Configuration
Once installed, you will find a new command for toggling mouse yoke control on or off in the Keyboard settings.

<p align="center">
  <a href="image-3.png?raw=true" target="_blank">
    <img title="Keyboard command" src="image-3.png?raw=true" width="860" height="503"/>
  </a>
</p>

##### Config File
On startup, the plugin looks for a *settings.ini* file in its' directory, i.e. *"X Plane 11\Resources\plugins\BetterMouseYoke\settings.ini"*. This file allows you to change some of the settings of the plugin. If the file does not exist, the plugin creates one with the default values. Options you toggle in the plugin's menu are saved to this file as well.

Here's the *settings.ini* file the plugin creates, with all the options the plugin allows you to configure. Lines starting with a **;** are comments.

```
; BetterMouseYoke settings. Lines starting with ; are comments.
; Remove a line to use the default value.
[settings]

; When entering mouse yoke mode, move the mouse cursor to match the
; current yoke deflection to avoid sudden control movements.
; Set to 0 to disable.
set_pos = 1

; When entering rudder mode, move the mouse cursor to match the current
; rudder deflection. Set to 0 to disable.
set_rudder_pos = 1

; Change the mouse cursor symbol in yoke and rudder mode (requires
; X-Plane 12.3 or newer). Set to 0 to disable.
change_cursor = 1

; Gradually return the rudder to neutral after leaving rudder mode.
; Set to 0 to disable.
rudder_return = 1

; The speed (in units per second) with which the rudder returns to
; neutral. A value of 2 takes half a second from full deflection.
rudder_return_speed = 2

; The mouse movement range for full rudder deflection in rudder mode,
; i.e. the distance between the two green bars on the screen.
rudder_deflection_distance = 200

; How many degrees the view turns per pixel of mouse movement when
; looking around with the right mouse button.
look_sensitivity = 0.2

; Write extra debug messages to X-Plane's Log.txt. Set to 1 to enable.
debug = 0
```

