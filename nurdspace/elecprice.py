#!/usr/bin/env python3

"""
    Polls electricity price from REST service, publishes it on MQTT.
    Requires paho-mqtt and requests python packages.
"""
import argparse
from paho.mqtt import publish
import requests

def get_current_price(url):
    """ gets current electricity price in EUR per MWh """
    print(f'Polling electricity price from {url}')
    response = requests.get(url)
    json = response.json()
    price = json['current']['price']
    print(f"Price is currently EUR {price} / MWh")
    return price

def main():
    """ The main entry point """

    # you can run a local HTTP server serving files
    # from the current local directory on port 8000, using
    #   python3 -m http.server
    # then use an url parameter of 'http://localhost:8000/elecprice.json'

    parser = argparse.ArgumentParser()
    parser.add_argument("--url", help="The URL of the electricity price REST service",
                        default="http://stofradar.nl:9001/electricity/price")
    parser.add_argument("--host", help="MQTT host", default="mqtt.vm.nurd.space")
    parser.add_argument("--topic", help="MQTT topic", default="bertrik/electricity/price/current")
    args = parser.parse_args()

    price = get_current_price(args.url) / 1000.0

    payload = f"{price:.5f}"
    print(f"Publishing {payload} to topic {args.topic} on {args.host}")
    publish.single(args.topic, payload, 0, True, args.host)

if __name__ == "__main__":
    main()
