#ifndef NFC_LTOCM_H__
#define NFC_LTOCM_H__

bool ltocm_req_std(uint8_t *ltoStandard);
bool ltocm_req_serial(uint8_t *serialNum, int *serialNumLen);
bool ltocm_select(uint8_t *serialNum, uint8_t *retSelect, int *retLenSelect);
bool ltocm_readblk(size_t block, uint8_t *retReadBlk, int *retLenReadBlk);
bool ltocm_readblk_ext(size_t block, uint8_t *retReadBlk, int *retLenReadBlk);
bool ltocm_readblkcnt(uint8_t *retReadBlk, int *retLenReadBlk);
#endif
