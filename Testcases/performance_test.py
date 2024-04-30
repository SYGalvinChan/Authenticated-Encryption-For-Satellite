from komodo_py import *
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
import sys
import random

def timestamp_to_ns (stamp, samplerate_khz):
    return (stamp * 1000 // (samplerate_khz // 1000))

def print_num_array (array, data_len):
    for i in range(data_len):
        print(array[i], end=' ')

def print_status (status):
    if status == KM_OK:                 print('OK', end=' ')
    if status & KM_READ_TIMEOUT:        print('TIMEOUT', end=' ')
    if status & KM_READ_ERR_OVERFLOW:   print('OVERFLOW', end=' ')
    if status & KM_READ_END_OF_CAPTURE: print('END OF CAPTURE', end=' ')
    if status & KM_READ_CAN_ARB_LOST:   print('ARBITRATION LOST', end=' ')
    if status & KM_READ_CAN_ERR:        print('ERROR %x' % (status &
                                                KM_READ_CAN_ERR_FULL_MASK), end=' ')

def print_events (events, bitrate):
    if events == 0:
        return
    if events & KM_EVENT_DIGITAL_INPUT:
        print('GPIO CHANGE 0x%x;' % (events & KM_EVENT_DIGITAL_INPUT_MASK), end=' ')
    if events & KM_EVENT_CAN_BUS_STATE_LISTEN_ONLY:
        print('BUS STATE LISTEN ONLY;', end=' ')
    if events & KM_EVENT_CAN_BUS_STATE_CONTROL:
        print('BUS STATE CONTROL;', end=' ')
    if events & KM_EVENT_CAN_BUS_STATE_WARNING:
        print('BUS STATE WARNING;', end=' ')
    if events & KM_EVENT_CAN_BUS_STATE_ACTIVE:
        print('BUS STATE ACTIVE;', end=' ')
    if events & KM_EVENT_CAN_BUS_STATE_PASSIVE:
        print('BUS STATE PASSIVE;', end=' ')
    if events & KM_EVENT_CAN_BUS_STATE_OFF:
        print('BUS STATE OFF;', end=' ')
    if events & KM_EVENT_CAN_BUS_BITRATE:
        print('BITRATE %d kHz;' % (bitrate // 1000), end=' ')

# Parameters to play with
bitrate = 40000  

seed = 1000
key_str = "ee84e19cda87a76291eaaf2054aef812"
direction_str = "13"
key_index_str = "36"
payload_length_str = "fffd"
replay_counter_str = "f2cb949b8fb0013e"

random.seed(seed)

crypto_header_str = direction_str.zfill(2) + key_index_str.zfill(2) + payload_length_str.zfill(4) + replay_counter_str.zfill(16)
print(crypto_header_str)
payload_length_int = int(payload_length_str, 16)
payload_bytes = random.randbytes(payload_length_int)

key_bytes = bytes.fromhex(key_str)
iv_bytes = bytes.fromhex(crypto_header_str)
aad_bytes = bytes.fromhex("00000000" + crypto_header_str)

aesgcm = AESGCM(key_bytes)
output = aesgcm.encrypt(iv_bytes, payload_bytes, aad_bytes)
ciphertext_bytes = output[:-16]
tag_bytes = output[-16:]
# print(payload_bytes.hex())
# print(ciphertext_bytes.hex())
bytes_to_send = iv_bytes + ciphertext_bytes + tag_bytes
port    = 0      # Use port 0
timeout = 1000   # ms

# Open the interface
km = km_open(port)
if km <= 0:
    print('Unable to open Komodo on port %d' % port)
    print('Error code = %d' % km)
    sys.exit(1)

ret = km_acquire(km, KM_FEATURE_CAN_A_CONFIG  |
                    KM_FEATURE_CAN_A_LISTEN  |
                    KM_FEATURE_CAN_A_CONTROL |
                    KM_FEATURE_GPIO_CONFIG   |
                    KM_FEATURE_GPIO_LISTEN)
print('Acquired features 0x%x' % ret)

# Set bitrate
ret = km_can_bitrate(km, KM_CAN_CH_A, bitrate)
print('Bitrate set to %d kHz' % (ret // 1000))

# Set timeout
km_timeout(km, timeout)
print('Timeout set to %d ms' % timeout)

# Set target power
km_can_target_power(km, KM_CAN_CH_A, KM_TARGET_POWER_OFF)
print('Target power OFF')

# Configure all GPIO pins as inputs
for i in range (8):
    km_gpio_config_in(km, i, KM_PIN_BIAS_PULLUP, KM_PIN_TRIGGER_BOTH_EDGES)
print('All pins set as inputs')

# Enable Komodo
ret = km_enable(km)
if ret != KM_OK:
    print('Unable to enable Komodo')
    sys.exit(1)

data_list = []
for byte in bytes_to_send:
    data_list.append(byte)
    if len(data_list) == 8:
        pkt       = km_can_packet_t()
        pkt.dlc   = len(data_list)
        pkt.id    = 1
        data      = array('B', data_list)
        # print(data)
        status = km_can_write(km, KM_CAN_CH_A, 0, pkt, data)
        # print(status)
        data_list = []
    
if len(data_list) > 0:
        pkt       = km_can_packet_t()
        pkt.dlc   = len(data_list)
        pkt.id    = 1
        data      = array('B', data_list)
        status = km_can_write(km, KM_CAN_CH_A, 1, pkt, data)
        # print(status)
        data_list = []
print(tag_bytes.hex())
count = 0
samplerate_khz = km_get_samplerate(km) // 1000
while (1):
    (ret, info, pkt, data) = km_can_read(km, data)
    print('%d,%d,(' % (count, timestamp_to_ns(info.timestamp,
                                                  samplerate_khz)), end=' ')
    if ret < 0:
        print('error=%d)' % ret)
        continue

    print_status(info.status)
    print_events(info.events, info.bitrate_hz)

    # Continue printing if we didn't see timeout, error or dataless events
    if ((info.status == KM_OK) and not info.events):
        print('),<%x:%s' % (pkt.id,
                            'rtr>' if pkt.remote_req else 'data>'), end=' ')
        # If packet contained data, print it
        if not pkt.remote_req:
            print_num_array(data, ret)
        
        # status = km_can_write(km, KM_CAN_CH_A, 1, pkt, data)
        # print()
        # print(f'Status: {status} Sent data: ', end=' ')
        # print_num_array(data, ret)
    else:
        print(')', end=' ')

    print()
    sys.stdout.flush()
    count += 1