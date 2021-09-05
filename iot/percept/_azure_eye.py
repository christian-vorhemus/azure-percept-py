import time
import sys
import argparse
from ._device_authentication import DeviceAuthentication
import threading
import io
import os
import uuid
from os import path
import subprocess
import usb.core
import usb.util
from ._azure_ear import AzurePercept
import _azureeye
import numpy as np
# from .mo import main as mo


class AzureEye(AzurePercept):
    """The AzureEye class initiates the Azure Eye device. It can be used to get camera data or run model inference jobs
    :param DeviceAuthentication authenticator:
        The authenticator object used for SoM authentication. Can be used to customize the VID/PID of the
        USB device as well as the authentication URI used.
    :param int timeout_seconds:
        The maximum time the sensor gets for authentication before abortion
    """

    def __init__(self, authenticator: DeviceAuthentication = None, timeout_seconds: int = 100):
        self._ready = False
        if authenticator is None:
            authenticator = DeviceAuthentication(0x045e, 0x066F)
        if self.is_authenticated() == False:
            t = threading.Thread(target=self._authenticate, args=(authenticator, timeout_seconds,))
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
        threading.Thread(target=authenticator.start_authentication).start()
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
        Starts the video recording as an MP4 file.
        :param str file: 
            A string that specifies the path to a new file to be created
        """
        if self.is_ready() == False:
            raise Exception("Device must be ready before recording can start")
        if isinstance(file, str):
            _azureeye.start_recording(file)
        else:
            raise Exception("start_recording(filepath) must be called with a string")

    def stop_recording(self):
        """
        Stops the video recording and closes the MP4 file.
        """
        _azureeye.stop_recording()

    def get_frame(self):
        """
        This captures an image using the camera with a numpy array as return type (BGR format - height, width, channels)
        """
        # bytes, width, height = _azureeye.get_frame()
        # img = np.zeros((3, height, width))
        # m = 0
        # for i in range(0, 3):
        #     for j in range(0, height):
        #         for k in range(0, width):
        #             img[i][j][k] = bytes[m]
        #             m += 1
        im = _azureeye.get_frame()
        # im = np.uint8(img)
        im = np.moveaxis(im, 0, -1)
        return im

    def convert_model(self, filepath, output_dir="./"):
        """
        Loads a .onnx model file and converts it to a .blob file for the Intel Myriad VPU to use.
        Only 1D model outputs using 32 bit floats are supported as for now.
        :param str filepath:
            The full path to the onnx model file that should be converted
        :param str output_dir:
            The path to the output directory where the converted .blob file will be placed
        """
        tmp_model_name = str(uuid.uuid4())
        model_name = os.path.basename(filepath).split(".")[0]
        mo_path = str(path.join(path.dirname(__file__)))
        print("Converting model, this can take time, please wait...")
        subprocess.check_call(f"python3 {mo_path}/mo.py --input_model {filepath} --output_dir /tmp --model_name {tmp_model_name}", shell=True)

        assets_path = str(path.join(path.dirname(__file__), 'assets'))
        subprocess.check_call(f"{assets_path}/myriad_compile -m /tmp/{tmp_model_name}.xml -o {path.join(output_dir, model_name)}.blob -VPU_NUMBER_OF_SHAVES 8 -VPU_NUMBER_OF_CMX_SLICES 8 -ip U8 -op FP32", shell=True)

    def start_inference(self, blob_model_path=None, device=None):
        """
        Starts the Azure Eye camera and the inference on the VPU based on the .blob model file path
        :param str blob_model_path:
            The path to the .blob model that should be used for inference
        """
        if self.is_ready() == False:
            raise Exception("Device must be ready before inference can start")
        if blob_model_path is None:
            raise Exception("blob_model_path must be set to a .blob file path")
        self._inference_running = True
        _azureeye.start_inference(blob_model_path)
        time.sleep(2)

    def stop_inference(self):
        """
        Stops the inference thread
        """
        self._inference_running = False
        _azureeye.stop_inference()

    def get_inference(self):
        """
        Returns the model inference output as a numpy array. The array length depends on the model specification.
        """
        if self._inference_running is False:
            raise Exception(f"Inference not started. Call <AzureyeObject>.start_inference('/path/to/model.blob') first")

        res, res_type = _azureeye.get_inference()
        if res_type == 0:
            le = int(len(res) / 2)
            return np.frombuffer(res, dtype='float16', count=le, offset=0)
        elif res_type == 1:
            le = int(len(res))
            return np.frombuffer(res, dtype='uint8', count=le, offset=0)
        elif res_type == 2:
            le = int(len(res) / 4)
            return np.frombuffer(res, dtype='int32', count=le, offset=0)
        elif res_type == 3:
            le = int(len(res) / 4)
            return np.frombuffer(res, dtype='float32', count=le, offset=0)
        elif res_type == 4:
            le = int(len(res))
            return np.frombuffer(res, dtype='int8', count=le, offset=0)
        else:
            raise Exception(f"Unknown datatype received on return: {res_type}")

    def close(self):
        """
        Cleans up resources. Call this when the AzureEye object is no longer used.
        """
        _azureeye.close_eye()
