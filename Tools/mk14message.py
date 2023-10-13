"""
File: mk14message.py
Author: Johannes le Roux (@dadecoza)
Date: 2023-10-13

Description:
    This script creates a program for displaying a custom message on the Mk14.
    It accepts a text message as argument and outputs the Mk14 program code in
    intel hex format that can be loaded into most emulators.
    The resulting program is based on the "Message" program from the
    "Mk14 Micro Computer Training Manual".
"""

import sys

code = [
    0xc4, 0xd, 0x35, 0xc4, 0x31, 0xc4, 0xf, 0x36, 0xc4, 0x73, 0x32, 0xc4,
    0x60, 0xc8, 0xf0, 0xc4, 0x7, 0x1, 0xc2, 0x80, 0xc9, 0x80, 0x8f, 0x1,
    0xc4, 0xff, 0x2, 0x70, 0x94, 0xf3, 0xb8, 0xdf, 0x9c, 0xed, 0xc6, 0xff,
    0x94, 0xe5, 0x90, 0xdd, 0x80
]

font = {
    "a": 0x77, "b": 0x7c, "c": 0x39, "d": 0x5e, "e": 0x79, "f": 0x71,
    "g": 0x6f, "h": 0x76, "i": 0x30, "j": 0x1e, "k": 0x76, "l": 0x38,
    "m": 0x15, "n": 0x54, "o": 0x3f, "p": 0x73, "q": 0x67, "r": 0x50,
    "s": 0x6d, "t": 0x78, "u": 0x3e, "v": 0x1c, "w": 0x2a, "x": 0x76,
    "y": 0x6e, "z": 0x5b, "0": 0x3f, "1": 0x6, "2": 0x5b, "3": 0x4f,
    "4": 0x66, "5": 0x6d, "6": 0x7d, "7": 0x7, "8": 0x7f, "9": 0x6f,
    " ": 0x40
}


def create_program(message):
    outstr = ""
    message += " "
    program = code.copy()
    data = []
    for letter in [lt.lower() for lt in message[::-1]]:
        if letter in font:
            data.append(font[letter])
    if (len(data) > 7):
        data += data[:7]

    program += data

    endaddr = 0xF20 + len(program)
    endhi = endaddr >> 8
    endlo = endaddr & 0xff
    program[6] = endhi
    program[9] = endlo-8

    chunks = []
    tmp = []
    for b in program:
        tmp.append(b)
        if not (len(tmp)) % 10:
            chunks.append(tmp)
            tmp = []
    if tmp:
        chunks.append(tmp)

    addr = 0xF20
    for chunk in chunks:
        length = len(chunk)
        addrhi = addr >> 8
        addrlo = addr & 0xff
        rtype = 0
        record = [length, addrhi, addrlo, rtype]
        record += chunk
        record.append((-sum(record)) & 0x0FF)
        rstr = "".join([format(b, '02X') for b in record])
        outstr += ":%s\n" % rstr
        addr += length
    outstr += ":00000001FF"
    return outstr


if __name__ == "__main__":
    args = sys.argv
    if len(args) > 1:
        message = args[1]
        print(create_program(message))
    else:
        print("Usage: %s message" % args[0])
