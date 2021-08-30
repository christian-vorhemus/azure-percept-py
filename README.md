This is the source code for azure-percept-py - an unofficial Python library to access the sensors of Azure Percept in Python.

## Prerequisites
Make sure the following is installed on your Percept device or the container you want to use
- libalsa and corresponding header files (run `sudo yum install alsa-lib-devel`)
- pyusb (run `sudo yum install pip3` first and then `sudo pip3 install pyusb`)
- pthreads

## Install
The following guide assumes you run the code directly on the Percept board (CBL Mariner OS). You can also create a docker image and install the Python module there, in that case make sure that you mount the host's /dev/bus/usb into the image.
- Clone the source code on your Percept device `git clone https://github.com/christian-vorhemus/azure-percept-py.git`
- Open a terminal and cd into `azure-percept-py`
- Run `sudo pip3 install .`
- To uninstall run `sudo pip3 uninstall azure-percept`

Note that the package brings pre-built libraries that will only run on an aarch64 architecture!

## Azure Ear sample
The following sample authenticates the Azure Ear sensor, records audio for 5 seconds and saves the result locally as a WAV file. Create a new file `earsample.py` in the `azure-percept-py` folder with the following content

```python
from azure.iot.percept import AzureEar
import time

ear = AzureEar()

print("Authenticating sensor...")
while True:
    if ear.is_authenticated() is True:
        break
    else:
        time.sleep(1)

print("Authentication successful!")

print("Recording...")
ear.start_recording("./sample.wav")
time.sleep(5)
ear.stop_recording()
print("Recording stopped")

```

Run `sudo python3 earsample.py` to run the script.
