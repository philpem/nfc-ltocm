/***
 * nfc-ltocm: Read LTO Cartridge Memory with libnfc
 *
 * Phil Pemberton <philpem@philpem.me.uk>
 * github.com/philpem
 * www.philpem.me.uk
 *
 * Licence: 2-clause BSD, see README.md
 *
 * References:
 *   ECMA-319: https://www.ecma-international.org/publications/files/ECMA-ST/ECMA-319.pdf
 */
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <nfc/nfc.h>

#include "nfc-ltocm.h"
#include "nfc-utils.h"


/// Set to 'false' for more debugging (print raw packets)
const bool quiet_output = true;

#define MAX_FRAME_LEN 264

/// Receive buffer
static uint8_t abtRx[MAX_FRAME_LEN];
/// Number of received bits
static int szRxBits;
/// Number of received bytes
static int szRxBytes;

/// NFC device handle
static nfc_device *pnd;


/***
 * LTO-CM commands
 ***/
/// LTO-CM REQUEST STANDARD: returns 2 bytes (D0:D1 = Block 0 Bytes 6:7)
const uint8_t LTOCM_REQUEST_STANDARD[]		= { 0x45 };

/// LTO-CM REQUEST SERIAL NUMBER: returns 5 byte serial number
const uint8_t LTOCM_REQUEST_SERIAL_NUM[]	= { 0x93, 0x20 };

/// LTO-CM SELECT: zeroes are 5 serial number bytes plus 2-byte checksum. Responds with ACK.
const uint8_t LTOCM_SELECT[]				= { 0x93, 0x70, 0, 0, 0, 0, 0, 0, 0 };

/// LTO-CM READ BLOCK: zeroes are block address and 2-byte checksum.
const uint8_t LTOCM_READ_BLOCK[]			= { 0x30, 0, 0, 0 };

/// LTO-CM READ BLOCK: zeroes are 2 bytes for block address and 2-byte checksum.
const uint8_t LTOCM_READ_BLOCK_EXT[]			= { 0x21, 0, 0, 0, 0 };

/// LTO-CM READ BLOCK CONTINUE
const uint8_t LTOCM_READ_BLOCK_CONTINUE[]	= { 0x80 };

/// ACK response
const uint8_t LTOCM_ACK = 0x0A;

/// NACK response
const uint8_t LTOCM_NACK = 0x05;


/***
 * Utility functions
 ***/

/**
 * Transmit bits over NFC and read the response.
 *
 * This is generally used for the 7-bit commands in the INIT state.
 *
 * Copied from the nfc_mfsetuid demo in the libnfc source package.
 *
 * @param	pbtTx		Bits to transmit.
 * @param	szTxBits	Number of bits to transmit.
 */
static bool transmit_bits(const uint8_t *pbtTx, const size_t szTxBits)
{
	// Show transmitted command
	if (!quiet_output) {
		printf("Sent bits:     ");
		print_hex_bits(pbtTx, szTxBits);
	}
	// Transmit the bit frame command, we don't use the arbitrary parity feature
	if ((szRxBits = nfc_initiator_transceive_bits(pnd, pbtTx, szTxBits, NULL, abtRx, sizeof(abtRx), NULL)) < 0)
		return false;

	// Show received answer
	if (!quiet_output) {
		printf("Received bits: ");
		print_hex_bits(abtRx, szRxBits);
	}
	// Succesful transfer
	return true;
}


/**
 * Transmit bytes over NFC and read the response.
 *
 * This is used for commands and data packets in the PRESELECT state.
 *
 * Copied from the nfc_mfsetuid demo in the libnfc source package.
 *
 * @param	pbtTx		Bits to transmit.
 * @param	szTx		Number of bytes to transmit.
 * @note Returned data is in abtRx.
 */
static bool transmit_bytes(const uint8_t *pbtTx, const size_t szTx)
{
	// Show transmitted command
	if (!quiet_output) {
		printf("Sent bits:     ");
		print_hex(pbtTx, szTx);
	}

	// Transmit the command bytes
	if ((szRxBytes = nfc_initiator_transceive_bytes(pnd, pbtTx, szTx, abtRx, sizeof(abtRx), 0)) < 0)
		return false;

	// Show received answer
	if (!quiet_output) {
		printf("Received bits: ");
		print_hex(abtRx, szRxBytes);
	}

	// Succesful transfer
	return true;
}

bool ltocm_req_std(uint8_t *ltoStandard)
{
	if (!transmit_bits(LTOCM_REQUEST_STANDARD, 7))
		return false;

	memcpy(ltoStandard, abtRx, 2);
	return true;

}

bool ltocm_req_serial(uint8_t *serialNum, int *serialNumLen)
{
	if (!transmit_bytes(LTOCM_REQUEST_SERIAL_NUM, 2))
		return false;

	memcpy(serialNum, abtRx, 5);
	*serialNumLen = szRxBytes;
	return true;

}

bool ltocm_select(uint8_t *serialNum, uint8_t *retSelect, int *retLenSelect)
{
	uint8_t selectCmd[sizeof(LTOCM_SELECT)];
	memcpy(selectCmd, LTOCM_SELECT, sizeof(LTOCM_SELECT));
	memcpy(&selectCmd[2], &serialNum[0], 5);

        iso14443a_crc_append(selectCmd, 7);

	if (!transmit_bytes(selectCmd, sizeof(LTOCM_SELECT)))
		return false;

	*retSelect = abtRx[0];
	*retLenSelect = szRxBytes;
	return true;

}

bool ltocm_readblk(size_t block, uint8_t *retReadBlk, int *retLenReadBlk)
{
	uint8_t readBlockCmd[sizeof(LTOCM_READ_BLOCK)];
	memcpy(readBlockCmd, LTOCM_READ_BLOCK, sizeof(LTOCM_READ_BLOCK));
	readBlockCmd[1] = block;

	iso14443a_crc_append(readBlockCmd, 2);
	
	if (!transmit_bytes(readBlockCmd, sizeof(readBlockCmd)))
		return false;

	memcpy(retReadBlk, abtRx, 18);
	*retLenReadBlk = szRxBytes;
	return true;

}

bool ltocm_readblk_ext(size_t block, uint8_t *retReadBlk, int *retLenReadBlk)
{
	uint8_t readBlockCmd[sizeof(LTOCM_READ_BLOCK_EXT)];
	memcpy(readBlockCmd, LTOCM_READ_BLOCK_EXT, sizeof(LTOCM_READ_BLOCK_EXT));
	readBlockCmd[1] = block & 0xff;
	readBlockCmd[2] = (block >>8) & 0xff;

	iso14443a_crc_append(readBlockCmd, 3);
	
	if (!transmit_bytes(readBlockCmd, sizeof(readBlockCmd)))
		return false;

	memcpy(retReadBlk, abtRx, 18);
	*retLenReadBlk = szRxBytes;
	return true;

}

bool ltocm_readblkcnt(uint8_t *retReadBlk, int *retLenReadBlk)
{
	if (!transmit_bytes(LTOCM_READ_BLOCK_CONTINUE, sizeof(LTOCM_READ_BLOCK_CONTINUE)))
		return false;

	memcpy(retReadBlk, abtRx, 18);
	*retLenReadBlk = szRxBytes;
	return true;

}

int main(int argc, char **argv)
{
	int returncode = EXIT_SUCCESS;

	// Initialise libnfc
	nfc_context *context;
	nfc_init(&context);
	if (context == NULL) {
		ERR("Unable to init libnfc (malloc)");
		exit(EXIT_FAILURE);
	}

	// Try to open the NFC reader
	pnd = nfc_open(context, NULL);

	if (pnd == NULL) {
		ERR("Error opening NFC reader");
		nfc_exit(context);
		exit(EXIT_FAILURE);
	}

	// Initialise NFC device as "initiator"
	if (nfc_initiator_init(pnd) < 0) {
		nfc_perror(pnd, "nfc_initiator_init");
		returncode = EXIT_FAILURE;
		goto err_exit;
	}

	// Configure the CRC
	if (nfc_device_set_property_bool(pnd, NP_HANDLE_CRC, false) < 0) {
		nfc_perror(pnd, "nfc_device_set_property_bool");
		returncode = EXIT_FAILURE;
		goto err_exit;
	}
	// Use raw send/receive methods
	if (nfc_device_set_property_bool(pnd, NP_EASY_FRAMING, false) < 0) {
		nfc_perror(pnd, "nfc_device_set_property_bool");
		returncode = EXIT_FAILURE;
		goto err_exit;
	}
	// Disable 14443-4 autoswitching
	if (nfc_device_set_property_bool(pnd, NP_AUTO_ISO14443_4, false) < 0) {
		nfc_perror(pnd, "nfc_device_set_property_bool");
		returncode = EXIT_FAILURE;
		goto err_exit;
	}

	printf("NFC reader: %s opened\n", nfc_device_get_name(pnd));


	// Send LTO-CM REQUEST STANDARD
	//   (LTO-CM state transition INIT -> PRESELECT)
	uint8_t ltoStandard[2];
	if (!ltocm_req_std(&ltoStandard[0])) {
		printf("Error: error with LTOCM REQUEST STANDARD, no tag present?\n");
		returncode = EXIT_FAILURE;
		goto err_exit;
	}
	printf("LTO REQUEST STANDARD: %02X %02X\n", ltoStandard[0], ltoStandard[1]);

	size_t numLTOCMBlocks = 0;

	/* According to the Proxmark 3 LTO-CM code (client/src/cmdhflto.c), the
	 * memory sizes are:
	 *   LTO type info 00,01: 101 blocks  -- wrong, 127
	 *   LTO type info 00,02:  95 blocks  -- wrong, 255
	 *   LTO type info 00,03: 255 blocks
	 *
	 * This seems to be incorrect. The LTO chip size is stored in Block 0.
	 * See ECMA-319 Annex D, D.2.1 "LTO-CM Manufacturer's Information"
	 *
	 * I have a type=2 chip (on a Sony LTO4 cartridge from 2015) which declares
	 * 8*1024 bytes capacity in Block 0, and has 255 readable blocks.
	 *
	 * A HP cleaning catridge with memory type=1 declares 4*1024 bytes capacity
	 * and has 127 readable blocks.
	 */

	// Validate LTO-CM REQUEST STANDARD response
	uint16_t ltoCMStandard = ((uint16_t)ltoStandard[0] << 8) | ((uint16_t)ltoStandard[1]);
	switch (ltoCMStandard) {
		case 0x0001:
			numLTOCMBlocks = 127;
			break;
		case 0x0002:
			numLTOCMBlocks = 255;
			break;
		case 0x0003:
			numLTOCMBlocks = 511;
			break;
		default:
			printf("Error: unknown LTO-CM memory type %04X\n", ltoCMStandard);
			returncode = EXIT_FAILURE;
			goto err_exit;
	}


	// Send LTO-CM REQUEST SERIAL NUMBER
	//   (LTO-CM state PRESELECT -> PRESELECT)
	uint8_t serialNum[5];
	int serialNumLen = 0;
	if (!ltocm_req_serial(&serialNum[0], &serialNumLen)) {
		printf("Error: error with REQUEST SERIAL NUMBER command.\n");
		returncode = EXIT_FAILURE;
		goto err_exit;
	}

	if (serialNumLen < 5) {
		printf("Error: REQUEST SERIAL NUMBER returned too few bytes.\n");
		returncode = EXIT_FAILURE;
		goto err_exit;
	}
	printf("Found LTO-CM tag with s/n %02X:%02X:%02X:%02X:%02X\n",
			serialNum[0], serialNum[1], serialNum[2], serialNum[3], serialNum[4]);
	char default_filename[13];
	sprintf(default_filename, "%02X%02X%02X%02X.bin", serialNum[0], serialNum[1], serialNum[2], serialNum[3]);

	// Check the serial number's validity
	uint8_t ltosnCheck = serialNum[0] ^ serialNum[1] ^ serialNum[2] ^ serialNum[3];
	if (ltosnCheck != serialNum[4]) {
		printf("Error: REQUEST SERIAL NUMBER returned an invalid serial number.\n");
		returncode = EXIT_FAILURE;
		goto err_exit;
	}

	// Send LTO-CM SELECT to Select the chip we just found
	//   (LTO-CM state PRESELECT -> COMMAND)
	uint8_t retSelect;
	int retLenSelect;
	if (!ltocm_select(&serialNum[0], &retSelect, &retLenSelect)) {
		printf("Error: error with SELECT command\n");
		returncode = EXIT_FAILURE;
		goto err_exit;
	}

	// Check that the LTO-CM chip sent us an acknowledgement
	if ((retLenSelect != 1) || (retSelect != LTOCM_ACK)) {
		printf("Error: Failed to SELECT the LTO-CM chip\n");
		returncode = EXIT_FAILURE;
		goto err_exit;
	}

	// Chip is now in the LTO-CM COMMAND state, we should be able to read it

	// Read all blocks in the chip
	printf("Reading LTO-CM data to file\n");

	char *p_filename;
	if (argc == 1) {
		p_filename = &default_filename[0];
	} else {
		p_filename = argv[1];
	}

	FILE *fp = fopen(p_filename, "wb");
	if (!fp) {
		printf("Error: cannot open output file '%s'\n", argv[1]);
		returncode = EXIT_FAILURE;
		goto err_exit;
	}

	uint8_t blockBuf[32];
	uint8_t crcBlock[2];
	uint8_t retReadBlk[18];
	int retLenReadBlk;

	for (size_t block = 0; block < numLTOCMBlocks; block++) {

		// read the first half of the block
		if (numLTOCMBlocks <= 255) {
			if (!ltocm_readblk(block, &retReadBlk[0], &retLenReadBlk)) {
				printf("Error: error with READ BLOCK command, block=%zu of %zu\n", block, numLTOCMBlocks-1);
				returncode = EXIT_FAILURE;
				goto err_exit;
			}
		} else {
			if (!ltocm_readblk_ext(block, &retReadBlk[0], &retLenReadBlk)) {
				printf("Error: error with READ BLOCK command, block=%zu of %zu\n", block, numLTOCMBlocks-1);
				returncode = EXIT_FAILURE;
				goto err_exit;
			}
		}
		// check the byte count and response bytes
		if ((retLenReadBlk == 1) && (retReadBlk[0] == LTOCM_NACK)) {
			printf("Error: READ BLOCK %zu (of %zu) failed, NACK\n", block, numLTOCMBlocks-1);
			returncode = EXIT_FAILURE;
			goto err_exit;
		} else if (retLenReadBlk != 18) {
			printf("Error: READ BLOCK %zu (of %zu) failed, insufficient response bytes\n", block, numLTOCMBlocks-1);
			returncode = EXIT_FAILURE;
			goto err_exit;
		}

		// check the CRC
		iso14443a_crc(retReadBlk, 16, crcBlock);
		if (memcmp(&retReadBlk[16], crcBlock, 2) != 0) {
			printf("Error: READ BLOCK %zu (of %zu) failed, CRC error\n", block, numLTOCMBlocks-1);
			returncode = EXIT_FAILURE;
			goto err_exit;
		}

		// copy first half of the block into the buffer
		memcpy(blockBuf, retReadBlk, 16);


		// read the second half of the block
		if (!ltocm_readblkcnt(&retReadBlk[0], &retLenReadBlk)) {
			printf("Error: error with READ BLOCK CONTINUE command, block=%zu\n", block);
			returncode = EXIT_FAILURE;
			goto err_exit;
		}

		// check the byte count and response bytes
		if ((retLenReadBlk == 1) && (retReadBlk[0] == LTOCM_NACK)) {
			printf("Error: READ BLOCK %zu (of %zu) failed, NACK\n", block, numLTOCMBlocks-1);
			returncode = EXIT_FAILURE;
			goto err_exit;
		} else if (retLenReadBlk != 18) {
			printf("Error: READ BLOCK %zu (of %zu) failed, insufficient response bytes\n", block, numLTOCMBlocks-1);
			returncode = EXIT_FAILURE;
			goto err_exit;
		}

		// check the CRC
		iso14443a_crc(retReadBlk, 16, crcBlock);
		if (memcmp(&retReadBlk[16], crcBlock, 2) != 0) {
			printf("Error: READ BLOCK %zu (of %zu) failed, CRC error\n", block, numLTOCMBlocks-1);
			returncode = EXIT_FAILURE;
			goto err_exit;
		}

		// copy second half of the block into the buffer
		memcpy(&blockBuf[16], retReadBlk, 16);

		// save the whole block to the file
		fwrite(blockBuf, 1, sizeof(blockBuf), fp);
	}

	fclose(fp);


err_exit:
	nfc_close(pnd);
	nfc_exit(context);
	exit(returncode);
}
