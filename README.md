This is the source code for the azure-percept package - an unofficial Python library to access the sensors of Azure Percept in Python.

**IMPORTANT**: This is an experimental privately written libray without any warranty, expect bugs. If you encounter them, please [open an issue](https://github.com/christian-vorhemus/azure-percept-py/issues) on Github.

## Connect to your Percept

Please refer to the official documentation to learn how to connect to the device: https://docs.microsoft.com/en-us/azure/azure-percept/how-to-ssh-into-percept-dk


## Install

### From PyPI
This package is intended to run directly on Azure Percept hardware (or a container hosted on Azure Percept). Make sure you use one of the following Python versions: 3.6, 3.7, 3.8 or 3.9. Then install the package with:

```
sudo yum install python3-pip
python3 -m pip install --upgrade pip
pip3 install azure-percept
```
### From source
Make sure the following is installed on your Percept device or the container you want to use:
- libalsa, libusb, gcc, binutils, Python headers, setuptools and pip (run `sudo yum install -y git alsa-lib-devel libusb-devel gcc glibc-devel kernel-devel kernel-headers binutils python3-devel python3-setuptools python3-pip`)
- pthreads (libpthread should be available on most OS by default, check your library path - for example /usr/lib/ - to be sure)
- Clone the source code on your Percept device `git clone https://github.com/christian-vorhemus/azure-percept-py.git`
- Open a terminal and cd into `azure-percept-py`
- Run `pip3 install .`
- To uninstall run `pip3 uninstall azure-percept`

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

Type `sudo python3 perceptaudio.py` to run the script.

## Azure Percept Vision samples
### Run a machine learning model on the VPU
The following sample shows how to run a model on the Azure Vision Myriad VPU. It assumes we have a .onnx model ready for inference. If not, download a model from the [ONNX Model Zoo](https://github.com/onnx/models), for example [ResNet-18](https://github.com/onnx/models/raw/master/vision/classification/resnet/model/resnet18-v1-7.onnx) or browse the [sample models](https://github.com/microsoft/azure-percept-advanced-development#model-urls) which are already converted to the .blob format. Create a new file `perceptvision.py` with the following content

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

Type `sudo python3 perceptvision.py` to run the script. Especially the model conversion can take several minutes. `vision.start_inference(blob_model_path)` will start the Azure Percept Vision camera as well as the VPU. To specify the input camera sources, pass the `input_src` argument, for example `vision.start_inference(blob_model_path, input_src=["/dev/video0", "/dev/video2"])` whereas `/camera1` would identify the Percept module camera and `/dev/video0`, `/dev/video2` are conventional USB cameras plugged into the Percept DK.  With `vision.get_inference()` the prediction results are returned as an `InferenceResult` object or as a list of `InferenceResult` objects in case of multiple input sources. The prediction is stored as a numpy array in `res.inference`.

### Process local image files on the VPU

It's also possible to use a local image file instead of reading from a camera device. To do so, convert the image into a BGR sequence of bytes and pass them in the `input` argument of `get_inference()`:

```python
from azure.iot.percept import VisionDevice, InferenceResult
import time
from PIL import Image
import numpy as np

vision = VisionDevice()

print("Authenticating sensor...")
while True:
    if vision.is_ready() is True:
        break
    else:
        time.sleep(1)

print("Authentication successful!")

image = Image.open("./<yourfile>.jpg")
image_np = np.array(image)
image_np = np.moveaxis(image_np, -1, 0)
r = image_np[0].tobytes()
g = image_np[1].tobytes()
b = image_np[2].tobytes()

img = b+g+r

vision.start_inference("<model>.blob")
res: InferenceResult = vision.get_inference(input=img, input_shape=(image.height, image.width))
print(res.inference)
vision.stop_inference()
vision.close()
```

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

## Troubleshooting

#### During model conversion I get "Cannot create X layer from unsupported opset"
This indicates that the model contains a layer that can't be converted to a model definition the VPU can process. For a list of supported layers see [here](https://docs.openvino.ai/latest/openvino_docs_MO_DG_prepare_model_Supported_Frameworks_Layers.html).

#### Reading audio data fails with "ValueError: Device not found" or "Exception: Azure Ear could not authenticate"
Type in "lsusb". You should see a list of several devices, try to find `Bus 003 Device 006: ID 045e:0673 Microsoft Corp.`. If this device is not present, unplug and plug in your Azure Audio device again and restart the device. Additionally make sure your device has Internet connectivity during the authentication process.

## License
This library is licensed under [Apache License Version 2.0](https://github.com/christian-vorhemus/azure-percept-py/blob/main/LICENSE) and uses binaries and scripts from the [OpenVINO toolkit](https://github.com/openvinotoolkit/openvino) which is as well licensed under Apache License Version 2.0. 
