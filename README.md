# Side Mission Firmware

How to load to your Side Mission motion module:
0. Remove the battery from the motion module and plug in via USB
1. Open VSCode
2. Install PlatformIO on VSCode
3. Open this workspace
4. [Change the name of your device in ble.h](https://github.com/Ukaton-Inc/side-missions-firmware/blob/main/src/ble.h#L17)
5. Click the upload button on the bottom (looks like "->")
6. After uploading, unplug the USB and plug it back in
7. Test [here](https://ukaton-side-mission.glitch.me/visualization) to see if it works
8. If it works, add the battery
