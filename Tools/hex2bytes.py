"""
File: hex2bytes.py
Author: Johannes le Roux (@dadecoza)
Date: 2023-10-19

Description:
    This script accepts an intel hex file as argument and outputs the data
    in the format of a C byte array.
"""

import sys


def get_record(record):
    if not record or not record.startswith(":"):
        return None
    record = record.strip().replace(":", "")
    data = [int(record[i:i+2], 16) for i in range(0, len(record), 2)]
    number_of_bytes = data[0]
    start_address = data[1]*16*16 + data[2]
    end_address = start_address+number_of_bytes
    bytes = data[4:number_of_bytes+4]
    return {
        "start": start_address,
        "end": end_address,
        "bytes": bytes
    }


def get_records(filename):
    records = []
    try:
        with open(filename, "r") as f:
            for line in f.readlines():
                record = get_record(line)
                if record:
                    records.append(record)
    except FileNotFoundError as e:
        print("%s not found" % filename)
        sys.exit(1)
    return records


def get_start_and_end(records):
    start = 4096
    end = 0
    for record in records:
        if not len(record["bytes"]):
            continue
        if record["start"] < start:
            start = record["start"]
        if record["end"] > end:
            end = record["end"]
    return [start, end]


def get_bytes(filename):
    records = get_records(filename)
    start, end = get_start_and_end(records)
    size = int(end-start)
    print("Start address: %s" % hex(start))
    print("End address  : %s" % hex(end))
    print("Size         : %s bytes" % size)
    bytes = [0xff] * size
    for record in records:
        rstart = record["start"]-start
        for inx in range(len(record["bytes"])):
            bytes[rstart+inx] = record["bytes"][inx]
    strbytes = ', '.join('0x{:02x}'.format(a) for a in bytes)
    print("C Array      :")
    print("[%s]" % strbytes)


if __name__ == "__main__":
    args = sys.argv
    if len(args) > 1:
        message = args[1]
        get_bytes(message)
    else:
        print("Usage: %s hexfile" % args[0])
