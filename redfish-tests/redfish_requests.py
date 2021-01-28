import enum
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
    metric_definition_path = telemetry_service_path + '/MetricDefinitions'
    metric_report_definition_path = \
        telemetry_service_path + '/MetricReportDefinitions'
    metric_report_path = telemetry_service_path + '/MetricReports'

    def __init__(self, host_addr, username, password):
        self.host_addr = host_addr
        self.username = username
        self.password = password

    def get(self, path, code=RedfishHttpStatus.ok):
        u = self.host_addr + path
        r = requests.get(u, auth=(self.username, self.password), verify=False)
        assert r.status_code == code, \
            f"{r.status_code} == {code} on path {u}\n{r.text}"
        print(r.text)
        return r.json()

    def post(self, path, body, code=RedfishHttpStatus.created):
        u = self.host_addr + path
        r = requests.post(u, auth=(self.username, self.password), verify=False,
                          json=body)
        assert r.status_code == code, \
            f"{r.status_code} == {code} on path {u}\n{r.text}"
        print(r.text)

    def delete(self, path, code=RedfishHttpStatus.no_content):
        u = self.host_addr + path
        r = requests.delete(u, auth=(self.username, self.password),
                            verify=False)
        assert r.status_code == code, \
            f"{r.status_code} == {code} on path {u}\n{r.text}"


class TelemetryService:
    def __init__(self, redfish):
        r = redfish.get(redfish.telemetry_service_path)
        self.min_interval = r["MinCollectionInterval"]
        self.max_reports = r["MaxReports"]
        self.metrics = []
        r = redfish.get(redfish.metric_definition_path)
        for m in r['Members']:
            path = m['@odata.id']
            metricDef = redfish.get(path)
            self.metrics += [x for x in metricDef['MetricProperties']]


class ReportDef:
    def __init__(self, redfish, telemetry):
        self.redfish = redfish
        self.min_interval = telemetry.min_interval
        self.metrics = telemetry.metrics

    def get_collection(self):
        r = self.redfish.get(self.redfish.metric_report_definition_path)
        return [x["@odata.id"] for x in r["Members"]]

    def add_report(self, id, metrics, code=RedfishHttpStatus.created):
        body = {
            "Id": id,
            "Metrics": metrics,
            "MetricReportDefinitionType": "Periodic",
            "ReportActions": ["RedfishEvent", "LogToMetricReportsCollection"],
            "Schedule": {"RecurrenceInterval": self.min_interval},
        }
        self.redfish.post(self.redfish.metric_report_definition_path, body,
                          code)

    def delete_report(self, path):
        self.redfish.delete(f'{path}')
