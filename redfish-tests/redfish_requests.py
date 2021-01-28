import enum
import math
import re
import requests


class RedfishHttpStatus(enum.IntEnum):
    ok = 200
    created = 201
    no_content = 204
    bad_request = 400
    not_found = 404
    internal_server_error = 500


class RedfishRequest:
    telemetry_service_path = '/redfish/v1/TelemetryService'
    metric_definition_path = f'{telemetry_service_path}/MetricDefinitions'
    metric_report_definition_path = \
        f'{telemetry_service_path}/MetricReportDefinitions'
    metric_report_path = f'{telemetry_service_path}/MetricReports'

    def __init__(self, host_addr, username, password):
        self.host_addr = host_addr
        self.username = username
        self.password = password

    def get(self, path, code=RedfishHttpStatus.ok):
        u = self.host_addr + path
        r = requests.get(u, auth=(self.username, self.password), verify=False)
        assert r.status_code == code, \
            f'{r.status_code} == {code} on path {u}\n{r.text}'
        print(r.text)
        return r.json()

    def post(self, path, body, code=RedfishHttpStatus.created):
        u = self.host_addr + path
        r = requests.post(u, auth=(self.username, self.password), verify=False,
                          json=body)
        assert r.status_code == code, \
            f'{r.status_code} == {code} on path {u}\n{r.text}'
        print(r.text)
        return r.json()

    def delete(self, path, code=RedfishHttpStatus.no_content):
        u = self.host_addr + path
        r = requests.delete(u, auth=(self.username, self.password),
                            verify=False)
        assert r.status_code == code, \
            f'{r.status_code} == {code} on path {u}\n{r.text}'


class TelemetryService:
    def __init__(self, redfish, metric_limit):
        r = redfish.get(redfish.telemetry_service_path)
        self.min_interval = Duration.to_seconds(r['MinCollectionInterval'])
        self.max_reports = r['MaxReports']
        self.metrics = []
        r = redfish.get(redfish.metric_definition_path)
        for m in r['Members']:
            path = m['@odata.id']
            metricDef = redfish.get(path)
            self.metrics += [x for x in metricDef['MetricProperties']]
        self.metrics = self.metrics[:metric_limit]


class ReportDef:
    def __init__(self, redfish):
        self.redfish = redfish

    def get_collection(self):
        r = self.redfish.get(self.redfish.metric_report_definition_path)
        return [x['@odata.id'] for x in r['Members']]

    def add_report(self, id, metrics=None, type='OnRequest', actions=None,
                   interval=None, code=RedfishHttpStatus.created):
        body = {
            'Id': id,
            'Metrics': [],
            'MetricReportDefinitionType': type,
            'ReportActions': ['RedfishEvent', 'LogToMetricReportsCollection']
        }
        if metrics is not None:
            body['Metrics'] = metrics
        if actions is not None:
            body['ReportActions'] = actions
        if interval is not None:
            body['Schedule'] = {'RecurrenceInterval': interval}
        return self.redfish.post(self.redfish.metric_report_definition_path,
                                 body, code)

    def delete_report(self, path):
        self.redfish.delete(f'{path}')


class Duration:
    def __init__(self):
        pass

    def to_iso8061(time):
        assert time >= 0, 'Invalid argument, time is negative'
        days = int(time / (24 * 60 * 60))
        time = math.fmod(time, (24 * 60 * 60))
        hours = int(time / (60 * 60))
        time = math.fmod(time, (60 * 60))
        minutes = int(time / 60)
        time = round(math.fmod(time, 60), 3)
        return f'P{str(days)}DT{str(hours)}H{str(minutes)}M{str(time)}S'

    def to_seconds(duration):
        r = re.fullmatch(r'-?P(\d+D)?(T(\d+H)?(\d+M)?(\d+(.\d+)?S)?)?',
                         duration)
        assert r, 'Invalid argument, not match with regex'
        days = r.group(1)
        hours = r.group(3)
        minutes = r.group(4)
        seconds = r.group(5)
        result = 0
        if days is not None:
            result += int(days[:-1]) * 60 * 60 * 24
        if hours is not None:
            result += int(hours[:-1]) * 60 * 60
        if minutes is not None:
            result += int(minutes[:-1]) * 60
        if seconds is not None:
            result += float(seconds[:-1])
        return result
