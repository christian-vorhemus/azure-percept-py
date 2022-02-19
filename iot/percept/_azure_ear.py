import time
from ._device_authentication import DeviceAuthentication
import threading
import io
import os
import sys
import _hardware
from abc import ABC
import numpy as np

class AzurePercept(ABC):
    def __init__(self, authenticator: DeviceAuthentication = None, timeout_seconds: int = 100):
        pass


class AudioDevice(AzurePercept):
    """The AudioDevice class can be used to record audio from the microphone array and save it as WAV file
    :param DeviceAuthentication authenticator:
        The authenticator object used for SoM authentication. Can be used to customize the VID/PID of the
        USB device as well as the authentication URI used.
    :param int timeout_seconds:
        The maximum time the sensor gets for authentication before abortion
    """

    def __init__(self, authenticator: DeviceAuthentication = None, timeout_seconds: int = 100):
        # super().__init__(authenticator, timeout_seconds)
        self.device_no = None
        self._ready = False
        self._is_recording = False
        if authenticator is None:
            authenticator = DeviceAuthentication(0x045e, 0x0673)
        if self.is_authenticated() == False:
            t = threading.Thread(target=self._authenticate, args=(authenticator, timeout_seconds,))
            t.daemon = True
            t.start()
        else:
            self.device_no = f"hw:{_hardware.get_azure_ear_hardware()},0"
            _hardware.prepare_ear(self.device_no)
            self._ready = True

    def is_ready(self):
        """
        Returns True if the device is authenticated and prepared and ready to work. False otherwise
        """
        return self._ready

    def is_authenticated(self):
        """
        Returns True when the device attestation with the Azure Percept online service was successful and False otherwise
        """
        if _hardware.get_azure_ear_hardware() == -1:
            return False
        else:
            return True

    def _authenticate(self, authenticator, timeout_seconds):
        def _auth():
            _hardware.authorize()
        threading.Thread(target=_auth).start()
        t = 0
        while t < timeout_seconds:
            if _hardware.get_azure_ear_hardware() != -1:
                break
            t += 1
            time.sleep(1)
        if _hardware.get_azure_ear_hardware() == -1:
            raise Exception("Azure Ear could not authenticate")
        else:
            self.device_no = f"hw:{_hardware.get_azure_ear_hardware()},0"
            _hardware.prepare_ear(self.device_no)
            self._ready = True
        sys.exit()

    def start_recording(self, file):
        """
        Starts the audio recording as a WAV file.
        :param io.BufferedWriter file:
            File can either be BufferedWriter opened with open("./file.wav", "wb") or a 
            string that specifies the path to a new file to be created
        """
        if self._is_recording is True:
            raise Exception("Already recording. Run stop_recording() first")
        if self.is_ready() == False:
            raise Exception("Device must be authenticated before recording can start")
        # if isinstance(file, io.BufferedWriter):
        #     _hardware.start_recording(file)
        if isinstance(file, str):
            # self.file = open(file, 'w+')
            self._is_recording = True
            _hardware.start_recording(file)
        else:
            raise Exception("start_recording(filehandle) must be called with a file handle")

    def stop_recording(self):
        """
        Stops the audio recording and closes the WAV file.
        """
        _hardware.stop_recording()
        self._is_recording = False

    def close(self):
        """
        Cleans up resources. Call this when the AzureEar object is no longer used.
        """
        _hardware.close_ear()
