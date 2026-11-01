from pwn import cyclic
pat = cyclic(400)
open("inputs/pattern.fastq","wb").write(b"@p\n"+pat+b"\n+\n"+b"I"*len(pat)+b"\n")