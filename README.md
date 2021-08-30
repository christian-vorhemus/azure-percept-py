This is the source code for azure-percept-py - an unofficial Python library to access the sensors of Azure Percept in Python.

## Connect to your Percept

Please refer to the official documentation to learn how to connect to the device: https://docs.microsoft.com/en-us/azure/azure-percept/how-to-ssh-into-percept-dk

## Prerequisites
Make sure the following is installed on your Percept device or the container you want to use
- libalsa and corresponding header files (run `sudo yum install alsa-lib-devel`)
- pyusb (run `sudo yum install libusb-devel`)
- pthreads
- Optional: Install pip in case you need additional Python packages: `sudo yum install pip3`

## Install
The following guide assumes you run the code directly on the Percept board (CBL Mariner OS). You can also create a docker image and install the Python module there, in that case make sure that you mount the host's /dev/bus/usb into the image.
- Clone the source code on your Percept device `git clone https://github.com/christian-vorhemus/azure-percept-py.git`
- Open a terminal and cd into `azure-percept-py`
- Run `sudo pip3 install .`
- To uninstall run `sudo pip3 uninstall azure-percept`

Note that the package includes pre-built libraries that will only run on an aarch64 architecture!

## Azure Ear sample
The following sample authenticates the Azure Ear sensor, records audio for 5 seconds and saves the result locally as a WAV file. Create a new file `earsample.py` with the following content

```python
from azure.iot.percept import AzureEar
import time

ear = AzureEar()

print("Authenticating sensor...")
while True:
    if ear.is_ready() is True:
        break
    else:
        time.sleep(1)

print("Authentication successful!")

print("Recording...")
ear.start_recording("./sample.wav")
time.sleep(5)
ear.stop_recording()
print("Recording stopped")
ear.close()
```

Run `sudo python3 earsample.py` to run the script.

## Azure Eye sample
The following sample shows how you can run a model on the Azure Eye Myriad VPU. It assumes we have a .onnx model ready for inference. If not, download a model from the [ONNX Model Zoo](https://github.com/onnx/models), for example [Mobilenet](https://github.com/onnx/models/raw/master/vision/classification/mobilenet/model/mobilenetv2-7.onnx). Create a new file `eyesample.py` with the following content

```python
from azure.iot.percept import AzureEye
import time
import numpy

eye = AzureEye()

print("Authenticating sensor...")
while True:
    if eye.is_ready() is True:
        break
    else:
        time.sleep(1)

print("Authentication successful!")

# this will convert a ONNX model to a model file with the same name 
# and a .blob suffix to the output directory "/home/admin"
eye.convert_model("/home/admin/mobilenetv2-7.onnx", "/home/admin") 
eye.start_inference("/home/admin/mobilenetv2-7.blob")
arr = eye.get_inference() # arr is a numpy array that contains the model output
print(arr.shape)
eye.stop_inference()
eye.close()
```

Run `sudo python3 eyesample.py` to run the script. Especially the model conversion can take several minutes. `eye.start_inference(model)` will start the Azure Eye Camera and those images are used as an input for `model`. Then `eye.get_inference()` is used to get prediction results as numpy vectors from the device.

The following sample gets an image (as a numpy array) from the Azure Eye device in BGR format with shape (height, width, channels) and saves it as a JPG file.

```python
from azure.iot.percept import AzureEye
import time
import numpy
from PIL import Image

eye = AzureEye()

print("Authenticating sensor...")
while True:
    if eye.is_ready() is True:
        break
    else:
        time.sleep(1)

print("Authentication successful!")

img = eye.get_frame() # can take several seconds
img = img[...,::-1].copy()
pil_img = Image.fromarray(img)
pil_img.save("frame.jpg")
eye.close()
```

