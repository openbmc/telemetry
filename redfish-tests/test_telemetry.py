from redfish_requests import RedfishHttpStatus, RedfishRequest, Duration
import pytest
import time


def test_get_telemetry_service(redfish):
    r = redfish.get(redfish.telemetry_service_path)
    assert r['Status']['State'] == 'Enabled', 'Invalid status of service'
    assert Duration.to_seconds(r['MinCollectionInterval']) > 0, \
        'Invalid duration format'
    assert r['MaxReports'] > 0, 'Invalid count of max reports'


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
    path = report_definitions.add_report("Report1")
    redfish.get(f'{redfish.metric_report_definition_path}/Report1')
    redfish.get(f'{redfish.metric_report_path}/Report1')
    assert 1 == len(report_definitions.get_collection())


def test_add_report_above_max_report_expect_bad_request(
        telemetry, report_definitions):
    for i in range(telemetry.max_reports):
        report_definitions.add_report('Test' + str(i))
    assert telemetry.max_reports == len(report_definitions.get_collection())
    report_definitions.add_report('Test' + str(telemetry.max_reports),
                                  metrics=[], interval=telemetry.min_interval,
                                  code=RedfishHttpStatus.bad_request)


def test_add_report_long_name(report_definitions):
    report_definitions.add_report('Test' * 65)


def test_add_report_twice_expect_bad_request(report_definitions):
    report_definitions.add_report('Test')
    report_definitions.add_report('Test', code=RedfishHttpStatus.bad_request)


@pytest.mark.parametrize(
    'actions', [[], ['RedfishEvent'], ['LogToMetricReportsCollection'],
                ['RedfishEvent', 'LogToMetricReportsCollection']])
def test_add_report_with_actions(actions, redfish, report_definitions):
    report_definitions.add_report('Test', actions=actions)
    r = redfish.get(redfish.metric_report_definition_path + '/Test')
    assert r['ReportActions'] == actions, \
        'Invalid actions, different then requested'


@pytest.mark.parametrize(
    'invalid_actions', [['NonExisting'], ['RedfishEvent', 'Partially'],
                        ['LogToMetricNotThisOne']])
def test_add_report_with_invalid_actions_expect_bad_request(
        invalid_actions, report_definitions):
    report_definitions.add_report('Test', actions=invalid_actions,
                                  code=RedfishHttpStatus.bad_request)


@pytest.mark.parametrize('invalid_id', ['test_-', 't t', 'T.T', 'T,t', 'T:t'])
def test_add_report_with_invalid_id_expect_bad_request(
        invalid_id, report_definitions):
    report_definitions.add_report(invalid_id,
                                  code=RedfishHttpStatus.bad_request)


def test_add_report_with_metric(redfish, telemetry, report_definitions):
    if len(telemetry.metrics) <= 0:
        pytest.skip('Redfish has no sensor available')
    metric = {'MetricId': 'Id1', 'MetricProperties': [telemetry.metrics[0]]}
    report_definitions.add_report('Test', metrics=[metric])
    r = redfish.get(redfish.metric_report_definition_path + '/Test')
    assert len(r['Metrics']) == 1, 'Invalid Metrics, different then requested'
    assert r['Metrics'][0]['MetricId'] == metric['MetricId'], \
        'Invalid MetricId, different then requested'
    assert r['Metrics'][0]['MetricProperties'] == metric['MetricProperties'], \
        'Invalid MetricProperties, different then requested'


def test_add_report_with_invalid_metric_expect_bad_request(report_definitions):
    metric = {
        'MetricId': 'Id1',
        'MetricProperties':
            '/redfish/v1/Chassis/chassis/Sensors/NonExisting/Reading'
    }
    report_definitions.add_report('Test', metrics=[metric],
                                  code=RedfishHttpStatus.bad_request)


def test_add_report_with_many_metrics(redfish, telemetry, report_definitions):
    if len(telemetry.metrics) <= 0:
        pytest.skip('Redfish has no sensor available')
    metrics = []
    for i, prop in enumerate(telemetry.metrics):
        metrics.append({'MetricId': f'Id{str(i)}', 'MetricProperties': [prop]})
    report_definitions.add_report('Test', metrics=metrics)
    r = redfish.get(redfish.metric_report_definition_path + '/Test')
    assert len(r['Metrics']) == len(telemetry.metrics), \
        'Invalid Metrics, different then requested'


def test_add_report_on_request_with_metric_expect_updated_metric_report(
        redfish, telemetry, report_definitions):
    if len(telemetry.metrics) <= 0:
        pytest.skip('Redfish has no sensor available')
    metric = {'MetricId': 'Id1', 'MetricProperties': [telemetry.metrics[0]]}
    report_definitions.add_report('Test', metrics=[metric], type='OnRequest')
    r = redfish.get(redfish.metric_report_path + '/Test')
    assert len(r['MetricValues']) > 0, 'Missing MetricValues'
    metric_value = r['MetricValues'][0]
    assert metric_value['MetricValue'], 'Missing MetricValues'
    assert metric_value['MetricId'] == metric['MetricId'], \
        'Different Id then set in request'
    assert metric_value['MetricProperty'] == metric['MetricProperties'][0], \
        'Different MetricProperty then set in request'


def test_add_report_periodic_with_metric_expect_updated_metric_report(
        redfish, telemetry, report_definitions):
    if len(telemetry.metrics) <= 0:
        pytest.skip('Redfish has no sensor available')
    metric = {'MetricId': 'Id1', 'MetricProperties': [telemetry.metrics[0]]}
    report_definitions.add_report(
        'Test', metrics=[metric], type='Periodic',
        interval=Duration.to_iso8061(telemetry.min_interval))
    time.sleep(telemetry.min_interval + 1)
    r = redfish.get(redfish.metric_report_path + '/Test')
    assert len(r['MetricValues']) > 0, 'Missing MetricValues'
    metric_value = r['MetricValues'][0]
    assert metric_value['MetricValue'], 'Missing MetricValues'
    assert metric_value['MetricId'] == metric['MetricId'], \
        'Different Id then set in request'
    assert metric_value['MetricProperty'] == metric['MetricProperties'][0], \
        'Different MetricProperty then set in request'


@pytest.mark.parametrize('interval', [10, 60, 2400, 90000])
def test_add_report_check_if_duration_is_set(interval, redfish, telemetry,
                                             report_definitions):
    if interval < telemetry.min_interval:
        pytest.skip('Interval is below minimal acceptable value, skipping')
    id = f'Test{str(interval)}'
    report_definitions.add_report(id, type='Periodic',
                                  interval=Duration.to_iso8061(interval))
    r = redfish.get(f'{redfish.metric_report_definition_path}/{id}')
    assert r['Schedule']['RecurrenceInterval'], 'Missing RecurrenceInterval'
    r_interval = Duration.to_seconds(r['Schedule']['RecurrenceInterval'])
    assert interval == r_interval, 'Invalid interval, different then requested'


@pytest.mark.parametrize(
    'invalid', ['50000', 'P12ST', 'PT12S12', 'PPP' 'PD222T222H222M222.222S'])
def test_add_report_with_invalid_duration_response_bad_request(
        invalid, report_definitions):
    r = report_definitions.add_report('Test', type='Periodic', interval=invalid,
                                      code=RedfishHttpStatus.bad_request)
    assert r['error']['@Message.ExtendedInfo'][0], \
        'Wrong response, not an error'
    info = r['error']['@Message.ExtendedInfo'][0]
    assert 'RecurrenceInterval' in info['MessageArgs'], \
        'Wrong response, should contain "RecurrenceInterval"'


def test_stress_add_reports_with_many_metrics_check_metric_reports(
        redfish, telemetry, report_definitions):
    if len(telemetry.metrics) <= 0:
        pytest.skip('Redfish has no sensor available')
    metrics = []
    for i, prop in enumerate(telemetry.metrics):
        metrics.append({'MetricId': f'Id{str(i)}', 'MetricProperties': [prop]})
    for i in range(telemetry.max_reports):
        report_definitions.add_report(f'Test{str(i)}', metrics=metrics)
    for i in range(telemetry.max_reports):
        r = redfish.get(f'{redfish.metric_report_definition_path}/Test{str(i)}')
        assert len(r['Metrics']) == len(telemetry.metrics), \
            'Invalid Metrics, different then requested'
    for i in range(telemetry.max_reports):
        r = redfish.get(f'{redfish.metric_report_path}/Test{str(i)}')
        assert len(r['MetricValues']) > 0, 'Missing MetricValues'
