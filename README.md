# esp-swarm-attestation
This repository demonstrates an implementation of the SIMPLE remote attestation and SIMPLE+ swarm attestation protocols for the ESP32-C3 SoC, as described in the paper: [SIMPLE: A Remote Attestation Approach for Resource-constrained IoT devices](https://ieeexplore.ieee.org/abstract/document/9096052). A trusted execution environment is established for isolating the attestation protocols from user code. Although the protocols themselves are fully implemented, currently only the state of an arbitrarily defined memory region can be verified. Verifying the state of the real user code is left as future work.

## Hardware
The prover software was tested on a network of 10 [ESP32-C3-DevKitC-02](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/hw-reference/esp32c3/user-guide-devkitc-02.html) development boards, and should work on any ESP32-C3 development board. The software MAY also work on ESP32-S3 development boards, however this is not tested.

The verifier software is written in Python, so should be compatible with most machines that have Wi-Fi support.

## Setup
The prover software relies on [ESP Privilege Separation](https://github.com/espressif/esp-privilege-separation) and a patched [ESP-IDF](https://github.com/espressif/esp-idf). A fork of ESP-IDF with the patches pre-applied is available at [cdigby/esp-idf-privilege-separation](https://github.com/cdigby/esp-idf-privilege-separation).

ESP Privilege Separation introduces additional build steps to ESP-IDF that fail on Windows-based systems. Building on MacOS is untested. The following instructions have been tested on Ubuntu. 

### Cloning the repository
To simplify setup, ESP Privilige Separation is included as a git submodule. Submodules are not cloned automatically, so the following commands are needed to correctly clone everything:
```
git clone https://github.com/cdigby/esp-swarm-attestation.git
cd esp-swarm-attestation
git submodule update --init --recursive
```

### Configuring the toolchain
First, clone the patched ESP-IDF. A suggested location is `~/esp/esp-idf-privilege-separation`. Use the following command:
```
git clone https://github.com/cdigby/esp-idf-privilege-separation.git
```

Now, the simplest way to setup the toolchain is using the [ESP-IDF VS Code extension](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension).
Open the esp-swarm-attestation repository in VS Code and install the extension. Once the extension is installed, you should be prompted to configure it. If not, press `F1` to open the command palette and use the `ESP-IDF: Configure ESP-IDF extension` command.

You should see the following screen:

![image](https://user-images.githubusercontent.com/20211754/236518616-ac740eb0-b26e-4d3a-9e3c-d96500fd29f3.png)

Note the link to the [ESP-IDF Prerequisites for Linux](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/linux-macos-setup.html#get-started-prerequisites).
For Ubuntu, you can use the following command to install these:
```
sudo apt-get install git wget flex bison gperf python3 python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
```

With the prerequisites installed, click `Express`. You will then be shown the following screen:

![image](https://user-images.githubusercontent.com/20211754/236518771-f64f32b2-e76e-4192-8943-87f0acef41a5.png)

Most settings can be left as their defaults, however `IDF_PATH` MUST be set to the path of the patched ESP-IDF that you cloned previously, as shown above. After verifying that the path is correct, click `Install`, and wait for installation to complete.


## Building and flashing the prover software
With the ESP-IDF extension installed, the bottom bar of VS Code should look like this:

![image](https://user-images.githubusercontent.com/20211754/236518975-4642cc71-20b8-4109-857c-d9f0b480747e.png)

First, set the target by clicking ![image](https://user-images.githubusercontent.com/20211754/236519068-cabe767e-4dea-41cd-9790-f29563f1538e.png). You will be prompted to select a workspace, the target, and the OpenOCD configuration. Select the current workspace, `esp32c3` as the target and `ESP32-C3 chip (via builtin USB-JTAG)` as the OpenOCD configuration.

Click ![image](https://user-images.githubusercontent.com/20211754/236520642-05c5d2b5-b932-4fdc-9042-36ba3f6e47f1.png) to select the flashing method and ensure it is `UART`.

Click ![image](https://user-images.githubusercontent.com/20211754/236520845-79781754-fb92-4271-b2ef-c0286f27b0b8.png) to select the serial port of the device that you want to flash with the software. 

### Build configuration
To configure the build, modify the definitions in `components/sa_config/sa_build_config.h`.

There are two global definitions which you may want to modify. In particular, `NODE_ID` must be unique for each prover node that the software is built for:
| Definition | Description |
| --- | --- |
| `NODE_ID` | Determines which specific prover node to build for |
| `NODE_PASSWORD` | The password for the node's Wi-Fi access point. All nodes use the same password. |

The rest of the definitions are specific to each individual node. Entries for extra nodes can be added as needed.
| Definition | Description |
| --- | --- |
| `NODE_SSID` | The broadcasted Wi-Fi SSID of this node |
| `NODE_PARENT` | The SSID of the node that this node will try and connect to |
| `NODE_FAKE_MEMORY_CONTENTS` | This implementation currently verifies the state of an arbitrary memory region considered the "user app". This is the value to which all addresses in that region are initialised. |

Once the build has been configured, click ![image](https://user-images.githubusercontent.com/20211754/236524044-7c00a2d1-d7e4-4a64-b352-9bdb606335a0.png) to build the software. The first build will likely take a long time, as ESP-IDF has many submodules need to be cloned.

Upon completion of the build, verify that the correct serial port is selected and click ![image](https://user-images.githubusercontent.com/20211754/236524567-2caf56ff-c839-457f-9d3f-0e50e66a35cd.png) to flash the software. Once this has completed, the prover's serial output can be monitored by clicking ![image](https://user-images.githubusercontent.com/20211754/236524801-a781219f-4d73-4451-9e21-e72dfc05ca5b.png).

## Usage
If all provers have been correctly configured, they should automatically connect to each other and form a network when powered on.
To execute the verifier software, Python scripts are provided in the `verifiers` directory.

### Counters
SIMPLE and SIMPLE+ need to store counters between executions. Additionally, SIMPLE+ updates its collection key after each execution. This data is stored in `verifiers/counters`. To reset all stored data, use `python3 reset_counters.py`. Note that provers do not accept attestation requests if their counter is higher than then verifier's counter, so it is likely that all provers will need to be rebooted after resetting the counters.

### SIMPLE
SIMPLE is a remote attestation protocol for verifying the software state of a single prover. It only supports a single valid software state. The steps to execute SIMPLE are as follows:

1. Connect to the Wi-Fi access point of one of the provers using the SSID and password defined in the build configuration.
2. Execute the verifier with `python3 simple_verifier.py NODE_ID VALID_STATE` where `NODE_ID` is the `NODE_ID` of the prover and `VALID_STATE` is the hex value of `NODE_FAKE_MEMORY_CONTENTS` that is considered a valid state. Note that `VALID_STATE` must not be prefixed by `0x`.

Here is an example in which SIMPLE is executed via node 1 with a `VALID_STATE` of `01`, and node 1 is in a valid state:
```
$ python3 simple_verifier.py 1 01
Connecting to node 1 via 192.168.1.1:3333
Executing SIMPLE...
Counter: 5
Sending attestation request...
Waiting for response...
Received report
HMAC verified
Report value is 1: prover is in a valid state
```

### SIMPLE+
SIMPLE+ is a swarm attestation protocol which verifies the software state of the entire network of provers at once. The protocol comprises of two phases:
- The attestation phase, where the software state of the provers is verified, and each prover securely stores its attestation report.
- The collection phase, where the attestation reports of each prover are aggregated and returned to the verifier.

Unlike SIMPLE, SIMPLE+ supports multiple valid software states. The steps to execute SIMPLE+ are as follows:

1. Connect to the Wi-Fi access point of one of the provers using the SSID and password defined in the build configuration.
2. Execute the attestation phase with `python3 simple_plus_verifier_attest.py NODE_ID VALID_STATES` where `NODE_ID` is the `NODE_ID` of the prover and `VALID_STATES` is a sequence of comma-separated hex values which are considered valid states. Note that the hex values must not be prefixed by `0x`.
3. Execute the collection phase with `python3 simple_plus_verifier_collect.py NODE_ID TIMEOUT_MS` where `NODE_ID` is the `NODE_ID` of the prover and `TIMEOUT_MS` is maximum time in milliseconds that each prover will wait to receive attestation reports before timing out.

Here is an example in which SIMPLE+ is executed on a network of 4 provers via node 1, with the `VALID_STATES` being `01,02,03,04`. Nodes 1 and 2 are in valid states, while nodes 3 and 4 are in invalid states:
```
$ python3 simple_plus_verifier_attest.py 1 01,02,03,04
Connecting to node 1 via 192.168.1.1:3333
Executing SIMPLE+ Attestation Phase...
Counter: 5
Sending attestation request...
Updating k_col...
Done!

$ python3 simple_plus_verifier_collect.py 1 5000
Connecting to node 1 via 192.168.1.1:3333
Executing SIMPLE+ Collection Phase...
Sending collection request...
Waiting for ACK...
Received ACK
Waiting for report...
Report contains data for up to 8 nodes
Received report:
[1, 1, 0, 0, 0, 0, 0, 0]
```

The last line of the collection phase output is the aggregated attestation report. Each value in the report represents the software state of a single prover node: a `1` represents a valid software state, and a `0` represents an invalid software state. Nodes are listed in ascending order, so the first value is node 1 and the last value is node 8.

The design of SIMPLE+ is such that reports always represent multiples of 8 nodes (1 byte is sent per 8 nodes), even if the actual number of nodes in the network is not a multiple of 8. Additionally, SIMPLE+ cannot tell the difference between a node that is in an invalid state, a node that didn't respond and a node that does not exist at all. Hence, even though the network size was 4, the output of SIMPLE+ suggests that there are nodes 5, 6, 7 and 8 on the network which are all in invalid states.

A new line will be started for every 8 nodes in the report, also in ascending order. Hence we have nodes 1-8, then nodes 9-16, etc.
