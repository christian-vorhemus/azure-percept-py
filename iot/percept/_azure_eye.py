import time
import sys
import argparse
#from _device_authentication import DeviceAuthentication
from ._device_authentication import DeviceAuthentication
import threading
import io
import os
import uuid
from os import path
import subprocess
import usb.core
import usb.util
#from _azure_ear import AzurePercept
from ._azure_ear import AzurePercept
import _azureeye
import numpy as np


class InferenceResult:
    def __init__(self, device_name, inference, image, dimension):
        self.device = device_name
        self.inference = inference
        self.image = image
        self.dimension = dimension


class VisionDevice(AzurePercept):
    """The VisionDevice class initiates the Azure Eye device. It can be used to get camera data or run model inference jobs
    :param DeviceAuthentication authenticator:
        The authenticator object used for SoM authentication. Can be used to customize the VID/PID of the
        USB device as well as the authentication URI used.
    :param int timeout_seconds:
        The maximum time the sensor gets for authentication before abortion
    """

    def __init__(self, authenticator: DeviceAuthentication = None, timeout_seconds: int = 100):
        self._ready = False
        self._is_recording = False
        self._inference_running = False
        if authenticator is None:
            authenticator = DeviceAuthentication(0x045e, 0x066F)
        if self.is_authenticated() == False:
            t = threading.Thread(target=self._authenticate,
                                 args=(authenticator, timeout_seconds,))
            t.daemon = False
            t.start()
        elif self.is_authenticated() == True and self.is_ready() is False:
            self._boot_vpu()

    def _boot_vpu(self):
        assets_dir = path.join(path.dirname(__file__), 'assets')
        _azureeye.prepare_eye(path.join(assets_dir, "mx.mvcmd"))
        self._ready = True

    def is_ready(self):
        """
        Returns True if the device is authenticated and prepared and ready to work. False otherwise
        """
        return self._ready

    def _authenticate(self, authenticator, timeout_seconds):
        def _auth():
            _azureeye.authorize()
        threading.Thread(target=_auth).start()
        t = 0
        while t < timeout_seconds:
            if self.is_authenticated() is True:
                break
            t += 1
            time.sleep(1)
        if self.is_authenticated() is False:
            raise Exception("Azure Eye could not authenticate")
        else:
            self._boot_vpu()
        sys.exit()

    def is_authenticated(self):
        """
        Returns True when the device attestation with the Azure Percept online service was successful and False otherwise
        """
        dev = usb.core.find(idVendor=0x03e7, idProduct=0x2485)
        if dev is None:
            return False
        else:
            return True

    def start_recording(self, file):
        """
        Starts the video recording as a MP4 file.
        :param str file: 
            A string that specifies the path to a new file to be created
        """
        if self._is_recording is True:
            raise Exception("Already recording. Run stop_recording() first")
        if self.is_ready() == False:
            raise Exception("Device must be ready before recording can start")
        if isinstance(file, str):
            self._is_recording = True
            _azureeye.start_recording(file)
        else:
            raise Exception(
                "start_recording(filepath) must be called with a string")

    def stop_recording(self):
        """
        Stops the video recording and closes the MP4 file.
        """
        _azureeye.stop_recording()
        self._is_recording = False

    def get_frame(self):
        """
        This captures an image using the camera with a numpy array as return type (BGR format - height, width, channels)
        """
        im = _azureeye.get_frame()
        im = np.moveaxis(im, 0, -1)
        im = np.ascontiguousarray(im, dtype=np.uint8)
        return im

    def convert_model(self, filepath, scale_values=None, mean_values=None, reverse_input_channels=False, output_dir="./"):
        """
        Loads a .onnx model file and converts it to a .blob file for the Intel Myriad VPU to use.
        :param str filepath:
            The full path to the onnx model file that should be converted
        :param list scale_values:
            The scale factor as a list for all input channels
        :param list mean_values:
            The values which are substracted from the input channels for image normalization
        :param reverse_input_channels:
            Specifies if the input image channels should be reversed (from BGR to RGB or vice versa)
        :param str output_dir:
            The path to the output directory where the converted .blob file will be placed
        """
        tmp_model_name = str(uuid.uuid4())
        model_name = os.path.basename(filepath).split(".")[0]
        mo_path = str(path.join(path.dirname(__file__)))
        print("Converting model, this can take time, please wait...")
        cmd = f"python3 {mo_path}/mo.py --input_model {filepath} --output_dir /tmp --model_name {tmp_model_name}"

        if scale_values is not None:
            cmd = cmd + " --scale_values " + str(scale_values).replace(" ", "")

        if mean_values is not None:
            cmd = cmd + " --mean_values " + str(mean_values).replace(" ", "")

        if reverse_input_channels is True:
            cmd = cmd + " --reverse_input_channels"

        subprocess.check_call(cmd, shell=True)

        assets_path = str(path.join(path.dirname(__file__), 'assets'))
        subprocess.check_call(
            f"{assets_path}/myriad_compile -m /tmp/{tmp_model_name}.xml -o {path.join(output_dir, model_name)}.blob -VPU_NUMBER_OF_SHAVES 8 -VPU_NUMBER_OF_CMX_SLICES 8 -ip U8 -op FP32", shell=True)

    def start_inference(self, blob_model_path=None, input_src=["/camera1"]):
        """
        Starts the Azure Eye camera and the inference on the VPU based on the .blob model file path
        :param str blob_model_path:
            The path to the .blob model that should be used for inference
        :param list(str) input_src:
            The camera devices as file paths (e.g. "/dev/video0") that should be used as input images for the model. By default the module camera will be used
        """
        if self._inference_running is True:
            raise Exception(
                "Inference already running. Call stop_inference() first")
        if self.is_ready() == False:
            raise Exception("Device must be ready before inference can start")
        if blob_model_path is None:
            raise Exception("blob_model_path must be set to a .blob file path")
        self._inference_running = True
        self.input_src = input_src
        _azureeye.start_inference(blob_model_path, input_src)
        time.sleep(2)

    def stop_inference(self):
        """
        Stops the inference thread
        """
        self._inference_running = False
        _azureeye.stop_inference()

    def get_inference(self, return_image=False, input=b'', input_shape=(720, 1280)):
        """
        Returns the model inference output as a InferenceResult object.
        :param bool return_image:
            If return_image is True, the image from the camera which was used for model inference is returned as well
        :param bytes input:
            If passed, the input (a concatonated B+G+R byte string) is used as input image for the VPU
        :param tuple input_shapes:
            Defines the (height, width) of the input image
        """
        if self._inference_running is False:
            raise Exception(
                f"Inference not started. Call <AzureyeObject>.start_inference('/path/to/model.blob') first")

        totals = []
        items = _azureeye.get_inference(
            return_image, input, input_shape[0], input_shape[1])

        for i, item in enumerate(items):
            if return_image is False or len(input) > 0:
                ret = item
                inference_dimension = len(item)
            else:
                ret = item[0]
                img = item[1]
                inference_dimension = len(ret)

            if inference_dimension == 1:
                ret = ret[0]

            if return_image is False or len(input) > 0:
                totals.append(InferenceResult(
                    self.input_src[i], ret, None, inference_dimension))
            else:
                totals.append(InferenceResult(self.input_src[i], ret, np.ascontiguousarray(
                    np.moveaxis(img, 0, -1), dtype=np.uint8), inference_dimension))

        if len(totals) == 1:
            return totals[0]
        else:
            return totals

    def close(self):
        """
        Cleans up resources. Call this when the VisionDevice object is no longer used.
        """
        time.sleep(1)
        _azureeye.close_eye()
