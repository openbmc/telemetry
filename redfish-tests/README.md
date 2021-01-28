# Requirements

At least python3.6 is required and following two python packages installed -
requests, pytest.

```
$ python3 --version
Python 3.6.8

$ pytest --version
This is pytest version 5.4.2, imported from /usr/local/lib/python3.6/site-packages/pytest/__init__.py

$ sudo pip3 install request pytest
```

# Config

There are few options included to tests:
- `--host_addr` - address of an OpenBMC together with protocol and port, ex.
'https://localhost:443'
- `--username` - name of the user that is used during tests, user should have
'ConfigureManager' privilege
- `--password` - plain password

By default above options are set to 'https://localhost:4443', 'root' and
'0penBmc'.

# Run

```
# To run all tests in verbose mode and with stop on first failed test (s option direct all print to stdout)
pytest test_telemetry.py -xsv

# To run specific test using  statement
pytest test_telemetry.py -xsv -k 'remove_all'
```
