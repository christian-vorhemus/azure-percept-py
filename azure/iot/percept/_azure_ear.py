import time
from ._device_authentication import DeviceAuthentication
import threading
import io
import _hardware
from abc import ABC


class AzurePercept(ABC):
    def __init__(self, authenticator: DeviceAuthentication = None, timeout_seconds: int = 100):
        pass


class AzureEar(AzurePercept):
    """The AzureEar class can be used to record audio from the microphone array and save it as WAV file
    :param DeviceAuthentication authenticator:
        The authenticator object used for SoM authentication. Can be used to customize the VID/PID of the
        USB device as well as the authentication URI used.
    :param int timeout_seconds:
        The maximum time the sensor gets for authentication before abortion
    """

    def __init__(self, authenticator: DeviceAuthentication = None, timeout_seconds: int = 100):
        # super().__init__(authenticator, timeout_seconds)
        self.device_no = None
        self.managed_file = False
        if authenticator is None:
            authenticator = DeviceAuthentication(0x045e, 0x0673)
        if _hardware.get_azure_ear_hardware() == -1:
            threading.Thread(target=self._authenticate, args=(authenticator, timeout_seconds,)).start()
        else:
            self.device_no = f"hw:{_hardware.get_azure_ear_hardware()},0"
            _hardware.prepare_device(self.device_no)

    def is_authenticated(self):
        if _hardware.get_azure_ear_hardware() == -1:
            return False
        else:
            return True

    def _authenticate(self, authenticator, timeout_seconds):
        authenticator.start_authentication()
        t = 0
        while t < timeout_seconds:
            if authenticator.is_authenticated == True:
                break
            t += 1
            time.sleep(1)
        if authenticator.is_authenticated == False:
            raise Exception("Azure Eye could not authenticate")
        else:
            self.device_no = f"hw:{_hardware.get_azure_ear_hardware()},0"
            _hardware.prepare_device(self.device_no)

    def start_recording(self, file):
        """
        Starts the audio recording as a WAV file.
        :param io.BufferedWriter file:
            File can either be BufferedWriter opened with open("./file.wav", "wb") or a 
            string that specifies the path to a new file to be created
        """
        self.managed_file = False
        if self.is_authenticated() == False:
            raise Exception("Device must be authenticated before recording can start")
        if isinstance(file, io.BufferedWriter):
            _hardware.start_recording(file)
        elif isinstance(file, str):
            self.file = open(file, 'w+')
            self.managed_file = True
            _hardware.start_recording(self.file)
        else:
            raise Exception("start_recording(filehandle) must be called with a file handle")

    def stop_recording(self):
        """
        Stops the audio recording and closes the WAV file.
        """
        _hardware.stop_recording()

    def get_raw_audio(self, frames_count=160):
        """
        Returns <frames_count> audio frames from the device as a byte array representing PCM amplitude values.
        The total number of bytes returned is bit depth (32 bit = 4 byte) * channels (5) * <frames_count>
        :param int frames_count:
            Specifies the number of frames read and returned from the audio interface
        """
        return _hardware.get_raw_audio(frames_count)
