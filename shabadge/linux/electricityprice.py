#!/usr/bin/env python3

"""
   Polls electricity hourly prices a plots them on SHA badge epaper.
"""

import time
import argparse
import requests

class ElecPricePoller:
    """ Polls build status from HTTP, pushes a command over serial """
    def __init__(self, url, interval):
        self.url = url
        self.interval = interval

    def fetch_prices(self):
        response = requests.get(self.url)
        return response.json()

    def __process_json(self, json):
        day_ahead = json['day-ahead']
        for item in day_ahead:
            print(f'{item}')

    def run(self):
        while True:
            # get the prices from the REST service
            print(f'Polling prices from {self.url}')
            json = self.fetch_prices()

            # parse it
            self.__process_json(json)

            # sleep until next time
            time.sleep(60 * self.interval)

def main():
    """ The main entry point """
    parser = argparse.ArgumentParser()
    parser.add_argument("-u", "--url", help="Where to get the price JSON",
                        default="http://stofradar.nl:9001/electricity/price")
    parser.add_argument("-i", "--interval", help="Fetch interval (minutes)",
                        default=5)
    args = parser.parse_args()

    poller = ElecPricePoller(args.url, args.interval)
    poller.run()

if __name__ == "__main__":
    main()
