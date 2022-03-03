from setuptools import setup, Extension
import os
import logging
from os import path
import site
from distutils.sysconfig import get_python_lib
import subprocess
import sys
from pathlib import Path
this_directory = Path(__file__).parent
long_description = (this_directory / "README.md").read_text()

package_path = site.getsitepackages()[0]

def check_package_manager():
    try:
        is_yum = subprocess.check_output("which yum 2>/dev/null", shell=True)
        return "yum"
    except:
        pass
    try:
        is_yum = subprocess.check_output("which apt 2>/dev/null", shell=True)
        return "apt"
    except:
        pass
    return ""

def install_yum_dependencies():
    subprocess.check_call("yum install -y alsa-lib-devel libusb-devel", shell=True)

def install_apt_dependencies():
    subprocess.check_call("apt update && apt install -y libasound2-dev libusb-1.0-0-dev", shell=True)

def install_numpy():
    subprocess.check_call([sys.executable, "-m", "pip", "install", "numpy"])

def get_files_recursively(directory):
    paths = []
    for (path, directories, filenames) in os.walk(directory):
        for filename in filenames:
            paths.append(os.path.join(path, filename))
    return paths

def main():
    import numpy
    mo_files = get_files_recursively('iot/percept/mo')
    extensions_files = get_files_recursively('iot/percept/extensions')
    package_files = mo_files + extensions_files + ['iot/percept/mo.py', 'iot/percept/mo_onnx.py', 'iot/percept/assets/myriad_compile', 
'iot/percept/assets/mx.mvcmd', 
'iot/percept/_azure_eye.py', 'iot/percept/__init__.py', 'iot/percept/__main__.py', 
'iot/percept/_azure_ear.py', 'iot/percept/_device_authentication.py', 'iot/percept/requirements_onnx.txt'] 
    assets_path = str(path.join(path.dirname(__file__), 'iot', 'percept', 'assets'))
    #numpy_include_path = path.join(package_path, "numpy/core/include")
    numpy_include_path = numpy.get_include()
    setup(name="azure-percept",
          version="0.0.13",
          description="Unofficial Python package to control the Azure Percept SoM",
          url="https://github.com/christian-vorhemus/azure-percept-py",
          long_description=long_description,
          long_description_content_type='text/markdown',
          license_files=('LICENSE',),
          author="Christian Vorhemus",
          author_email="",
          packages=['azure'],
          package_dir={'azure':'.'},
          install_requires=[
              'numpy>=1.16.6', 'pyusb', 'networkx~=2.5', 'protobuf>=3.15.6', 'onnx>=1.8.1', 'defusedxml>=0.7.1'
          ],
          package_data={
              'azure': package_files
          },
          data_files=[('/lib64', ['iot/percept/assets/plugins.xml',
'iot/percept/assets/libinference_engine_ir_reader.so']), ('/azure_percept.libs', 
['iot/percept/assets/plugins.xml', 
'iot/percept/assets/libinference_engine_ir_reader.so']), ('/usr/lib', 
['iot/percept/assets/libmyriadPlugin.so', 
'iot/percept/assets/libngraph.so', 'iot/percept/assets/libusb-1.0.so.0', 
'iot/percept/assets/libusb-1.0.so', 'iot/percept/assets/libtbb.so.2', 'iot/percept/assets/libtbbmalloc.so.2',
'iot/percept/assets/libasound.so.2', 'iot/percept/assets/libasound.so',
'iot/percept/assets/libswresample.so', 'iot/percept/assets/libmxIf.so', 'iot/percept/assets/libavcodec.so.57', 
'iot/percept/assets/libinference_engine.so', 
'iot/percept/assets/libinference_engine_ir_reader.so', 'iot/percept/assets/libinference_engine_legacy.so', 
'iot/percept/assets/libinference_engine_transformations.so', 
'iot/percept/assets/libavformat.so.57', 
'iot/percept/assets/libavutil.so.55', 'iot/percept/assets/libswresample.so.2', 'iot/percept/assets/libswscale.so.4'])],
          ext_modules=[Extension("_hardware", ["iot/percept/ext/perceptmodule.c", "iot/percept/ext/validator.c"], 
include_dirs=["iot/percept/ext/include"], library_dirs=[assets_path], libraries=["asound"], 
extra_link_args=["-lpthread", "-ludev", "-lusb-1.0", "-Wl,-rpath=$ORIGIN/usr/lib"]),
                       Extension("_azureeye", ["iot/percept/ext/azureeyemodule.cpp", "iot/percept/ext/usbcamera.c", "iot/percept/ext/validator.c"], 
include_dirs=["iot/percept/ext/include", 
"iot/percept/ext/include/VPUInferBlock/host/include", "iot/percept/ext/include/XLink", 
"iot/percept/ext/include/common/host", "iot/percept/ext/include/VPUCameraBlock/host", "iot/percept/ext/include/libavformat", "iot/percept/ext/include/libavutil", 
"iot/percept/ext/include/libavcodec", numpy_include_path], 
extra_objects=[path.join(assets_path, "libmxIf.so")], library_dirs=[assets_path], libraries=["avformat", "avcodec", 
"avutil", "swscale"],
extra_link_args=["-lpthread", "-ludev", "-lusb-1.0", "-lstdc++", "-Wl,-rpath=$ORIGIN/usr/lib"])])


if __name__ == "__main__":
    install_numpy()
    main()
