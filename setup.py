#from distutils.core import setup, Extension
from setuptools import setup, Extension
import os
from os import path
import numpy as np

def get_files_recursively(directory):
    paths = []
    for (path, directories, filenames) in os.walk(directory):
        for filename in filenames:
            paths.append(os.path.join(path, filename))
    return paths

def main():
    mo_files = get_files_recursively('iot/percept/mo')
    extensions_files = get_files_recursively('iot/percept/extensions')
    package_files = mo_files + extensions_files + ['iot/percept/mo.py', 'iot/percept/mo_onnx.py', 'iot/percept/assets/myriad_compile', 
'iot/percept/assets/mx.mvcmd', 
'iot/percept/_azure_eye.py', 'iot/percept/__init__.py', 
'iot/percept/_azure_ear.py', 'iot/percept/_device_authentication.py', 'iot/percept/requirements_onnx.txt'] 
    assets_path = str(path.join(path.dirname(__file__), 'iot', 'percept', 'assets'))
    numpy_include_path = path.join(path.dirname(np.__file__), "core/include")
    setup(name="azure-percept",
          version="1.0.0",
          description="Unofficially Python package to control the Azure Percept SoM",
          author="Christian Vorhemus",
          author_email="chris.vorhemus@outlook.com",
          packages=['azure'],
          package_dir={'azure':'.'},
          install_requires=[
              'numpy>=1.16.6,<1.20', 'pyusb', 'networkx~=2.5', 'protobuf>=3.15.6', 'onnx>=1.8.1', 'defusedxml>=0.7.1'
          ],
          package_data={
              'azure': package_files
          },
          data_files=[('/usr/lib', ['iot/percept/assets/libswresample.so', 'iot/percept/assets/libmxIf.so', 'iot/percept/assets/libmxIf.a', 'iot/percept/assets/libavcodec.so.57', 'iot/percept/assets/libavformat.so.57', 'iot/percept/assets/libavutil.so.55', 'iot/percept/assets/libswresample.so.2', 'iot/percept/assets/libswscale.so.4'])],
          ext_modules=[Extension("_hardware", ["iot/percept/ext/perceptmodule.c"], extra_link_args=["-lasound", "-lpthread"]),
                       Extension("_azureeye", ["iot/percept/ext/azureeyemodule.cpp"], include_dirs=["iot/percept/ext/include", 
"iot/percept/ext/include/VPUInferBlock/host/include", "iot/percept/ext/include/XLink", 
"iot/percept/ext/include/common/host", "iot/percept/ext/include/VPUCameraBlock/host", "iot/percept/ext/include/libavformat", "iot/percept/ext/include/libavutil", 
"iot/percept/ext/include/libavcodec", numpy_include_path], 
extra_objects=[path.join(assets_path, "libmxIf.so"), path.join(assets_path, "libmxIf.a")], 
library_dirs=[assets_path], libraries=["avformat", "avcodec", "avutil", "swscale"],
extra_link_args=["-lpthread", "-lusb-1.0", "-lstdc++"])])


if __name__ == "__main__":
    main()
