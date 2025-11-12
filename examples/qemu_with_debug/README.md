# QEMU with Step Debugging

There is nothing special about this project directory that makes it "QEMU-enabled". Since we are using 
the official Espressif image for development in a VSCode dev container, we get it for free.

## Run Project With QEMU

To run your code on an emulated ESP32 in QEMU all you have to do is run the following:

`idf.py qemu monitor`

This will run it on QEMU and enable the normal output monitoring you are used to. That's it!

You can do this from any project folder that uses portions of the IDF supported by QEMU.

## Limitations

Always check the latest documentation from Espressif. QEMU will not emulate the WiFi or Bluetooth 
systems among other things. It's most useful for non-hardware related code testing.

## Step Debug with QEMU

This repo has some global tools setup to allow debugging from the VSCode debugging panel. To step 
debug a project do the following:

1. Open a terminal in VSCode
1. Navigate to the project folder under **examples**
1. Run `idf.py fullclean` to ensure the build folder is clean
1. Run `setchip` to select the target architecture (QEMU only supports `esp32`, `esp32s3` and `esp32c3`)
1. Run `debugthis` to configure the settings.json file for debugging
1. Set a breakpoint in your code
1. Use the built-in VSCode debug panel to start debugging