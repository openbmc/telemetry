from redfish_requests import RedfishHttpStatus, RedfishRequest
import pytest


def test_get_telemetry_service(redfish):
    r = redfish.get(redfish.telemetry_service_path)
    assert r['Status']['State'] == 'Enabled', 'Wrong status'


def test_get_metric_definition(redfish):
    r = redfish.get(redfish.metric_definition_path)
    assert 'Members' in r, 'Missing members property'
    assert 'Members@odata.count' in r, 'Missing members count property'


def test_verify_metric_definition_members_if_contains_metrics(redfish):
    r = redfish.get(redfish.metric_definition_path)
    for m in r['Members']:
        path = m['@odata.id']
        metricDefinition = redfish.get(path)
        assert 'MetricProperties' in metricDefinition, 'Missing metrics'
        assert len(metricDefinition['MetricProperties']) > 0, 'Missing metrics'


def test_get_metric_report_definition(redfish):
    r = redfish.get(redfish.metric_report_definition_path)
    assert 'Members' in r, 'Missing members property'
    assert 'Members@odata.count' in r, 'Missing members count property'


def test_get_metric_report(redfish):
    r = redfish.get(redfish.metric_report_path)
    assert 'Members' in r, 'Missing members property'
    assert 'Members@odata.count' in r, 'Missing members count property'


def test_post_report_definition_with_empty_body_expect_bad_request(redfish):
    redfish.post(redfish.metric_report_definition_path, body={},
                 code=RedfishHttpStatus.bad_request)


def test_post_report_definition_with_some_body_expect_bad_request(redfish):
    redfish.post(redfish.metric_report_definition_path, body={'key': 'value'},
                 code=RedfishHttpStatus.bad_request)


def test_delete_non_exisiting_metric_report_definition(redfish):
    redfish.delete(
        f'{redfish.metric_report_definition_path}/NonExisitingReport',
        code=RedfishHttpStatus.not_found)


def test_add_report(redfish, report_definitions):
    path = report_definitions.add_report("Report1", metrics=[])
    redfish.get(f'{redfish.metric_report_definition_path}/Report1')
    assert 1 == len(report_definitions.get_collection())


def test_add_report_above_max_report_expect_bad_request(
        telemetry, report_definitions):
    for i in range(telemetry.max_reports):
        report_definitions.add_report('Report' + str(i), metrics=[])
    assert telemetry.max_reports == len(report_definitions.get_collection())
    report_definitions.add_report('Report' + str(telemetry.max_reports),
                                  metrics=[],
                                  code=RedfishHttpStatus.bad_request)


def test_add_report_with_metric(telemetry, report_definitions):
    if len(telemetry.metrics) <= 0:
        pytest.skip('Redfish has no sensor available')
    metric = {'MetricId': 'Id1', 'MetricProperties': [telemetry.metrics[0]]}
    report_definitions.add_report('Report', metrics=[metric])
