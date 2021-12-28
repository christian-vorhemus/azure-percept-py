import subprocess
import sys
import os
import site

package_path = site.getsitepackages()[0]
lib_path = os.path.join(package_path, "usr", "lib")

if "LD_LIBRARY_PATH" not in os.environ or lib_path not in os.environ["LD_LIBRARY_PATH"]: 
    package_path = site.getsitepackages()[0]
    lib_path = os.path.join(package_path, "usr", "lib")

    ldpath = os.environ.get("LD_LIBRARY_PATH")
    if ldpath:
        os.environ["LD_LIBRARY_PATH"] = ldpath + ":" + lib_path
    else:
        os.environ["LD_LIBRARY_PATH"] = lib_path

    #res = subprocess.check_output([sys.executable, "-m", "azure.iot.percept"])
    #os.execve(sys.executable, ["-m", "azure.iot.percept"], os.environ)
    subprocess.Popen([sys.executable, "-m", "azure.iot.percept"], env=os.environ)
    #os.execve(sys.argv[0], [], os.environ)

    #from ._device_authentication import DeviceAuthentication
    #from ._azure_ear import AudioDevice
    #from ._azure_eye import VisionDevice, InferenceResult

    
from ._device_authentication import DeviceAuthentication
from ._azure_ear import AudioDevice
from ._azure_eye import VisionDevice, InferenceResult
