/***
 * nfc-ltocm: Read LTO Cartridge Memory with libnfc
 *
 * Phil Pemberton <philpem@philpem.me.uk>
 * github.com/philpem
 * www.philpem.me.uk
 *
 * MIT licenced.
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

#include "nfc-utils.h"



const bool quiet_output = false;

#define MAX_FRAME_LEN 264

static uint8_t abtRx[MAX_FRAME_LEN];
static int szRxBits;
static nfc_device *pnd;


// LTO-CM commands
const uint8_t LTOCM_REQUEST_STANDARD = { 0x45 };		///< LTO-CM REQUEST STANDARD: returns 2 bytes


static  bool
transmit_bits(const uint8_t *pbtTx, const size_t szTxBits)
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


static  bool
transmit_bytes(const uint8_t *pbtTx, const size_t szTx)
{
  // Show transmitted command
  if (!quiet_output) {
    printf("Sent bits:     ");
    print_hex(pbtTx, szTx);
  }
  int res;
  // Transmit the command bytes
  if ((res = nfc_initiator_transceive_bytes(pnd, pbtTx, szTx, abtRx, sizeof(abtRx), 0)) < 0)
    return false;

  // Show received answer
  if (!quiet_output) {
    printf("Received bits: ");
    print_hex(abtRx, res);
  }
  // Succesful transfer
  return true;
}


int main(void)
{
	int returncode = EXIT_SUCCESS;


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

	// Send the 7 bits request command specified in ISO 14443A (0x26)
	if (!transmit_bits(&LTOCM_REQUEST_STANDARD, 7)) {
		printf("Error: No tag available\n");
		returncode = EXIT_FAILURE;
		goto err_exit;
	}
	//memcpy(abtAtqa, abtRx, 2);

	// Anti-collision
	//transmit_bytes(abtSelectAll, 2);

err_exit:
	nfc_close(pnd);
	nfc_exit(context);
	exit(returncode);
}
