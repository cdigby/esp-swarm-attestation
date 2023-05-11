# esp-swarm-attestation
This repository demonstrates an implementation of the SIMPLE remote attestation and SIMPLE+ swarm attestation protocols for the ESP32-C3 SoC, as described in the paper: [SIMPLE: A Remote Attestation Approach for Resource-constrained IoT devices](https://ieeexplore.ieee.org/abstract/document/9096052). A trusted execution environment is established for isolating the attestation protocols from user code. Although the protocols themselves are fully implemented, currently only the state of an arbitrarily defined memory region can be verified. Verifying the state of the real user code is left as future work.


## Hardware
The prover software was tested on a network of 10 [ESP32-C3-DevKitC-02](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/hw-reference/esp32c3/user-guide-devkitc-02.html) development boards, and should work on any ESP32-C3 development board. The software MAY also work on ESP32-S3 development boards, however this is not tested.

The verifier software is written in Python, so should be compatible with most machines that have Wi-Fi support.


## Setup
The prover software relies on [ESP Privilege Separation](https://github.com/espressif/esp-privilege-separation) and a patched [ESP-IDF](https://github.com/espressif/esp-idf). A fork of ESP-IDF with the patches pre-applied is available at [cdigby/esp-idf-privilege-separation](https://github.com/cdigby/esp-idf-privilege-separation).

ESP Privilege Separation introduces additional build steps to ESP-IDF that fail on Windows-based systems. Building on MacOS is untested. The following instructions have been tested on Ubuntu. 

### Cloning the repository
To simplify setup, the patched ESP-IDF and ESP Privilige Separation are included as git submodules. Submodules are not cloned automatically, so the following commands are required to correctly clone everything:
```
$ git clone https://github.com/cdigby/esp-swarm-attestation.git
$ cd esp-swarm-attestation
$ git submodule update --init --recursive
```
Since ESP-IDF is large and contains many submodules, this may take a while.

### Installing the toolchain
First, ensure that the [ESP-IDF Prerequisites for Linux](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/linux-macos-setup.html#get-started-prerequisites) are installed. On Ubuntu, these can be installed with the following command:
```
$ sudo apt-get install git wget flex bison gperf python3 python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
```

Next, install ESP-IDF:
```
$ esp-idf-privilege-separation/install.sh
```
This will install the toolchain to `$HOME/.espressif`.


## Building and flashing the prover software
The prover software is contained in the `prover` directory. To configure the build, modify the definitions in `components/sa_config/sa_build_config.h`.

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

Once the build has been configured, use the following commands to build and flash the software:
```
$ source esp-idf-privilege-separation/export.sh
$ cd prover
$ idf.py set-target esp32c3
$ idf.py build
$ idf.py -p <PORT> flash
```
Replace `<PORT>` with the serial port of your prover e.g. `/dev/ttyUSB0`

Two things to note:
1. `source esp-idf-privilege-separation/export.sh` configures the current terminal to build ESP-IDF projects. If the terminal is closed, `export.sh` will need to be sourced again.
2. `idf.py set-target esp32c3` only needs to be used when building the prover for the first time.

Once the software has been flashed, the prover's serial output can be monitored with the following command:
```
$ idf.py -p <PORT> monitor
```
To exit the monitor, press `Ctrl+T` followed by `Ctrl+X`. Also useful is `Ctrl+T` followed by `Ctrl+R`, which reboots the prover.


## Usage
If all provers have been correctly configured, they should automatically connect to each other and form a network when powered on.
To execute the verifier software, Python scripts are provided in the `verifier` directory.

### Counters
SIMPLE and SIMPLE+ need to store counters between executions. Additionally, SIMPLE+ updates its collection key after each execution. This data is stored in `verifier/counters`. To reset all stored data, use `python3 reset_counters.py`. Note that provers do not accept attestation requests if their counter is higher than then verifier's counter, so it is likely that all provers will need to be rebooted after resetting the counters.

### SIMPLE
SIMPLE is a remote attestation protocol for verifying the software state of a single prover. It only supports a single valid software state. The steps to execute SIMPLE are as follows:

1. Connect to the Wi-Fi access point of one of the provers using the SSID and password defined in the build configuration.
2. Ensure that your current working directory is `verifier`, or the verifier will fail.
3. Execute the verifier with `python3 simple_verifier.py <NODE_ID> <VALID_STATE>` where `<NODE_ID>` is the `NODE_ID` of the prover and `<VALID_STATE>` is the hex value of `NODE_FAKE_MEMORY_CONTENTS` that is considered a valid state. Note that the hex value must not be prefixed by `0x`.

Here is an example in which SIMPLE is executed via node 1 with a valid state of `01`, and node 1 is in a valid state:
```
$ cd verifier
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
2. Ensure that your current working directory is `verifier`, or the verifier will fail.
3. Execute the attestation phase with `python3 simple_plus_verifier_attest.py <NODE_ID> <VALID_STATES>` where `<NODE_ID>` is the `NODE_ID` of the prover and `<VALID_STATES>` is a sequence of comma-separated hex values which are considered valid states. Note that the hex values must not be prefixed by `0x`.
4. Execute the collection phase with `python3 simple_plus_verifier_collect.py <NODE_ID> <TIMEOUT_MS>` where `<NODE_ID>` is the `NODE_ID` of the prover and `<TIMEOUT_MS>` is maximum time in milliseconds that each prover will wait to receive attestation reports before timing out.

Here is an example in which SIMPLE+ is executed on a network of 4 provers via node 1, with the valid states being `01,02,03,04`. Nodes 1 and 2 are in valid states, while nodes 3 and 4 are in invalid states:
```
$ cd verifier
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
ACK HMAC verified
Waiting for report...
Report contains data for up to 8 nodes
Report HMAC verified
Received report:
[1, 1, 0, 0, 0, 0, 0, 0]
```

The last line of the collection phase output is the aggregated attestation report. Each value in the report represents the software state of a single prover node: a `1` represents a valid software state, and a `0` represents an invalid software state. Nodes are listed in ascending order, so the first value is node 1 and the last value is node 8.

The design of SIMPLE+ is such that reports always represent multiples of 8 nodes (1 byte is sent per 8 nodes), even if the actual number of nodes in the network is not a multiple of 8. Additionally, SIMPLE+ cannot tell the difference between a node that is in an invalid state, a node that didn't respond and a node that does not exist at all. Hence, even though the network size was 4, the output of SIMPLE+ suggests that there are nodes 5, 6, 7 and 8 on the network which are all in invalid states.

A new line will be started for every 8 nodes in the report, also in ascending order. Hence we have nodes 1-8, then nodes 9-16, etc.
