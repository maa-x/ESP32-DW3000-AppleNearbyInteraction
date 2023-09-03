#ifndef NI_UWB_H_
#define NI_UWB_H_

#include "dw3000.h"
#include "dw3000_mac_802_15_4.h"

#define APP_NAME "UWB"

/*

void niq_configure_and_start_uwb(uint8_t *payload,uint8_t length,fira_device_configure_s *arg)

{
  byte bVar1;
  undefined2 uVar2;
  ushort uVar3;
  uint16_t uVar4;
  uint uVar5;
  code *uwb_init;

  if (length != 30) {
    return;
  }
  if ((*(short *)payload == 1) && (*(short *)(payload + 2) == 1)) {
    arg->Preamble_Code = payload[11];
    arg->Channel_Number = payload[12];
    uVar3 = *(ushort *)(payload + 15);
    arg->Slot_Duration_RSTU = (uint)uVar3;
    arg->Round_Duration_RSTU = (uint)uVar3 * (uint)*(ushort *)(payload + 13);
    arg->Block_Duration_ms = (uint)*(ushort *)(payload + 17);
    arg->Session_ID = *(uint32_t *)(payload + 7);
    bVar1 = payload[19];
    *(undefined2 *)arg->SRC_ADDR = gCurrentSourceAddress;
    uVar5 = count_leading_zeroes(bVar1 - 3);
    arg->ToF_Report = (uint8_t)(uVar5 >> 5);
    uVar2 = *(undefined2 *)(payload + 26);
    arg->DST_ADDR[0] = (uint8_t)uVar2;
    arg->DST_ADDR[1] = (uint8_t)((ushort)uVar2 >> 8);
    arg->Vendor_ID[0] = 'L';
    arg->Vendor_ID[1] = 0;
    arg->Static_STS_IV[0] = payload[25];
    arg->Static_STS_IV[1] = payload[24];
    arg->Static_STS_IV[2] = payload[23];
    arg->Static_STS_IV[3] = payload[22];
    arg->Static_STS_IV[4] = payload[21];
    arg->Static_STS_IV[5] = payload[20];
    arg->role = gCurrentRangingRole;
    uVar4 = *(uint16_t *)(payload + 28);
    arg->STS_Config = 0;
    uwb_init = ResumeUwbTasks;
    arg->Block_Timing_Stability = uVar4;
    arg->MAX_RR_Retry = 0;
    arg->enc_payload = '\x01';
    *(undefined2 *)&arg->Ranging_Round_Usage = 2;
    arg->SP0_PHY_Set = '\x02';
    arg->Rframe_Config = '\x03';
    arg->SP3_PHY_Set = '\x04';
    arg->UWB_Init_Time_ms = 5;
    if (uwb_init != (code *)0x0) {
      (*uwb_init)();
      return;
    }
    return;
  }
  return;
}
*/

// #define SRC_ADDR 0x1122334455667788  /* this is the address of the initiator */
// #define DEST_ADDR 0x8877665544332211 /* this is the address of the responder */
#define DEST_PAN_ID 0x4321           /* this is the PAN ID used in this example */

/* Default antenna delay values for 64 MHz PRF. See NOTE 2 below. */
#define TX_ANT_DLY 16385
#define RX_ANT_DLY 16385

/* Index to access some of the fields in the frames involved in the process. */
#define ALL_MSG_SN_IDX 2          // sequence number byte index in MHR
#define RESP_MSG_POLL_RX_TS_IDX 0 // index in the MAC payload for Poll RX time
#define RESP_MSG_RESP_TX_TS_IDX 4 // index in the MAC payload for Response TX time
#define RESP_MSG_TS_LEN 4

/* Buffer to store received response message.
 * Its size is adjusted to longest frame that this example code can handle. */
#define RX_BUF_LEN 127 /* The received frame cannot be bigger than 127 if STD PHR mode is used */

/* Note, the key index of 0 is forbidden to send as key index. Thus index 1 is the first.
 * This example uses this index for the key table for the encryption of responder's data */
#define RESPONDER_KEY_INDEX 2

/* Delay between frames, in UWB microseconds. See NOTE 1 below.
 * this includes the poll frame length ~ 240 us*/
#define POLL_RX_TO_RESP_TX_DLY_UUS 2000

class NI_UWB
{
private:
  // connection pins
  const uint8_t PIN_RST = 27; // reset pin
  const uint8_t PIN_IRQ = 34; // irq pin
  const uint8_t PIN_SS = 4;   // spi select pin

  mac_frame_802_15_4_format_t mac_frame;

  dwt_aes_config_t aes_config{
      AES_key_RAM,
      AES_core_type_CCM,
      MIC_0,
      AES_KEY_Src_Register,
      AES_KEY_Load,
      0,
      AES_KEY_128bit,
      AES_Encrypt};

  // /* Default communication configuration. We use default non-STS DW mode. */
  // dwt_config_t currentConfig
  // {
  //     9,                /* Channel number. */
  //     DWT_PLEN_64,     /* Preamble length. Used in TX only. */
  //     DWT_PAC8,         /* Preamble acquisition chunk size. Used in RX only. */
  //     9,               /* TX preamble code. Used in TX only. */
  //     9,               /* RX preamble code. Used in RX only. */
  //     3,                /* 0 to use standard 8 symbol SFD, 1 to use non-standard 8 symbol, 2 for non-standard 16 symbol SFD and 3 for 4z 8 symbol SDF type */
  //     DWT_BR_6M8,       /* Data rate. */
  //     DWT_PHRMODE_STD,  /* PHY header mode. */
  //     DWT_PHRRATE_STD,  /* PHY header rate. */
  //     (64 + 1 + 8 - 8),    /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
  //     DWT_STS_MODE_OFF, /* STS disabled */
  //     DWT_STS_LEN_64,  /* STS length see allowed values in Enum dwt_sts_lengths_e */
  //     DWT_PDOA_M0       /* PDOA mode off */
  // };

  dwt_config_t currentConfig {
    5,                  /* Channel number. */
    DWT_PLEN_128,       /* Preamble length. Used in TX only. */
    DWT_PAC8,           /* Preamble acquisition chunk size. Used in RX only. */
    9,                  /* TX preamble code. Used in TX only. */
    9,                  /* RX preamble code. Used in RX only. */
    3,                  /* 0 to use standard 8 symbol SFD, 1 to use non-standard 8 symbol, 2 for non-standard 16 symbol SFD and 3 for 4z 8 symbol SDF type */
    DWT_BR_6M8,         /* Data rate. */
    DWT_PHRMODE_STD,    /* PHY header mode. */
    DWT_PHRRATE_STD,    /* PHY header rate. */
    (128 + 1 + 8 - 8),  /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
    DWT_STS_MODE_OFF,   /* STS Off */
    DWT_STS_LEN_128,    /* Ignore value when STS is disabled */
    DWT_PDOA_M0         /* PDOA mode off */
};

  dwt_txconfig_t txconfig_options
  {
          0x34,       /* PG delay. */
          0xfefefefe, /* TX power. */
          0x0         /*PG count*/
  };

  /* Optional keys according to the key index - In AUX security header*/
  dwt_aes_key_t keys_options[NUM_OF_KEY_OPTIONS]{
      {0x00010203, 0x04050607, 0x08090A0B, 0x0C0D0E0F, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
      {0x11223344, 0x55667788, 0x99AABBCC, 0xDDEEFF00, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
      {0xFFEEDDCC, 0xBBAA9988, 0x77665544, 0x33221100, 0x00000000, 0x00000000, 0x00000000, 0x00000000}};

  /* Timestamps of frames transmission/reception. */
  uint64_t poll_rx_ts;
  uint64_t resp_tx_ts;

  /* MAC payload data of the frames used in the ranging process. See NOTE 3 below. */
  /* Poll message from the initiator to the responder */
  uint8_t rx_poll_msg[12]{'P', 'o', 'l', 'l', ' ', 'm', 'e', 's', 's', 'a', 'g', 'e'};
  /* Response message to the initiator. The first 8 bytes are used for Poll RX time and Response TX time.*/
  uint8_t tx_resp_msg[16]{0, 0, 0, 0, 0, 0, 0, 0, 'R', 'e', 's', 'p', 'o', 'n', 's', 'e'};

  uint8_t rx_buffer[RX_BUF_LEN];

  /* Values for the PG_DELAY and TX_POWER registers reflect the bandwidth and power of the spectrum at the current
   * temperature. These values can be calibrated prior to taking reference measurements. See NOTE 5 below. */

  dwt_aes_job_t aes_job_rx, aes_job_tx;
  int8_t status;
  uint32_t status_reg;

  bool hasInitialized = false;
  bool hasStarted = false;
  bool hasConfigured = false;
  bool appHasReadConfiguration = false;

  uint16_t shortAddress;
  uint64_t destinationAddress = 0x00000000;

  void setSourceAddress();
  void setDestAddress(uint16_t destAddress);
  void setPanId(uint16_t panId);

public:
  NI_UWB();

  void configure(dwt_config_t *config);
  void configure();
  dwt_config_t *getConfig();
  void start();
  void loop();
  void stop();
  void init();

  void populateUWBConfigData(uint8_t *outBuffer, uint8_t *outBufferLength);
  void handleUWBConfigData(uint8_t *inBuffer, uint8_t inBufferLength);
};

#endif /* NI_UWB_H_ */

/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. The single-sided two-way ranging scheme implemented here has to be considered carefully as the accuracy of the distance measured is highly
 *    sensitive to the clock offset error between the devices and the length of the response delay between frames. To achieve the best possible
 *    accuracy, this response delay must be kept as low as possible. In order to do so, 6.8 Mbps data rate is used in this example and the response
 *    delay between frames is defined as low as possible. The user is referred to User Manual for more details about the single-sided two-way ranging
 *    process.
 *
 *    Initiator: |Poll TX| ..... |Resp RX|
 *    Responder: |Poll RX| ..... |Resp TX|
 *                   ^|P RMARKER|                    - time of Poll TX/RX
 *                                   ^|R RMARKER|    - time of Resp TX/RX
 *
 *                       <--TDLY->                   - POLL_TX_TO_RESP_RX_DLY_UUS (RDLY-RLEN)
 *                               <-RLEN->            - RESP_RX_TIMEOUT_UUS   (length of response frame)
 *                    <----RDLY------>               - POLL_RX_TO_RESP_TX_DLY_UUS (depends on how quickly responder can turn around and reply)
 *
 *
 * 2. The sum of the values is the TX to RX antenna delay, experimentally determined by a calibration process. Here we use a hard coded typical value
 *    but, in a real application, each device should have its own antenna delay properly calibrated to get the best possible precision when performing
 *    range measurements.
 * 3. The frames used here are Decawave specific ranging frames, complying with the IEEE 802.15.4 standard data frame encoding. The frames are the
 *    following:
 *     - a poll message sent by the initiator to trigger the ranging exchange.
 *     - a response message sent by the responder to complete the exchange and provide all information needed by the initiator to compute the
 *       time-of-flight (distance) estimate.
 *    The first 10 bytes of those frame are common and are composed of the following fields:
 *     - byte 0/1: frame control (0x8841 to indicate a data frame using 16-bit addressing).
 *     - byte 2: sequence number, incremented for each new frame.
 *     - byte 3/4: PAN ID (0xDECA).
 *     - byte 5/6: destination address, see NOTE 4 below.
 *     - byte 7/8: source address, see NOTE 4 below.
 *     - byte 9: function code (specific values to indicate which message it is in the ranging process).
 *    The remaining bytes are specific to each message as follows:
 *    Poll message:
 *     - no more data
 *    Response message:
 *     - byte 0 -> 13: poll message reception timestamp.
 *     - byte 4 -> 17: response message transmission timestamp.
 *    All messages end with a 2-byte checksum automatically set by DW IC.
 * 4. Source and destination addresses are hard coded constants in this example to keep it simple but for a real product every device should have a
 *    unique ID. Here, 16-bit addressing is used to keep the messages as short as possible but, in an actual application, this should be done only
 *    after an exchange of specific messages used to define those short addresses for each device participating to the ranging exchange.
 * 5. In a real application, for optimum performance within regulatory limits, it may be necessary to set TX pulse bandwidth and TX power, (using
 *    the dwt_configuretxrf API call) to per device calibrated values saved in the target system or the DW IC OTP memory.
 * 6. We use polled mode of operation here to keep the example as simple as possible but all status events can be used to generate interrupts. Please
 *    refer to DW IC User Manual for more details on "interrupts". It is also to be noted that STATUS register is 5 bytes long but, as the event we
 *    use are all in the first bytes of the register, we can use the simple dwt_read32bitreg() API call to access it instead of reading the whole 5
 *    bytes.
 * 7. As we want to send final TX timestamp in the final message, we have to compute it in advance instead of relying on the reading of DW IC
 *    register. Timestamps and delayed transmission time are both expressed in device time units so we just have to add the desired response delay to
 *    response RX timestamp to get final transmission time. The delayed transmission time resolution is 512 device time units which means that the
 *    lower 9 bits of the obtained value must be zeroed. This also allows to encode the 40-bit value in a 32-bit words by shifting the all-zero lower
 *    8 bits.
 * 8. In this operation, the high order byte of each 40-bit timestamps is discarded. This is acceptable as those time-stamps are not separated by
 *    more than 2**32 device time units (which is around 67 ms) which means that the calculation of the round-trip delays (needed in the
 *    time-of-flight computation) can be handled by a 32-bit subtraction.
 * 9. dwt_writetxdata() takes the full size of the message as a parameter but only copies (size - 2) bytes as the check-sum at the end of the frame is
 *    automatically appended by the DW IC. This means that our variable could be two bytes shorter without losing any data (but the sizeof would not
 *    work anymore then as we would still have to indicate the full length of the frame to dwt_writetxdata()).
 * 10. When running this example on the EVB1000 platform with the POLL_RX_TO_RESP_TX_DLY response delay provided, the dwt_starttx() is always
 *     successful. However, in cases where the delay is too short (or something else interrupts the code flow), then the dwt_starttx() might be issued
 *     too late for the configured start time. The code below provides an example of how to handle this condition: In this case it abandons the
 *     ranging exchange and simply goes back to awaiting another poll message. If this error handling code was not here, a late dwt_starttx() would
 *     result in the code flow getting stuck waiting subsequent RX event that will will never come. The companion "initiator" example (ex_06a) should
 *     timeout from awaiting the "response" and proceed to send another poll in due course to initiate another ranging exchange.
 * 11. The user is referred to DecaRanging ARM application (distributed with DW3000 product) for additional practical example of usage, and to the
 *     DW IC API Guide for more details on the DW IC driver functions.
 * 12. In this example, the DW IC is put into IDLE state after calling dwt_initialise(). This means that a fast SPI rate of up to 20 MHz can be used
 *     thereafter.
 * 13. Desired configuration by user may be different to the current programmed configuration. dwt_configure is called to set desired
 *     configuration.
 * 14. When CCM core type is used, AES_KEY_Load needs to be set prior to each encryption/decryption operation, even if the AES KEY used has not changed.
 ****************************************************************************************************************************************************/
