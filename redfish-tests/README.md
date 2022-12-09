# Redfish Tests

## Requirements

At least python3.6 is required and following two python packages installed -
requests, pytest.

```sh
$ python3 --version
Python 3.6.8

$ pytest --version
This is pytest version 5.4.2, imported from /usr/local/lib/python3.6/site-packages/pytest/__init__.py

$ sudo pip3 install request pytest
```

## Config

There are few options included to tests:

- `--host_addr` - address of an OpenBMC together with protocol and port, ex.
  'https://localhost:443', default: 'https://localhost:4443'
- `--username` - name of the user that is used during tests, user should have
  'ConfigureManager' privilege, default: 'root'
- `--password` - plain password, default: '0penBmc'
- `--metric_limit` - allows to limit number of detected metrics in system,
  default: 200

SSL verification is disabled during tests.

## Run

```sh
# To run all tests in verbose mode and with stop on first failed test (s option direct all print to stdout)
 pytest test_telemetry.py -xsv

# To run specific test using statement
 pytest test_telemetry.py -xsv -k 'test_get_telemetry_service or test_get_metric_report'
```
