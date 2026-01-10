# bgw_flipper_ir_uart

https://github.com/flipperdevices/flipperzero-ufbt-action

aceback (most recent call last):
  File "/Users/bgw/.ufbt/toolchain/arm64-darwin/lib/python3.11/site-packages/serial/serialposix.py", line 575, in read

--- exit ---
    buf = os.read(self.fd, size - len(read))
          ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
OSError: [Errno 6] Device not configured

During handling of the above exception, another exception occurred:

Traceback (most recent call last):
  File "/Users/bgw/.ufbt/toolchain/arm64-darwin/lib/python3.11/threading.py", line 1045, in _bootstrap_inner
    self.run()
  File "/Users/bgw/.ufbt/toolchain/arm64-darwin/lib/python3.11/threading.py", line 982, in run
    self._target(*self._args, **self._kwargs)
  File "/Users/bgw/.ufbt/toolchain/arm64-darwin/lib/python3.11/site-packages/serial/tools/miniterm.py", line 499, in reader
    data = self.serial.read(self.serial.in_waiting or 1)
           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/Users/bgw/.ufbt/toolchain/arm64-darwin/lib/python3.11/site-packages/serial/serialposix.py", line 581, in read
    raise SerialException('read failed: {}'.format(e))
serial.serialutil.SerialException: read failed: [Errno 6] Device not configured
(venv) bgwxmacbook:build1.5 bgw$