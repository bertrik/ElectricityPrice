#!/usr/bin/env python3

"""
    Polls gas price from REST service, publishes it on MQTT.
    Requires paho-mqtt and requests python packages.

    You can make this a cronjob by editing the crontab using "crontab -e" and adding (for example):
    */5 * * * *
"""
import argparse
from paho.mqtt import publish
import requests

def get_current_price(url):
    """ gets current gas price in EUR per MWh """
    print(f'Polling gas price from {url}')
    response = requests.get(url)
    json = response.json()
    price = json['day-ahead'][0]['price']
    print(f"Price is currently EUR {price} / MWh")
    return price

def main():
    """ The main entry point """

    # you can run a local HTTP server serving files
    # from the current local directory on port 8000, using
    #   python3 -m http.server
    # then use an url parameter of 'http://localhost:8000/gasprice.json'

    parser = argparse.ArgumentParser()
    parser.add_argument("--url", help="The URL of the natural gas price REST service",
                        default="http://stofradar.nl:9001/naturalgas/price")
    parser.add_argument("--host", help="MQTT host", default="mqtt.vm.nurd.space")
    parser.add_argument("--topic", help="MQTT topic", default="bertrik/naturalgas/price/current")
    args = parser.parse_args()

    price_per_m3 = get_current_price(args.url) * 35.17 / 3600;

    payload = f"{price_per_m3:.5f}"
    print(f"Publishing {payload} to topic {args.topic} on {args.host}")
    publish.single(args.topic, payload, 0, True, args.host)

if __name__ == "__main__":
    main()
