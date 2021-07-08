import time
from ._device_authentication import DeviceAuthentication
import _hardware
import threading
from ._azure_ear import AzurePercept


class AzureEye(AzurePercept):
    def __init__(self, authenticator: DeviceAuthentication = None, timeout_seconds: int = 100):
        if authenticator is None:
            authenticator = DeviceAuthentication(0x045e, 0x066F)
        super().__init__(authenticator, timeout_seconds)

    def is_authenticated(self):
        pass

    def start_recording(self):
        pass

    def stop_recording(self):
        pass

    def get_raw_frame(self):
        """
        This should return a numpy array with shape (height, width, channels)"""
        pass
