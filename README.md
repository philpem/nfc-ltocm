# LTO Cartridge Memory (LTO-CM / MAM) reader for libnfc

Author: Phil Pemberton <philpem@philpem.me.uk>


## Usage

  - Place an LTO tape on your NFC reader, with the CM chip (on the label side, opposite the write-protect tab) centred over the aerial.
  - Run `nfc-ltocm`.
  - Feed the output to [LTO-CM-Analyzer](https://github.com/Kevin-Nakamoto/LTO-CM-Analyzer)
  - Enjoy.


## Credits

This code is based on the `libnfc-mfsetuid` example included with [libnfc](https://github.com/nfc-tools/libnfc/).

LTO-CM is specified in Annex F of [ECMA-319](https://www.ecma-international.org/publications/files/ECMA-ST/ECMA-319.pdf)


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

