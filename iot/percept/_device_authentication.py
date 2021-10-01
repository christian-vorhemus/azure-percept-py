import _hardware
import usb.core
import usb.util
import socket
import time
import sys
import threading


class DeviceAuthentication:
    def __init__(self, vid, pid):
        self.auth_url = "auth.projectsantacruz.azure.net"
        self.vid = vid
        self.pid = pid
        self.write_endpoint = 0x01
        self.read_endpoint = 0x81
        self.is_authenticated = False
        self._current_data = None
        self._previous_data = None
        self._current = b''
        self._is_listening = True
        self._active_thread = False
        self._s = socket.socket()

    def _call_service(self, arr):
        msg_bytes = bytearray(arr)
        self._s.send(msg_bytes)

    def _send_to_usb(self, dev, data_bytes):
        msg = [x for x in data_bytes]
        # print(f"Msg length: {len(msg)}")
        # print("Write:", msg, dev.write(self.write_endpoint, msg))
        dev.write(self.write_endpoint, msg)
        time.sleep(1)
        read_msg = dev.read(self.read_endpoint, 5120)
        # print("Read:", read_msg)
        # print(len(read_msg))
        return read_msg

    def _listen_service(self, arr, max_length=4096):
        # print("Start listening...")
        while self._is_listening:
            received = self._s.recv(max_length)
            # print(f"Received from webservice: {received}")
            self._current += received

            if self._active_thread is False:
                t = threading.Thread(target=self._set_received_data)
                t.daemon = True
                t.start()
                self._active_thread = True

    def _set_received_data(self):
        # print("Thread started")
        timestamp = time.time()
        while True:
            if time.time() > timestamp + 2:
                # print("Data received fully")
                self._previous_data = self._current_data
                self._current_data = self._current
                self._current = b''
                self._active_thread = False
                return

    def start_authentication(self):
        self._s.connect((self.auth_url, 443))
        self._l_thread = threading.Thread(target=self._listen_service, args=(4096,))
        self._l_thread.daemon = True
        self._l_thread.start()

        # This is for audio/ear SoM
        dev = usb.core.find(idVendor=self.vid, idProduct=self.pid)
        if dev is None:
            raise ValueError('Device not found')
        # print(dev)

        if dev.is_kernel_driver_active(0):
            try:
                dev.detach_kernel_driver(0)
                # print("Kernel driver detached")
            except usb.core.USBError as e:
                sys.exit("Could not detach kernel driver: ")

        try:
            dev.set_configuration()
        except:
            print("ERROR: USB SoM is busy")
            exit(1)

        msg = [0x77, 0x01]
        # print("Write:", msg, dev.write(self.write_endpoint, msg))
        dev.write(self.write_endpoint, msg)

        read_msg = dev.read(self.read_endpoint, 5120)

        # print("Read:", read_msg)
        # print(len(read_msg))

        while True:
            if read_msg[1] == 4:
                print("Authentication failed")
                self._is_listening = False
                self._s.shutdown(socket.SHUT_RDWR)
                self._s.close()
                # self._l_thread.join()
                sys.exit()
            elif read_msg[1] == 5:
                # print("Authentication successful!")
                self.is_authenticated = True
                self._is_listening = False
                self._s.shutdown(socket.SHUT_RDWR)
                self._s.close()
                # self._l_thread.join()
                sys.exit()
            elif read_msg[1] == 2:
                # print(f"Call Webservice (2)")
                self._call_service(read_msg[2:])
                time.sleep(1)
                read_msg = dev.read(self.read_endpoint, 5120)
                continue
            elif read_msg[1] == 3:
                # print("Data from USB sensor:")
                # print(read_msg)
                time.sleep(3)
                expected_length = int.from_bytes(read_msg[2:], byteorder='little', signed=False)
                # print(f"Expected length: {expected_length}")
                read_msg = self._send_to_usb(dev, self._current_data[:expected_length])
                self._current_data = self._current_data[expected_length:]
                time.sleep(1)
