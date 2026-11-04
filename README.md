# LTO Cartridge Memory (LTO-CM / MAM) reader for libnfc

Author: Phil Pemberton <philpem@philpem.me.uk>


## Usage

  - Place an LTO tape on your NFC reader, with the CM chip (on the label side, opposite the write-protect tab) centred over the aerial.
  - Run `nfc-ltocm`.
  - Feed the output to [LTO-CM-Analyzer](https://github.com/Kevin-Nakamoto/LTO-CM-Analyzer)
  - Enjoy.


## Hints on antenna/LTO placement

The ACR122U (Touchatag) reader can read LTO-CM chips quite reliably, if slowly. Place the LTO-CM chip over the centre of the Touchatag (or NFC) logo.

The SCL3711 also works, but antenna placement is more critical. Place the tape flat down, with the rear-right corner over the NFC reader's antenna. Support the front-left corner to stop the tape from slipping.

The LTO-CM chip is located at the back of the cartridge, behind the right side of the label. This is on the opposite side of the label from the write protect tab.


## Limitations

To reduce the potential for abuse by unscrupulous dealers of "refurbished" media, `nfc-ltocm` can only read from the LTO-CM chip. Write support is not present, nor is it likely to be added.

The chip size is taken from Block 0, byte 5, which declares the memory capacity in units of 1 KiB (ECMA-319 Annex D, D.2.1). The memory type reported by REQUEST STANDARD is only used as a fallback if that value looks implausible, because it is commonly one block short of the real capacity.

If the final block of the declared range cannot be read, the tool reports it and keeps the rest of the dump. On the LTO-6 cartridge tested, blocks 0-510 read normally and block 511 is not exposed, so a 16 KiB chip yields 511 blocks. Note that `READ BLOCK` only addresses blocks 0-254; the tool uses `READ BLOCK EXT` from block 255 upwards.

LTO-CM memories of type 3 (16 KiB) are supported and have been tested on an LTO-6 cartridge.

LTO-CM memories of type 4 or later are not specified in ECMA-319; the tool will read up to the declared capacity, with a sanity limit of 32 KiB (1024 blocks). If you have a datasheet or specification for a later revision of LTO-CM memory chip, please contact me on the email address above. If you've successfully tested Type 3 or later memory device support, please open a PR.


## Credits

This code is based on the `libnfc-mfsetuid` example included with [libnfc](https://github.com/nfc-tools/libnfc/).

LTO-CM is specified in Annexes D and F of [ECMA-319](https://www.ecma-international.org/publications/files/ECMA-ST/ECMA-319.pdf)


## Licence

This is the same licence as applies to the `libnfc` examples.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 1) Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.

 2) Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

