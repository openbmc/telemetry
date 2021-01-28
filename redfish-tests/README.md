# Requirements

At least python3.6 is required and following two python packages installed - requests pytest.

```
$ python3 --version
Python 3.6.8

$ pytest --version
This is pytest version 5.4.2, imported from /usr/local/lib/python3.6/site-packages/pytest/__init__.py

$ sudo pip3 install request pytest
```

# Config

Head of the test\_telemetry.py contains WEB\_ADDR that points to bmcweb service.

# Run

```
# To run all tests in verbose mode and with stop on first failed test (s option direct all print to stdout)
pytest test_telemetry.py -xsv

# To run specific test using  statement
pytest test_telemetry.py -xsv -k 'remove_all'
```
