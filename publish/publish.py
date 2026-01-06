#!/usr/bin/env python3
"""
Interactive MQTT publisher for the ESP32 scoreboard firmware.

This script runs in interactive mode only and provides the following commands:
  i  - init (reset names to defaults and scores to 0)
  r  - reset (set scores to 0)
  h  - home goal (increment home score, event=GOAL_HOME)
  a  - away goal (increment away score, event=GOAL_AWAY)
  H  - home adjust (enter +n/-n or absolute number)
  A  - away adjust (enter +n/-n or absolute number)
  n  - set home name (in memory)
  N  - set away name (in memory)
  s  - show current state
  q  - quit

Sends JSON messages compatible with the device's expected format:
  {"homeName":"..","awayName":"..","homeScore":"..","awayScore":"..","event":"..."}

Defaults are loaded from `config.py` when available.
Requires: paho-mqtt (pip install paho-mqtt)
"""

from __future__ import annotations
import argparse
import json
import sys
import time
import logging
from typing import Tuple

# Configure simple logging for the CLI tool
logging.basicConfig(level=logging.INFO,
                    format='%(asctime)s %(levelname)s: %(message)s')

try:
    import paho.mqtt.client as mqtt
    # Suppress the paho-mqtt Callback API v1 deprecation warning when present.
    # We still use the newer callback signatures, but some installations may emit the warning.
    import warnings
    # Ignore the specific callback API deprecation and DeprecationWarnings generally for paho internals
    warnings.filterwarnings(
        "ignore", message="Callback API version 1 is deprecated.*", category=DeprecationWarning)
    warnings.filterwarnings(
        "ignore", message="Callback API version 1 is deprecated.*", category=UserWarning)
    warnings.filterwarnings("ignore", category=DeprecationWarning)
except ImportError:  # pragma: no cover - nice message for users
    print("Error: paho-mqtt is required. Install with: pip install paho-mqtt", file=sys.stderr)
    sys.exit(1)

# Require project config to be present and complete (strict mode)
try:
    import config
except Exception:
    print("Error: required file 'config.py' not found. Create it with MQTT_BROKER, MQTT_PORT, MQTT_QOS, MQTT_RETAIN, MQTT_CLIENT_ID, MQTT_TOPIC.", file=sys.stderr)
    sys.exit(1)

# Ensure required config variables exist
_required = ['MQTT_BROKER', 'MQTT_PORT', 'MQTT_QOS',
             'MQTT_RETAIN', 'MQTT_CLIENT_ID', 'MQTT_TOPIC']
_missing = [name for name in _required if not hasattr(config, name)]
if _missing:
    print(
        f"Error: 'config.py' is missing required variables: {', '.join(_missing)}", file=sys.stderr)
    sys.exit(1)

DEFAULT_BROKER = config.MQTT_BROKER
DEFAULT_PORT = config.MQTT_PORT
DEFAULT_QOS = config.MQTT_QOS
DEFAULT_RETAIN = config.MQTT_RETAIN
DEFAULT_CLIENT_ID = config.MQTT_CLIENT_ID
DEFAULT_TOPIC = config.MQTT_TOPIC
# Optional credentials (set to None if not used)
DEFAULT_USERNAME = getattr(config, 'MQTT_USERNAME', None)
DEFAULT_PASSWORD = getattr(config, 'MQTT_PASSWORD', None)


def make_client(client_id: str | None = None, username: str | None = None, password: str | None = None) -> mqtt.Client:
    # Prefer constructing the client with the newer callback API when supported
    try:
        client = mqtt.Client(client_id=client_id, protocol=mqtt.MQTTv311,
                             callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
    except Exception as exc:
        # Could be TypeError (arg not supported) or RuntimeError (unsupported version)
        logging.debug(
            f"Could not initialize Client with callback_api_version=2: {exc}; falling back to default constructor")
        client = mqtt.Client(client_id=client_id)

    # small structure to track pending publishes so we can log details on on_publish
    client._pending_publishes = {}

    # set username/password if provided
    if username is not None:
        # password may be None; paho accepts that
        client.username_pw_set(username, password)

    # Updated callbacks compatible with paho-mqtt's newer callback API
    def on_connect(client, userdata, flags, rc, properties=None):
        # rc may be an int (v3) or a ReasonCode/enum (v5). Try to coerce to int when possible.
        try:
            code = int(rc)
        except Exception:
            code = rc
        if code == 0:
            logging.info("Connected to broker")
        else:
            logging.error(f"Connection failed with code {code}")

    def on_publish(client, userdata, mid, rc, properties=None):
        # Look up the pending publish details if available and log them
        info = client._pending_publishes.pop(mid, None)
        if info is None:
            logging.info(f"Published (mid={mid})")
            return
        topic, event, payload = info
        logging.info(
            f"Published mid={mid} topic={topic} event={event} payload={payload}")

    # Attach callbacks after we've selected the callback API version
    client.on_connect = on_connect
    client.on_publish = on_publish
    client.tls_set('./isrgrootx1.pem')
    return client


def clamp_score(v: int) -> int:
    return max(0, v)


def build_payload(home_name: str, away_name: str, home_score: int, away_score: int, event: str) -> str:
    payload = {
        "homeName": str(home_name),
        "awayName": str(away_name),
        "homeScore": str(home_score),
        "awayScore": str(away_score),
        "event": event,
    }
    return json.dumps(payload)


def parse_adjust_input(prompt: str) -> Tuple[bool, int]:
    """Returns (is_relative, value) where is_relative True for +n/-n, False for absolute assignment."""
    s = input(prompt).strip()
    if not s:
        raise ValueError('empty input')
    if s[0] in ('+', '-'):
        return True, int(s)
    return False, int(s)


def interactive_mode(broker, port, topic, qos, retain, client_id=None):
    client = make_client(client_id, DEFAULT_USERNAME, DEFAULT_PASSWORD)
    client.connect(broker, port)
    client.loop_start()

    home_name = 'HOME'
    away_name = 'AWAY'
    home_score = 0
    away_score = 0

    def publish_state(event: str):
        payload = build_payload(home_name, away_name,
                                home_score, away_score, event)
        result = client.publish(topic, payload=payload, qos=qos, retain=retain)
        # If MQTT reports immediate failure, log it
        try:
            if getattr(result, 'rc', 0) != mqtt.MQTT_ERR_SUCCESS:
                logging.error(
                    f"Immediate publish error rc={result.rc} for event={event} payload={payload}")
        except Exception:
            pass
        # record pending publish so on_publish can log details when it completes
        try:
            client._pending_publishes[result.mid] = (topic, event, payload)
        except Exception:
            logging.debug("Could not record pending publish mid")
        # tiny delay to let callbacks/transport run
        time.sleep(0.05)

    menu = (
        "Available commands:\n"
        "  i  - init (reset names to defaults and scores to 0)\n"
        "  r  - reset (set scores to 0)\n"
        "  h  - home goal (increment home score)\n"
        "  a  - away goal (increment away score)\n"
        "  H  - home adjust (+n/-n or absolute)\n"
        "  A  - away adjust (+n/-n or absolute)\n"
        "  n  - set home name\n"
        "  N  - set away name\n"
        "  s  - show current state\n"
        "  q  - quit\n"
    )

    print(menu)
    try:
        while True:
            try:
                cmd = input('command> ').strip()
            except EOFError:
                print('\nEOF received, exiting')
                break

            if not cmd:
                continue

            if cmd == 'q':
                print('Exiting')
                break

            if cmd == 's':
                print(
                    f"State -> {home_name}: {home_score}  |  {away_name}: {away_score}")
                continue

            if cmd == 'i':
                home_score = 0
                away_score = 0
                publish_state('INIT')
                print('Initialized names/scores')
                continue

            if cmd == 'r':
                home_score = 0
                away_score = 0
                publish_state('RESET')
                print('Scores reset to 0')
                continue

            if cmd == 'h':
                home_score = clamp_score(home_score + 1)
                publish_state('GOAL_HOME')
                print(f'Home goal -> {home_name}: {home_score}')
                continue

            if cmd == 'a':
                away_score = clamp_score(away_score + 1)
                publish_state('GOAL_AWAY')
                print(f'Away goal -> {away_name}: {away_score}')
                continue

            if cmd == 'H':
                try:
                    is_rel, val = parse_adjust_input(
                        'Home adjust (+n/-n or absolute number): ')
                except Exception as exc:
                    print('Invalid input:', exc)
                    continue
                if is_rel:
                    home_score = clamp_score(home_score + val)
                else:
                    home_score = clamp_score(val)
                publish_state('ADJUST_HOME')
                print(f'Home adjusted -> {home_name}: {home_score}')
                continue

            if cmd == 'A':
                try:
                    is_rel, val = parse_adjust_input(
                        'Away adjust (+n/-n or absolute number): ')
                except Exception as exc:
                    print('Invalid input:', exc)
                    continue
                if is_rel:
                    away_score = clamp_score(away_score + val)
                else:
                    away_score = clamp_score(val)
                publish_state('ADJUST_AWAY')
                print(f'Away adjusted -> {away_name}: {away_score}')
                continue

            if cmd == 'n':
                try:
                    nm = input('Enter home name: ').strip()
                except EOFError:
                    print('Aborted')
                    continue
                if nm:
                    home_name = nm
                    publish_state('SET_HOME_NAME')
                    print(f'Home name set to: {home_name}')
                else:
                    print('Empty name, ignored')
                continue

            if cmd == 'N':
                try:
                    nm = input('Enter away name: ').strip()
                except EOFError:
                    print('Aborted')
                    continue
                if nm:
                    away_name = nm
                    publish_state('SET_AWAY_NAME')
                    print(f'Away name set to: {away_name}')
                else:
                    print('Empty name, ignored')
                continue

            print('Unknown command; type i/r/h/a/H/A/n/N/s/q')

    except KeyboardInterrupt:
        print('\nInterrupted, disconnecting')
    finally:
        client.loop_stop()
        client.disconnect()


def parse_args(argv):
    p = argparse.ArgumentParser(
        description='Interactive MQTT publisher for the scoreboard (interactive-only)')
    p.add_argument('-b', '--broker', default=DEFAULT_BROKER,
                   help='Broker hostname or IP')
    p.add_argument('-p', '--port', type=int,
                   default=DEFAULT_PORT, help='Broker port')
    p.add_argument('-t', '--topic', default=DEFAULT_TOPIC,
                   help='MQTT topic to publish to')
    p.add_argument('-q', '--qos', type=int, choices=(0, 1, 2),
                   default=DEFAULT_QOS, help='MQTT QoS level')
    p.add_argument('--retain', action='store_true', default=DEFAULT_RETAIN,
                   help='Set retain flag on published messages')
    p.add_argument('--client-id', default=DEFAULT_CLIENT_ID,
                   help='Optional MQTT client id')
    return p.parse_args(argv)


def main(argv=None):
    args = parse_args(argv)
    print(
        f"Connecting to {args.broker}:{args.port} topic={args.topic} qos={args.qos} retain={args.retain}")
    interactive_mode(args.broker, args.port, args.topic,
                     args.qos, args.retain, args.client_id)


if __name__ == '__main__':
    main()
