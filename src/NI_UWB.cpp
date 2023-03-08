#include "NI_UWB.h"
#include "dw3000.h"
#include "dw3000_config_options.h"

#define DWT_API_ERROR_CHECK

NI_UWB::NI_UWB() 
{
    Serial.println("NI_UWB constructor");

    currentConfig = new dwt_config_t;

    memcpy(currentConfig, &config_options, sizeof(dwt_config_t));

    // currentConfig = {
    //     5,                 /* Channel number. */
    //     DWT_PLEN_128,      /* Preamble length. Used in TX only. */
    //     DWT_PAC8,          /* Preamble acquisition chunk size. Used in RX only. */
    //     9,                 /* TX preamble code. Used in TX only. */
    //     9,                 /* RX preamble code. Used in RX only. */
    //     3,                 /* 0 to use standard 8 symbol SFD, 1 to use non-standard 8 symbol, 2 for non-standard 16 symbol SFD and 3 for 4z 8 symbol SDF type */
    //     DWT_BR_6M8,        /* Data rate. */
    //     DWT_PHRMODE_STD,   /* PHY header mode. */
    //     DWT_PHRRATE_STD,   /* PHY header rate. */
    //     (128 + 1 + 8 - 8), /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
    //     DWT_STS_MODE_1,    /* Mode 1 STS enabled */
    //     DWT_STS_LEN_128,   /* (STS length  in blocks of 8) - 1*/
    //     DWT_PDOA_M0        /* PDOA mode off */
    // };

    Serial.printf("NI_UWB constructor: %d\r\n", currentConfig->rxCode);
    /* Configure SPI rate, DW3000 supports up to 38 MHz */
    /* Reset DW IC */
    spiBegin(PIN_IRQ, PIN_RST);
    spiSelect(PIN_SS);

    delay(2); // Time needed for DW3000 to start up (transition from INIT_RC to IDLE_RC, or could wait for SPIRDY event)

    while (!dwt_checkidlerc()) // Need to make sure DW IC is in IDLE_RC before proceeding
    {
        UART_puts("IDLE FAILED\r\n");
        while (1)
            ;
    }

    if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR)
    {
        UART_puts("INIT FAILED\r\n");
        while (1)
            ;
    }

    // Enabling LEDs here for debug so that for each TX the D1 LED will flash on DW3000 red eval-shield boards.
    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);
}

void NI_UWB::configure(dwt_config_t *config)
{   
    Serial.println(">> NI_UWB configure");
    Serial.printf("NI_UWB configure: %p\r\n", config);
    currentConfig = config;

    /* Configure DW3000. See NOTE 7 below. */
    if (dwt_configure(currentConfig) == DWT_ERROR)
    {
        UART_puts("CONFIG FAILED\r\n");
        throw std::runtime_error("DWT CONFIG FAILED");
    } else
    {
        UART_puts("CONFIG OK\r\n");
    }

    /* Configure DW3000 LEDs */
    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);

    /* Configure DW3000 TX power. */
    dwt_configuretxrf(&txconfig_options);

    /* Configure DW3000 antenna delay. See NOTE 8 below. */
    dwt_setrxantennadelay(RX_ANT_DLY);
    dwt_settxantennadelay(TX_ANT_DLY);
    Serial.println("<< NI_UWB configure");
}

void NI_UWB::configure()
{   
    Serial.println(">> NI_UWB configure no arg");

    Serial.printf("NI_UWB configure: %p\r\n", currentConfig);
    Serial.printf("NI_UWB configure: %d\r\n", currentConfig->chan);
    

    /* Configure DW3000. See NOTE 7 below. */
    if (dwt_configure(currentConfig) == DWT_ERROR)
    {
        UART_puts("CONFIG FAILED\r\n");
        throw std::runtime_error("DWT CONFIG FAILED");
    } else
    {
        UART_puts("CONFIG OK\r\n");
    }

    /* Configure DW3000 LEDs */
    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);

    /* Configure DW3000 TX power. */
    dwt_configuretxrf(&txconfig_options);

    /* Configure DW3000 antenna delay. See NOTE 8 below. */
    dwt_setrxantennadelay(RX_ANT_DLY);
    dwt_settxantennadelay(TX_ANT_DLY);
    Serial.println("<< NI_UWB configure");
}

dwt_config_t *NI_UWB::getConfig()
{
    return currentConfig;
}

void NI_UWB::start()
{
    Serial.println("NI_UWB start");
    hasStarted = true;
}

void NI_UWB::loop() 
{
    if (hasStarted)
    {
        Serial.println(">> NI_UWB loop || hasStarted");
        /* Activate reception immediately. */
        dwt_rxenable(DWT_START_RX_IMMEDIATE);
        Serial.println(">> NI_UWB loop || dwt_rxenable");
        /* Poll for reception of a frame or error/timeout. See NOTE 6 below. */
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_ERR)))
        {
        };
        Serial.println(">> NI_UWB loop || dwt_read32bitreg");
        if (status_reg & SYS_STATUS_RXFCG_BIT_MASK)
        {
            uint32_t frame_len;
            Serial.println(">> NI_UWB loop || SYS_STATUS_RXFCG_BIT_MASK");
            /* Clear good RX frame event in the DW IC status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);

            /* A frame has been received, read it into the local buffer. */
            frame_len = dwt_read32bitreg(RX_FINFO_ID) & RXFLEN_MASK;
            if (frame_len <= sizeof(rx_buffer))
            {
                Serial.println(">> NI_UWB loop || frame_len <= sizeof(rx_buffer)");
                dwt_readrxdata(rx_buffer, frame_len, 0);

                /* Check that the frame is a poll sent by "SS TWR initiator" example.
                 * As the sequence number field of the frame is not relevant, it is cleared to simplify the validation of the frame. */
                rx_buffer[ALL_MSG_SN_IDX] = 0;
                if (memcmp(rx_buffer, rx_poll_msg, ALL_MSG_COMMON_LEN) == 0)
                {
                    Serial.println(">> NI_UWB loop || memcmp(rx_buffer, rx_poll_msg, ALL_MSG_COMMON_LEN) == 0");
                    uint32_t resp_tx_time;
                    int ret;

                    /* Retrieve poll reception timestamp. */
                    poll_rx_ts = get_rx_timestamp_u64();

                    /* Compute response message transmission time. See NOTE 7 below. */
                    resp_tx_time = (poll_rx_ts + (POLL_RX_TO_RESP_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
                    dwt_setdelayedtrxtime(resp_tx_time);

                    /* Response TX timestamp is the transmission time we programmed plus the antenna delay. */
                    resp_tx_ts = (((uint64_t)(resp_tx_time & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;

                    /* Write all timestamps in the final message. See NOTE 8 below. */
                    resp_msg_set_ts(&tx_resp_msg[RESP_MSG_POLL_RX_TS_IDX], poll_rx_ts);
                    resp_msg_set_ts(&tx_resp_msg[RESP_MSG_RESP_TX_TS_IDX], resp_tx_ts);

                    /* Write and send the response message. See NOTE 9 below. */
                    tx_resp_msg[ALL_MSG_SN_IDX] = frame_seq_nb;
                    dwt_writetxdata(sizeof(tx_resp_msg), tx_resp_msg, 0); /* Zero offset in TX buffer. */
                    dwt_writetxfctrl(sizeof(tx_resp_msg), 0, 1);          /* Zero offset in TX buffer, ranging. */
                    ret = dwt_starttx(DWT_START_TX_DELAYED);

                    Serial.println(">> NI_UWB loop || dwt_starttx(DWT_START_TX_DELAYED)");
                    /* If dwt_starttx() returns an error, abandon this ranging exchange and proceed to the next one. See NOTE 10 below. */
                    if (ret == DWT_SUCCESS)
                    {
                        /* Poll DW IC until TX frame sent event set. See NOTE 6 below. */
                        while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK))
                        {
                        };

                        /* Clear TXFRS event. */
                        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);

                        /* Increment frame sequence number after transmission of the poll message (modulo 256). */
                        frame_seq_nb++;
                    }
                }
            }
        }
        else
        {
            Serial.println(">> NI_UWB loop || SYS_STATUS_ALL_RX_ERR");
            /* Clear RX error events in the DW IC status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);
        }
        Serial.println("<< NI_UWB loop || hasStarted");
    }
}

