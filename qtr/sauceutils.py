from restutils import RESTTools


class SauceTools(RESTTools):
    """Base Class for Communicating with SauceLabs"""
    def __init__(self, api_host, username, password):
        """Configures the API Tester to be user and environment specific

        :param str api_host: The base url of the API Server
        :param str username: API Username
        :param str password: API Password
        :return: None
        """
        self.API_HOST = api_host
        self.AUTH = (username, password)

    def get_jobs(self, num_jobs=100, **kwargs):
        query_url = self.API_HOST + "/rest/v1/{0}/jobs?limit={1}".format(self.AUTH[0], num_jobs)

        query_url += "".join(["&{0}={1}".format(key, val) for key, val in kwargs.items()])
        resp = self.http_get(query_url, verify=True)

        try:
            jobs = resp.json()
        except ValueError:
            print "Error retrieving all jobs.\nList All Jobs Response: {0}\nList All Jobs Response Content: {1}".format(resp, self.format_json(resp.json()))
            raise

        return jobs

    def get_job(self, job_id):
        resp = self.http_get(self.API_HOST + "/rest/v1/{0}/jobs/{1}".format(self.AUTH[0], job_id), verify=True)

        try:
            job = resp.json()
        except ValueError:
            print "Error retrieving job.\nJob Response: {0}\nJob Response Content: {1}".format(resp, self.format_json(resp.json()))
            raise

        return job

    def update_job(self, job_id, **kwargs):
        pass

    def delete_job(self, job_id):
        pass

    def stop_job(self, job_id):
        pass

    def get_job_asset_names(self, job_id):
        pass

    def get_job_asset_files(self, job_id, files):
        pass

    def get_tunnels(self):
        pass

    def get_tunnel(self, tunnel_id):
        pass