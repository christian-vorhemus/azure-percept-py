import time
from ._device_authentication import DeviceAuthentication
import _hardware
import threading
from ._azure_ear import AzurePercept


class AzureEye(AzurePercept):
    def __init__(self, authenticator: DeviceAuthentication = None, timeout_seconds: int = 80):
        self.is_authenticated = False
        self.device_no = None
        self.managed_file = False
        if authenticator is None:
            authenticator = DeviceAuthentication(0x045e, 0x066F)
        if _hardware.get_azure_ear_hardware() == -1:
            threading.Thread(target=self._authenticate, args=(authenticator, timeout_seconds,)).start()
        self.device_no = f"hw:{_hardware.get_azure_ear_hardware()},0"
        _hardware.prepare_device(self.device_no)

    def _authenticate(self, authenticator, timeout_seconds):
        authenticator.start_authentication()
        t = 0
        while t < timeout_seconds:
            if authenticator.is_authenticated == True:
                self.is_authenticated = True
                break
            t += 1
            time.sleep(1)
        if self.is_authenticated == False:
            raise Exception("Azure Eye could not authenticate")

    def start_recording(self):
        pass

    def stop_recording(self):
        pass

    def get_raw_frame(self):
        """
        This should return a numpy array with shape (height, width, channels)"""
        pass
