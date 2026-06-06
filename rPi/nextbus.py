#!/usr/bin/env python3
"""Polls the WMATA bus API and drives a two-tube nixie display via GPIO."""

import time
from datetime import datetime
import requests
import RPi.GPIO as GPIO

# --- Configuration -----------------------------------------------------------
WMATA_API_KEY        = '6ee4fbf9b52d4b21ac20ccd77fe1bac8'
WMATA_STOP_ID        = '1002006'
WMATA_ENDPOINT       = 'https://api.wmata.com/NextBusService.svc/json/jPredictions'

POLL_INTERVAL        = 5    # seconds between API polls
CATHODE_EX_INTERVAL  = 600  # seconds between cathode exercise cycles (10 min)

MIN_MINUTES          = 3    # ignore buses arriving sooner than this
PERIOD_WINDOW        = 15   # right period lights when another bus is within this many minutes

QUIET_START          = 1    # hour (24h) to blank display
QUIET_END            = 7    # hour (24h) to resume display

# --- GPIO --------------------------------------------------------------------
CLK_PIN  = 17
DATA_PIN = 27
NUM_BITS = 22
BIT_DELAY = 0.000010  # 10 µs half-period

CMD_CATHODE_EXERCISE = 0x155555  # alternating bits; can't appear on a floating/pulled-up bus

# Bit mapping (MSB first):
#   bit 21 = Rp   bit 20 = R9  ...  bit 11 = R0
#   bit 10 = Lp   bit  9 = L9  ...  bit  0 = L0
_RIGHT_PERIOD_BIT = 21
_LEFT_PERIOD_BIT  = 10


def number_to_code(n, right_period=False, left_period=False):
    """Convert 0-99 to a 22-bit nixie code.

    Tens digit -> left tube  (L0-L9, bits 0-9).
    Units digit -> right tube (R0-R9, bits 11-20).
    Single-digit numbers leave the left tube dark.
    """
    if not 0 <= n <= 99:
        raise ValueError(f"Number must be 0-99, got {n}")
    right_digit = n % 10
    left_digit  = n // 10
    code = 1 << (11 + right_digit)
    if left_digit:
        code |= 1 << left_digit
    if right_period:
        code |= 1 << _RIGHT_PERIOD_BIT
    if left_period:
        code |= 1 << _LEFT_PERIOD_BIT
    return code


def send_code(code):
    for i in range(NUM_BITS - 1, -1, -1):  # MSB first
        bit = (code >> i) & 1
        GPIO.output(DATA_PIN, bit)
        time.sleep(BIT_DELAY)
        GPIO.output(CLK_PIN, GPIO.HIGH)
        time.sleep(BIT_DELAY)
        GPIO.output(CLK_PIN, GPIO.LOW)
        time.sleep(BIT_DELAY)


# --- WMATA -------------------------------------------------------------------
def fetch_predictions():
    resp = requests.get(
        WMATA_ENDPOINT,
        params={'StopID': WMATA_STOP_ID},
        headers={'api_key': WMATA_API_KEY},
        timeout=10,
    )
    resp.raise_for_status()
    return resp.json().get('Predictions', [])


# --- Main loop ---------------------------------------------------------------
def main():
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(DATA_PIN, GPIO.OUT, initial=GPIO.LOW)  # DATA before CLK so any CLK glitch samples 0
    GPIO.setup(CLK_PIN,  GPIO.OUT, initial=GPIO.LOW)
    time.sleep(0.1)  # let lines settle before clocking anything into the STM8
    send_code(0)     # flush any partial frame accumulated during GPIO init
    time.sleep(0.1)
    send_code(0)     # second flush for safety

    last_cathode_ex = time.time()  # first exercise fires after CATHODE_EX_INTERVAL

    try:
        while True:
            now = time.time()

            # Cathode exercise every CATHODE_EX_INTERVAL seconds
            if now - last_cathode_ex >= CATHODE_EX_INTERVAL:
                print('Running cathode exercise')
                send_code(CMD_CATHODE_EXERCISE)
                last_cathode_ex = now
                time.sleep(2)
                continue

            # Blank display during quiet hours
            if QUIET_START <= datetime.now().hour < QUIET_END:
                send_code(0)
                time.sleep(POLL_INTERVAL)
                continue

            # Poll WMATA
            try:
                predictions = fetch_predictions()
            except Exception as e:
                print(f'API error: {e}')
                time.sleep(POLL_INTERVAL)
                continue

            next_bus = next((p for p in predictions if p['Minutes'] >= MIN_MINUTES), None)

            if next_bus is None:
                print('No bus arriving in >= 3 minutes')
                time.sleep(POLL_INTERVAL)
                continue

            minutes = min(next_bus['Minutes'], 99)
            route   = next_bus['RouteID']

            following_bus = next(
                (p for p in predictions
                 if p is not next_bus and p['Minutes'] <= PERIOD_WINDOW),
                None
            )

            right_period = next_bus['Minutes'] < PERIOD_WINDOW and following_bus is not None
            send_code(number_to_code(minutes, right_period=right_period, left_period=True))
            time.sleep(0.25)
            send_code(number_to_code(minutes, right_period=right_period, left_period=False))

            status = f'Next bus ({route}): {minutes} min'
            if following_bus:
                status += f'  |  Following bus ({following_bus["RouteID"]}): {following_bus["Minutes"]} min'
            print(status)

            time.sleep(POLL_INTERVAL)

    except KeyboardInterrupt:
        print('\nStopped.')
    finally:
        GPIO.cleanup()


if __name__ == '__main__':
    main()
