#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Validate packaged artwork dimensions, RGBA format and pane alpha/ink."""
import struct
import sys
import zipfile
import zlib

EXPECTED = {"icon-128.png":128,"icon.png":52,"icon-40.png":40,"pane.png":96}
with zipfile.ZipFile(sys.argv[1]) as archive:
    for name,size in EXPECTED.items():
        data=archive.read(".arcade/"+name)
        assert data[:8]==b"\x89PNG\r\n\x1a\n",name
        assert struct.unpack(">IIBBBBB",data[16:29])==(size,size,8,6,0,0,0),name
        off=8
        compressed=bytearray()
        while off<len(data):
            n=struct.unpack(">I",data[off:off+4])[0]
            kind=data[off+4:off+8]
            payload=data[off+8:off+8+n]
            assert zlib.crc32(kind+payload)&0xffffffff==struct.unpack(">I",data[off+8+n:off+12+n])[0]
            if kind==b"IDAT":compressed.extend(payload)
            off+=n+12
        raw=zlib.decompress(compressed)
        stride=1+size*4
        assert len(raw)==stride*size
        pixels=[]
        for y in range(size):
            assert raw[y*stride]==0
            pixels.extend(tuple(raw[i:i+4]) for i in range(y*stride+1,(y+1)*stride,4))
        assert any(p[3]==0 for p in pixels) and any(p[3]==255 for p in pixels),name
        if name=="pane.png":
            assert all(p[:3]==(255,255,255) for p in pixels),name
print("Arcade artwork package validated")
