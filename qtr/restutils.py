import json
import requests
from requests import Response


class RESTTools(object):
    """Base Class for using REST style APIs"""
    def __init__(self, username, password):
        """Configures the API Tester to be user and environment specific

        :param str username: API Username
        :param str password: API Password
        :return: None
        """
        self.AUTH = (username, password)

    @staticmethod
    def print_json(jsonic, indent=4):
        """Prints the supplied JSON as a formatted string

        :param dict or list jsonic: JSON to print
        :param int indent: JSON indent size
        :rtype: None
        """
        print json.dumps(jsonic, indent=indent)

    @staticmethod
    def format_json(jsonic, indent=4):
        """Formats and returns the supplied JSON into a formatted string

        :param dict or list jsonic: JSON to format
        :param int indent: JSON indent size
        :rtype: str
        """
        return json.dumps(jsonic, indent=indent)


    def http_post(self, url, data, files=None, verify=False):
        """Performs an appropriate HTTP POST based on the data passed into the method. Returns the POST response

        :param str url: The URL to send the HTTP POST to
        :param dict or list data: JSON data to send to the server
        :param dict files: Contains a file object in a dictionary, with the name the server should use to refer to the
                           file
        :rtype: Response
        """
        resp = requests.post(url=url,
                             auth=self.AUTH,
                             data=data if files else json.dumps(data),
                             verify=verify,
                             files=files,
                             headers=None if files else {'Content-type': 'application/json'})

        return resp

    def http_get(self, url, verify=False):
        """Performs an HTTP GET based on a supplied URL. Returns the GET response

        :param str url: The URL to supply to the HTTP GET
        :rtype: Response
        """
        resp = requests.get(url=url,
                            verify=verify,
                            auth=self.AUTH)
        return resp

    def http_delete(self, url):
        """Performs an HTTP DELETE based on a supplied URL. Returns the DELETE response

        :param str url: The URL of the object you want to delete
        :rtype: Response
        """
        resp = requests.delete(url=url,
                               verify=False,
                               auth=self.AUTH)
        return resp
