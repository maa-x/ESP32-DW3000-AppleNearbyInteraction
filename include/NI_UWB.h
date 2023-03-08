#ifndef NI_UWB_H_
#define NI_UWB_H_

#include "dw3000.h"

#define APP_NAME "UWB"



/* Default antenna delay values for 64 MHz PRF. See NOTE 2 below. */
#define TX_ANT_DLY 16385
#define RX_ANT_DLY 16385

/* Frames used in the ranging process. See NOTE 3 below. */
static uint8_t rx_poll_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0xE0, 0, 0};
static uint8_t tx_resp_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'V', 'E', 'W', 'A', 0xE1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
/* Length of the common part of the message (up to and including the function code, see NOTE 3 below). */
#define ALL_MSG_COMMON_LEN 10
/* Index to access some of the fields in the frames involved in the process. */
#define ALL_MSG_SN_IDX 2
#define RESP_MSG_POLL_RX_TS_IDX 10
#define RESP_MSG_RESP_TX_TS_IDX 14
#define RESP_MSG_TS_LEN 4
/* Frame sequence number, incremented after each transmission. */
static uint8_t frame_seq_nb = 0;

/* Buffer to store received messages.
 * Its size is adjusted to longest frame that this example code is supposed to handle. */
#define RX_BUF_LEN 12//Must be less than FRAME_LEN_MAX_EX
static uint8_t rx_buffer[RX_BUF_LEN];

/* Hold copy of status register state here for reference so that it can be examined at a debug breakpoint. */
static uint32_t status_reg = 0;

/* Delay between frames, in UWB microseconds. See NOTE 1 below. */
#ifdef RPI_BUILD
#define POLL_RX_TO_RESP_TX_DLY_UUS 550
#endif //RPI_BUILD
#ifdef STM32F429xx
#define POLL_RX_TO_RESP_TX_DLY_UUS 450
#endif //STM32F429xx
#ifdef NRF52840_XXAA
#define POLL_RX_TO_RESP_TX_DLY_UUS 650
#endif //NRF52840_XXAA

#define POLL_RX_TO_RESP_TX_DLY_UUS 450

/* Timestamps of frames transmission/reception. */
static uint64_t poll_rx_ts;
static uint64_t resp_tx_ts;

/* Values for the PG_DELAY and TX_POWER registers reflect the bandwidth and power of the spectrum at the current
 * temperature. These values can be calibrated prior to taking reference measurements. See NOTE 5 below. */
extern dwt_txconfig_t txconfig_options;
extern dwt_txconfig_t txconfig_options;
extern dwt_txconfig_t txconfig_options_ch9;
// extern dwt_config_t config_options;

class NI_UWB {
    public:
        NI_UWB();

        void configure(dwt_config_t *config);
        void configure();
        dwt_config_t* getConfig();
        void start();
        void loop();
        
    private:
        // connection pins
        const uint8_t PIN_RST = 27; // reset pin
        const uint8_t PIN_IRQ = 34; // irq pin
        const uint8_t PIN_SS = 4; // spi select pin

        dwt_config_t *currentConfig = nullptr;

        bool hasStarted = false;
};

#endif /* NI_UWB_H_ */