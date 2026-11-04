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

/// Read the first 16 bytes of a block with READ BLOCK or READ BLOCK EXT.
///
/// Returns true only if the half was received, acknowledged and its CRC
/// checked out.
static bool ltocm_read_half(size_t block, bool use_ext, uint8_t *retHalf)
{
	uint8_t readBlk[18];
	int lenReadBlk;
	uint8_t crcBlock[2];

	if (use_ext) {
		if (!ltocm_readblk_ext(block, &readBlk[0], &lenReadBlk))
			return false;
	} else {
		if (!ltocm_readblk(block, &readBlk[0], &lenReadBlk))
			return false;
	}

	if ((lenReadBlk == 1) && (readBlk[0] == LTOCM_NACK))
		return false;
	if (lenReadBlk != 18)
		return false;

	iso14443a_crc(readBlk, 16, crcBlock);
	if (memcmp(&readBlk[16], crcBlock, 2) != 0)
		return false;

	memcpy(retHalf, readBlk, 16);
	return true;
}

/// Read the second 16 bytes of the current block with READ BLOCK CONTINUE.
static bool ltocm_read_half_cont(uint8_t *retHalf)
{
	uint8_t readBlk[18];
	int lenReadBlk;
	uint8_t crcBlock[2];

	if (!ltocm_readblkcnt(&readBlk[0], &lenReadBlk))
		return false;

	if ((lenReadBlk == 1) && (readBlk[0] == LTOCM_NACK))
		return false;
	if (lenReadBlk != 18)
		return false;

	iso14443a_crc(readBlk, 16, crcBlock);
	if (memcmp(&readBlk[16], crcBlock, 2) != 0)
		return false;

	memcpy(retHalf, readBlk, 16);
	return true;
}

/// Read one complete 32-byte LTO-CM block.
///
/// READ BLOCK carries a single-byte address, so it only covers blocks 0..254;
/// the value 0xFF is rejected. Block 255 and above need READ BLOCK EXT. If
/// the preferred form fails we retry with the other one, so the tool copes
/// with chips that only implement one of them.
bool ltocm_read_block(size_t block, uint8_t *retBlock)
{
	bool use_ext = (block >= 0xFF);

	if (block > 0xFF) {
		// Too large for the short form's one-byte address.
		if (!ltocm_read_half(block, true, &retBlock[0]))
			return false;
	} else if (!ltocm_read_half(block, use_ext, &retBlock[0]) &&
			!ltocm_read_half(block, !use_ext, &retBlock[0])) {
		return false;
	}

	return ltocm_read_half_cont(&retBlock[16]);
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

	// Validate LTO-CM REQUEST STANDARD response. This gives the memory type,
	// but the reply does not always agree with the capacity the chip actually
	// declares, so it is only used as a fallback for sizing (see below).
	uint16_t ltoCMStandard = ((uint16_t)ltoStandard[0] << 8) | ((uint16_t)ltoStandard[1]);
	switch (ltoCMStandard) {
		case 0x0001:
		case 0x0002:
		case 0x0003:
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

	// The chip declares its own capacity in Block 0, byte 5, in units of
	// 1 KiB (ECMA-319 Annex D, D.2.1). This is more reliable than the memory
	// type from REQUEST STANDARD, whose usual values are one block short:
	// a 4 KiB part has 128 blocks, an 8 KiB part 256, a 16 KiB part 512.
	uint8_t block0[32];
	if (!ltocm_read_block(0, block0)) {
		printf("Error: could not read LTO-CM block 0\n");
		returncode = EXIT_FAILURE;
		goto err_exit;
	}

	size_t capacity = (size_t)block0[5] << 10;
	size_t numLTOCMBlocks = capacity / 32;

	if ((numLTOCMBlocks < 32) || (numLTOCMBlocks > 1024)) {
		printf("Warning: implausible LTO-CM capacity %zu bytes (block 0 byte 5 = 0x%02X), falling back to memory type\n",
				capacity, block0[5]);
		numLTOCMBlocks = (ltoCMStandard == 0x0001) ? 128 :
				(ltoCMStandard == 0x0002) ? 256 : 512;
	}

	printf("LTO-CM capacity: %zu bytes, %zu blocks\n",
			numLTOCMBlocks * 32, numLTOCMBlocks);

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

	// Block 0 has already been read (it carries the capacity), so write it
	// out first and then read the remainder.
	fwrite(block0, 1, sizeof(block0), fp);
	size_t blocksWritten = 1;

	uint8_t blockBuf[32];

	for (size_t block = 1; block < numLTOCMBlocks; block++) {
		if (!ltocm_read_block(block, blockBuf)) {
			if (block == numLTOCMBlocks - 1) {
				// The final block of the declared range is often not
				// exposed by the chip. That is not an error.
				printf("Note: final block (%zu) is not readable; the chip does not expose it\n", block);
				break;
			}
			printf("Error: READ BLOCK %zu (of %zu) failed\n", block, numLTOCMBlocks-1);
			returncode = EXIT_FAILURE;
			fclose(fp);
			goto err_exit;
		}

		// save the whole block to the file
		fwrite(blockBuf, 1, sizeof(blockBuf), fp);
		blocksWritten++;
	}

	fclose(fp);

	printf("Wrote %zu blocks (%zu bytes) to %s\n",
			blocksWritten, blocksWritten * 32, p_filename);


err_exit:
	nfc_close(pnd);
	nfc_exit(context);
	exit(returncode);
}
