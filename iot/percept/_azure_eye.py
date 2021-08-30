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
        raise Exception("Not implemented as for now")
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
        raise Exception("Not implemented as for now")
        _azureeye.stop_recording()

    def get_frame(self):
        """
        This captures an image using the camera with a numpy array as return type (BGR format - height, width, channels)
        """
        bytes, width, height = _azureeye.get_frame()
        img = np.zeros((3, height, width))
        m = 0
        for i in range(0, 3):
            for j in range(0, height):
                for k in range(0, width):
                    img[i][j][k] = bytes[m]
                    m += 1
        im = np.uint8(img)
        im = np.moveaxis(im, 0, -1)
        return im

    def convert_model(self, filepath, output_dir="./"):
        """
        Loads a .onnx model file and converts it to a .blob file for the Intel Myriad VPU to use
        """
        tmp_model_name = str(uuid.uuid4())
        model_name = os.path.basename(filepath).split(".")[0]
        # parser = argparse.ArgumentParser()
        # parser.add_argument('--input_model',required=False, default=filepath)
        # parser.add_argument('--output_dir',required=False, default='/tmp')
        # parser.add_argument('--silent', required=False, default=True)
        # parser.add_argument('--log_level', required=False, default='ERROR')
        # parser.add_argument('--model_name', required=False, default=tmp_model_name)
        # parser.add_argument('--transform', required=False, default="")
        # parser.add_argument('--legacy_ir_generation', required=False, default=False)
        # parser.add_argument('--reverse_input_channels', required=False, default=False)
        # parser.add_argument('--scale', required=False)
        # parser.add_argument('--input_shape', required=False)
        # parser.add_argument('--input', required=False)
        # parser.add_argument('--output', required=False)
        # parser.add_argument('--mean_values', required=False, default=())
        # parser.add_argument('--scale_values', required=False, default=())
        # parser.add_argument('--data_type', required=False, default='float')
        # parser.add_argument('--disable_fusing', required=False, default=True)
        # parser.add_argument('--disable_resnet_optimization', required=False, default=True)
        # parser.add_argument('--finegrain_fusing', required=False)
        # parser.add_argument('--disable_gfusing', required=False, default=True)
        # parser.add_argument('--enable_concat_optimization', required=False, default=True)
        # parser.add_argument('--move_to_preprocess', required=False, default=True)
        # parser.add_argument('--extensions', required=False, default="DIR")
        # parser.add_argument('--batch', required=False, default=None)
        # parser.add_argument('--version', required=False, default="2021.4.0-3839-cd81789d294-releases/2021/4")
        # parser.add_argument('--freeze_placeholder_with_value', required=False, default=None)
        # parser.add_argument('--generate_deprecated_IR_V7', required=False, default=False)
        # parser.add_argument('--static_shape', required=False, default=False)
        # parser.add_argument('--keep_shape_ops', required=False, default=True)
        # parser.add_argument('--disable_weights_compression', required=False, default=False)
        # parser.add_argument('--progress', required=False, default=False)
        # parser.add_argument('--stream_output', required=False, default=False)
        # parser.add_argument('--transformations_config', required=False)

        mo_path = str(path.join(path.dirname(__file__)))
        print("Converting model, this can take time, please wait...")
        subprocess.check_call(f"python3 {mo_path}/mo.py --input_model {filepath} --output_dir /tmp --model_name {tmp_model_name}", shell=True)

        # mo.main(parser, 'onnx')
        assets_path = str(path.join(path.dirname(__file__), 'assets'))
        subprocess.check_call(f"{assets_path}/myriad_compile -m /tmp/{tmp_model_name}.xml -o {path.join(output_dir, model_name)}.blob -VPU_NUMBER_OF_SHAVES 8 -VPU_NUMBER_OF_CMX_SLICES 8 -ip U8 -op FP32", shell=True)

    def start_inference(self, blob_model_path=None):
        """
        Starts the Azure Eye camera and the inference on the VPU based on the .blob model file path
        """
        self._inference_running = True
        _azureeye.start_inference(blob_model_path)

    def stop_inference(self):
        self._inference_running = False
        _azureeye.stop_inference()

    def get_inference(self):
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
        _azureeye.close_eye()
