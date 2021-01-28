import pytest
import redfish_requests


def pytest_addoption(parser):
    parser.addoption('--host_addr', action='store',
                     default='https://localhost:4443')
    parser.addoption('--username', action='store', default='root')
    parser.addoption('--password', action='store', default='0penBmc')
    parser.addoption('--metric_limit', action='store', default=200)


@pytest.fixture(scope='session')
def redfish(request):
    host_addr = request.config.getoption('--host_addr')
    username = request.config.getoption('--username')
    password = request.config.getoption('--password')
    return redfish_requests.RedfishRequest(host_addr, username, password)


@pytest.fixture(scope='session')
def telemetry(request, redfish):
    metric_limit = request.config.getoption('--metric_limit')
    return redfish_requests.TelemetryService(redfish, metric_limit)


@pytest.fixture(scope='function')
def report_definitions(redfish):
    report_definitions = redfish_requests.ReportDef(redfish)
    print('Cleaning reports before test')
    for report in report_definitions.get_collection():
        report_definitions.delete_report(report)
    yield report_definitions
    print('Cleaning reports after test')
    for report in report_definitions.get_collection():
        report_definitions.delete_report(report)
