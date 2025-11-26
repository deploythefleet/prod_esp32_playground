# Simple Deploy the Fleet Example

This example shows a trivial example of integrating with the Deploy the Fleet OTA service.

## Setup

### Configure WiFi Credentials
Most of the setup is handled for you. Since the `sdkconfig` file is stored in the **build** directory, you need 
to do a build and then launch `idf.py menuconfig` to set your WiFi credentials which can be found under 
_Component config->Production ESP32 Playground->WiFi_.

### Set Deploy the Fleet Product ID
This is located in the **main.cpp** for this example as the define `DTF_PRODUCT_ID` near the top of the file. Set 
this value to the product ID found in the Deploy the Fleet Management Console and be sure to uncomment the line.