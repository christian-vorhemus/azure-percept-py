This is the source code for azure-percept-py - an unofficial Python library to access the sensors of Azure Percept in Python.

**IMPORTANT**: This is an experimental libray, expect bugs. If you encounter them, please [open an issue](https://github.com/christian-vorhemus/azure-percept-py/issues) on Github.

## Connect to your Percept

Please refer to the official documentation to learn how to connect to the device: https://docs.microsoft.com/en-us/azure/azure-percept/how-to-ssh-into-percept-dk

## Prerequisites
Make sure the following is installed on your Percept device or the container you want to use
- libalsa, libusb, gcc, binutils, Python headers, setuptools and pip (run `sudo yum install -y alsa-lib-devel libusb-devel gcc glibc-devel kernel-devel kernel-headers binutils python3-devel python3-setuptools python3-pip`)
- pthreads (libpthread should be available on most OS by default, check your library path - for example /usr/lib/ - to be sure)
- Numpy: `sudo pip3 install numpy`

## Install
The following guide assumes you run the code directly on the Percept board (CBL Mariner OS). You can also create a docker image and install the Python module there, in that case make sure that you mount the host's /dev/bus/usb into the image.
- Clone the source code on your Percept device `git clone https://github.com/christian-vorhemus/azure-percept-py.git` (if you're accessing the repo from Azure Devops use `git clone https://chrysalis-innersource@dev.azure.com/chrysalis-innersource/Azure%20Percept%20Python%20library/_git/Azure%20Percept%20Python%20library`)
- Open a terminal and cd into `azure-percept-py`
- Run `sudo pip3 install .`
- To uninstall run `sudo pip3 uninstall azure-percept`

Note that the package includes pre-built libraries that will only run on an aarch64 architecture!

## Azure Percept Audio sample
The following sample authenticates the Azure Percept Audio sensor, records audio for 5 seconds and saves the result locally as a WAV file. Create a new file `perceptaudio.py` with the following content

```python
from azure.iot.percept import AudioDevice
import time

audio = AudioDevice()

print("Authenticating sensor...")
while True:
    if audio.is_ready() is True:
        break
    else:
        time.sleep(1)

print("Authentication successful!")

print("Recording...")
audio.start_recording("./sample.wav")
time.sleep(5)
audio.stop_recording()
print("Recording stopped")
audio.close()
```

Run `sudo python3 perceptaudio.py` to run the script.

## Azure Percept Vision samples
### Run a machine learning model on the VPU
The following sample shows how you can run a model on the Azure Vision Myriad VPU. It assumes we have a .onnx model ready for inference. If not, download a model from the [ONNX Model Zoo](https://github.com/onnx/models), for example [ResNet-18](https://github.com/onnx/models/raw/master/vision/classification/resnet/model/resnet18-v1-7.onnx). Create a new file `perceptvision.py` with the following content

```python
from azure.iot.percept import VisionDevice, InferenceResult
import time
import numpy

vision = VisionDevice()

print("Authenticating sensor...")
while True:
    if vision.is_ready() is True:
        break
    else:
        time.sleep(1)

print("Authentication successful!")

# this will convert a ONNX model to a model file with the same name
# and a .blob suffix to the output directory "/path/to"
vision.convert_model("/path/to/resnet18-v1-7.onnx", output_dir="/path/to")
vision.start_inference("/path/to/resnet18-v1-7.blob")
res: InferenceResult = vision.get_inference(return_image=True)
print(res.inference)
print(res.image)
vision.stop_inference()
vision.close()
```

Run `sudo python3 perceptvision.py` to run the script. Especially the model conversion can take several minutes. `vision.start_inference(blob_model_path)` will start the Azure Percept Vision Camera and those images are used as an input for `model`. To specify the input sources, pass the input_src argument, for example `vision.start_inference(blob_model_path, input_src=["/camera1", "/dev/video0"])` whereas `/camera1` identifies the Percept camera and `/dev/video0` is a conventional USB camera.  With `vision.get_inference()` the prediction results are returned, `res.inference` is a numpy array.

### Take a picture and save it locally
The following sample gets an image (as a numpy array) from the Azure Percept Vision device in BGR format with shape (height, width, channels) and saves it as a JPG file (you need Pillow for this sample to work: `pip3 install Pillow`)

```python
from azure.iot.percept import VisionDevice
import time
import numpy as np
from PIL import Image

vision = VisionDevice()

print("Authenticating sensor...")
while True:
    if vision.is_ready() is True:
        break
    else:
        time.sleep(1)

print("Authentication successful!")

img = vision.get_frame() # get a camera frame from the Azure Vision device
img = img[...,::-1].copy() # copy the BGR image as RGB
pil_img = Image.fromarray(img) # convert the numpy array to a Pillow image
pil_img.save("frame.jpg")
vision.close()
```
### Record a video
The following sample records a video for 5 seconds and saves it locally as a MP4 file.

```python
from azure.iot.percept import VisionDevice
import time

vision = VisionDevice()

print("Authenticating sensor...")
while True:
    if vision.is_ready() is True:
        break
    else:
        time.sleep(1)

print("Authentication successful!")

print("Recording...")
vision.start_recording("./sample.mp4")
time.sleep(5)
vision.stop_recording()
print("Recording stopped")
vision.close()
```
