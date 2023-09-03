#include "NI_UWB.h"
#include "dw3000.h"
// #include "dw3000_config_options.h"
// #include "niq.h"

#define DWT_API_ERROR_CHECK 1

NI_UWB::NI_UWB()
{
    Serial.println("NI_UWB constructor");
    UART_init();
    test_run_info((unsigned char *)APP_NAME);
    shortAddress = (uint16_t)ESP.getEfuseMac();
    dwt_setaddress16(shortAddress);
    init();
}

void NI_UWB::init()
{
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

    /* Enabling LEDs here for debug so that for each TX the D1 LED will flash on DW3000 red eval-shield boards.
     * Note, in real low power applications the LEDs should not be used. */
    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);

    hasInitialized = true;
}

void NI_UWB::stop()
{
    Serial.println("NI_UWB stop");
    dwt_setdwstate(2); // DWT_DE_IDLE_RC (2)
    delay(2);
    while (!dwt_checkidlerc()) // Need to make sure DW IC is in IDLE_RC before proceeding
    {
        Serial.println("IDLE FAILED");
        throw "IDLE FAILED";
    }
    dwt_softreset();
    hasInitialized = false;
    hasStarted = false;
}

void NI_UWB::configure(dwt_config_t *config)
{
    Serial.println(">> NI_UWB configure");
    Serial.println("@@@@ NI_UWB configure with args. Should not be called");
    Serial.println("<< NI_UWB configure");
}

void NI_UWB::configure()
{
    Serial.println(">> NI_UWB configure no arg");

    if (!hasInitialized)
    {
        init();
    }

    setSourceAddress();

    /* Configure DW IC. See NOTE 13 below. */
    if (dwt_configure(&currentConfig)) /* if the dwt_configure returns DWT_ERROR either the PLL or RX calibration has failed the host should reset the device */
    {
        test_run_info((unsigned char *)"CONFIG FAILED     ");
        while (1)
        {
        };
    }

    /* Configure the TX spectrum parameters (power, PG delay and PG count) */
    dwt_configuretxrf(&txconfig_options);

    /* Apply default antenna delay value. See NOTE 2 below. */
    dwt_setrxantennadelay(RX_ANT_DLY);
    dwt_settxantennadelay(TX_ANT_DLY);

    /* Next can enable TX/RX states output on GPIOs 5 and 6 to help debug, and also TX/RX LEDs
     * Note, in real low power applications the LEDs should not be used. */
    // dwt_setlnapamode(DWT_LNA_ENABLE | DWT_PA_ENABLE);

    /*Configure the TX and RX AES jobs, the TX job is used to encrypt the Response message,
     * the RX job is used to decrypt the Poll message */
    aes_job_rx.mode = AES_Decrypt;                               /* Mode is set to decryption */
    aes_job_rx.src_port = AES_Src_Rx_buf_0;                      /* Take encrypted frame from the RX buffer */
    aes_job_rx.dst_port = AES_Dst_Rx_buf_0;                      /* Decrypt the frame to the same RX buffer : this will destroy original RX frame */
    aes_job_rx.header_len = MAC_FRAME_HEADER_SIZE(&mac_frame);   /* Set the header length (mac_frame contains the MAC header) */
    aes_job_rx.header = (uint8_t *)MHR_802_15_4_PTR(&mac_frame); /* Set the pointer to plain-text header which will not be encrypted */
    aes_job_rx.payload = rx_buffer;                              /* the decrypted RX MAC frame payload will be read out of IC into this buffer */

    aes_job_tx.mode = AES_Encrypt;        /* this is encyption job */
    aes_job_tx.src_port = AES_Src_Tx_buf; /* dwt_do_aes will take plain text to the TX buffer */
    aes_job_tx.dst_port = AES_Dst_Tx_buf; /* dwt_do_aes will replace the original plain text TX buffer with encrypted one */
    aes_job_tx.header_len = aes_job_rx.header_len;
    aes_job_tx.header = aes_job_rx.header;        /* plain-text header which will not be encrypted */
    aes_job_tx.payload = tx_resp_msg;             /* payload to be sent */
    aes_job_tx.payload_len = sizeof(tx_resp_msg); /* payload length */

    Serial.println("<< NI_UWB configure");
}

dwt_config_t *NI_UWB::getConfig()
{
    return &currentConfig;
}

void NI_UWB::setPanId(uint16_t panId)
{
    Serial.printf("Setting panId to %04X\n", panId);
    rx_poll_msg[3] = (uint8_t)panId;
    rx_poll_msg[4] = (uint8_t)(panId >> 8);
}

void NI_UWB::setDestAddress(uint16_t destAddress)
{
    Serial.printf("Setting dest address to %04X\n", destAddress);
    rx_poll_msg[7] = (uint8_t)destAddress;
    rx_poll_msg[8] = (uint8_t)(destAddress >> 8);

    tx_resp_msg[5] = (uint8_t)destAddress;
    tx_resp_msg[6] = (uint8_t)(destAddress >> 8);

    // destinationAddress = (uint64_t)(destAddress);
}

void NI_UWB::setSourceAddress()
{
    Serial.println(">> NI_UWB setSourceAddress");

    // Set the source address in the rx_poll_msg
    rx_poll_msg[5] = (uint8_t)shortAddress;
    rx_poll_msg[6] = (uint8_t)(shortAddress >> 8);

    // Set the dest address in the rx_resp_msg
    tx_resp_msg[7] = (uint8_t)shortAddress;
    tx_resp_msg[8] = (uint8_t)(shortAddress >> 8);
    Serial.println("<< NI_UWB setSourceAddress");
}

void NI_UWB::handleUWBConfigData(uint8_t *inBuffer, uint8_t inBufferLength)
{
    Serial.println(">> NI_UWB handleUWBConfigData");
    uint64_t destAddress = 0;
    uint16_t panId = 0;

    // dwt_softreset();
    if (inBufferLength < 28)
    {
        Serial.printf("NI_UWB handleUWBConfigData: invalid buffer length (%d)", inBufferLength);
        return;
    }

    // destAddress = (uint64_t)(inBuffer[26] | inBuffer[27] >> 8);
    setDestAddress(0);
    panId = (uint16_t)('L' | (0 >> 8));

    // Debug print
    Serial.printf("src_addr: %#08X, dst_addr: %#08X, pan_id: %#08lX\n", shortAddress, destAddress, panId);
    // end debug print

    mac_frame_set_pan_ids_and_addresses_802_15_4(&mac_frame, panId, destAddress, (uint64_t)shortAddress);

    Serial.println("<< NI_UWB handleUWBConfigData");
}

void NI_UWB::populateUWBConfigData(uint8_t *outBuffer, uint8_t *outBufferLength)
{
    Serial.println(">> NI_UWB populateUWBConfigData");

    // Hacky magic
    (*outBuffer) = 1;
    (*(outBuffer + 1)) = 0;
    (*(outBuffer + 2)) = 1;
    (*(outBuffer + 3)) = 0;
    (*(outBuffer + 4)) = 0x3f;
    (*(outBuffer + 5)) = 0xf5;
    (*(outBuffer + 6)) = 0x3;
    (*(outBuffer + 7)) = 0;
    (*(outBuffer + 8)) = 0xb8;
    (*(outBuffer + 9)) = 0xb;
    (*(outBuffer + 10)) = 0;
    (*(outBuffer + 11)) = 0;
    (*(outBuffer + 12)) = 0;
    (*(outBuffer + 13)) = 0;
    (*(outBuffer + 14)) = 1;
    (*(outBuffer + 15)) = 1;
    (*(outBuffer + 16)) = 1;
    (*(outBuffer + 17)) = (uint8_t)shortAddress;        // Short Address
    (*(outBuffer + 18)) = (uint8_t)(shortAddress >> 8); // Short Address
    (*(outBuffer + 19)) = 0x19;
    (*(outBuffer + 20)) = 0;
    *outBufferLength = 21;
    /*
    *(undefined *)((int)out_buffer + 0x10) = gCurrentRangingRole;
    uVar1 = gCurrentSourceAddress;
    *(undefined *)((int)out_buffer + 1) = 0;
    *(undefined *)((int)out_buffer + 3) = 0;
    *(undefined *)((int)out_buffer + 7) = 0;
    *(undefined *)((int)out_buffer + 10) = 0;
    *(undefined *)((int)out_buffer + 11) = 0;
    *(undefined *)((int)out_buffer + 12) = 0;
    *(undefined *)((int)out_buffer + 13) = 0;
    *(undefined *)((int)out_buffer + 20) = 0;
    *(undefined2 *)((int)out_buffer + 17) = uVar1;
    *(undefined *)out_buffer = 1;
    *(undefined *)((int)out_buffer + 2) = 1;
    *(undefined *)((int)out_buffer + 14) = 1;
    *(undefined *)((int)out_buffer + 15) = 1;
    *(undefined *)((int)out_buffer + 4) = 0x3f;
    *(undefined *)((int)out_buffer + 6) = 0x3;
    *(undefined *)((int)out_buffer + 5) = 0xf5;
    *(undefined *)((int)out_buffer + 8) = 0xb8;
    *(undefined *)((int)out_buffer + 9) = 0xb;
    *(undefined *)((int)out_buffer + 19) = 0x19;
    *out_buffer_len = 21; // 0x3f03f5b80b19
    */

    Serial.println("<< NI_UWB populateUWBConfigData");
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

        /* Poll for reception of a frame or error/timeout. See NOTE 6 below. */
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_ERR)))
        {
        };

        /* Once a frame has been received read the payload and decrypt*/
        if (status_reg & SYS_STATUS_RXFCG_BIT_MASK)
        {
            uint32_t frame_len;

            /* Clear good RX frame event in the DW IC status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);

            /* Read data length that was received */
            frame_len = dwt_read32bitreg(RX_FINFO_ID) & RXFLEN_MASK;

            /* A frame has been received: firstly need to read the MHR and check this frame is what we expect:
             * the destination address should match our source address (frame filtering can be configured for this check,
             * however that is not part of this example); then the header needs to have security enabled.
             * If any of these checks fail the rx_aes_802_15_4 will return an error
             * */
            aes_config.mode = AES_Decrypt;                /* configure for decryption*/
            PAYLOAD_PTR_802_15_4(&mac_frame) = rx_buffer; /* Set the MAC frame structure payload pointer
                                                               (this will contain decrypted data if status below is AES_RES_OK) */

            // Debug print
            uint64_t            src_addr,dst_addr;
            get_src_and_dst_frame_addr(&mac_frame, &src_addr, &dst_addr);
            Serial.printf("<< src_addr: %#08X, dst_addr: %#08lX\n", src_addr, dst_addr);
            destinationAddress = 0;
            Serial.printf(">> src_addr: %#08X, dst_addr: %" PRIu64 ", test: %d\n", shortAddress, destinationAddress, (uint64_t)0);
            // end debug print

            //             status = rx_aes_802_15_4(&mac_frame, frame_len, &aes_job_rx, sizeof(rx_buffer), keys_options, destinationAddress, (uint64_t)shortAddress, &aes_config);

            status = rx_aes_802_15_4(&mac_frame, frame_len, &aes_job_rx, sizeof(rx_buffer), keys_options, 0, (uint64_t)shortAddress, &aes_config);
            if (status != AES_RES_OK)
            {
                /* report any errors */
                switch (status)
                {
                case AES_RES_ERROR_LENGTH:
                    test_run_info((unsigned char *)"AES length error");
                    return;
                case AES_RES_ERROR:
                    test_run_info((unsigned char *)"ERROR AES");
                    return;
                case AES_RES_ERROR_FRAME:
                    test_run_info((unsigned char *)"Error Frame");
                    return;
                case AES_RES_ERROR_IGNORE_FRAME:
                    test_run_info((unsigned char *)"Frame not for us");
                    return;
                    // Got frame with wrong destination address
                }
            }

            /* Check that the payload of the MAC frame matches the expected poll message
             * as should be sent by "SS TWR AES initiator" example. */
            if (memcmp(rx_buffer, rx_poll_msg, aes_job_rx.payload_len) == 0)
            {
                uint32_t resp_tx_time;
                int ret;
                uint8_t nonce[13];

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

                /* Now need to encrypt the frame before transmitting*/

                /* Program the correct key to be used */
                dwt_set_keyreg_128(&keys_options[RESPONDER_KEY_INDEX - 1]);
                /* Set the key index for the frame */
                MAC_FRAME_AUX_KEY_IDENTIFY_802_15_4(&mac_frame) = RESPONDER_KEY_INDEX;

                /* Increment the sequence number */
                MAC_FRAME_SEQ_NUM_802_15_4(&mac_frame)
                ++;

                /* Update the frame count */
                mac_frame_update_aux_frame_cnt(&mac_frame, mac_frame_get_aux_frame_cnt(&mac_frame) + 1);

                /* Configure the AES job */
                aes_job_tx.mic_size = mac_frame_get_aux_mic_size(&mac_frame);
                aes_job_tx.nonce = nonce; /* set below once MHR is set*/
                aes_config.mode = AES_Encrypt;
                aes_config.mic = dwt_mic_size_from_bytes(aes_job_tx.mic_size);
                dwt_configure_aes(&aes_config);

                /* Update the MHR (reusing the received MHR, thus need to swap SRC/DEST addresses */
                mac_frame_set_pan_ids_and_addresses_802_15_4(&mac_frame, DEST_PAN_ID, destinationAddress, shortAddress);

                /* construct the nonce from the MHR */
                mac_frame_get_nonce(&mac_frame, nonce);

                /* perform the encryption, the TX buffer will contain a full MAC frame with encrypted payload*/
                status = dwt_do_aes(&aes_job_tx, aes_config.aes_core_type);
                if (status < 0)
                {
                    test_run_info((unsigned char *)"AES length error");
                    while (1)
                        ; /* Error */
                }
                else if (status & AES_ERRORS)
                {
                    test_run_info((unsigned char *)"ERROR AES");
                    while (1)
                        ; /* Error */
                }

                /* configure the frame control and start transmission */
                dwt_writetxfctrl(aes_job_tx.header_len + aes_job_tx.payload_len + aes_job_tx.mic_size + FCS_LEN, 0, 1); /* Zero offset in TX buffer, ranging. */
                ret = dwt_starttx(DWT_START_TX_DELAYED);

                /* If dwt_starttx() returns an error, abandon this ranging exchange and proceed to the next one. See NOTE 10 below. */
                if (ret == DWT_SUCCESS)
                {
                    /* Poll DW IC until TX frame sent event set. See NOTE 6 below. */
                    while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK))
                    {
                    };

                    /* Clear TXFRS event. */
                    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
                }
            }
        }
        else
        {
            /* Clear RX error events in the DW IC status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);
        }
        Serial.println("<< NI_UWB loop || hasStarted");
    }
}
