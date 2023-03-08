#include "nrf.h"
#include "nrf-hw-support.h"

// enable crc, enable 2 byte
#   define set_bit(x) (1<<(x))
#   define enable_crc      set_bit(3)
#   define crc_two_byte    set_bit(2)
#   define mask_int         set_bit(6)|set_bit(5)|set_bit(4)
enum {
    // pre-computed: can write into NRF_CONFIG to enable TX.
    tx_config = enable_crc | crc_two_byte | set_bit(PWR_UP) | mask_int,
    // pre-computed: can write into NRF_CONFIG to enable RX.
    rx_config = tx_config  | set_bit(PRIM_RX)
} ;

nrf_t * nrf_init(nrf_conf_t c, uint32_t rxaddr, unsigned acked_p) {
    nrf_t *n = kmalloc(sizeof *n);
    n->config = c;
    nrf_stat_start(n);
    n->spi = pin_init(c.ce_pin, c.spi_chip);
    n->rxaddr = rxaddr;

    // turn everything off
    nrf_put8_chk(n, NRF_CONFIG, 0);
    assert(!nrf_is_pwrup(n));

    // disable pipes, p57
    nrf_put8_chk(n, NRF_EN_RXADDR, 0);

    if (acked_p) {
        // enable auto acknowledgment
        nrf_put8_chk(n, NRF_EN_AA, 1 << 1 | 1);

        // enable pipes 0, 1
        nrf_put8_chk(n, NRF_EN_RXADDR, 1 << 1 | 1);
        
        // set default retran values
        unsigned rt_d = 0b0111; // 2000usec
        unsigned rt_cnt = nrf_default_retran_attempts;

        // set up retran delay and retran attempts
        nrf_put8_chk(n, NRF_SETUP_RETR, rt_cnt | (rt_d << 4));
    } else {
        // disable auto acknowledgment
        nrf_put8_chk(n, NRF_EN_AA, 0);
        // enable pipe 1
        nrf_put8_chk(n, NRF_EN_RXADDR, 1 << 1);

        // zero out retran delay and attempts
        nrf_put8_chk(n, NRF_SETUP_RETR, 0);
    }

    // set address size, p58
    unsigned v = nrf_default_addr_nbytes - 2;
    nrf_put8_chk(n, NRF_SETUP_AW, v);

    // zero TX addr
    nrf_set_addr(n, NRF_TX_ADDR, 0x0, nrf_default_addr_nbytes);

    // zero pipe 0
    nrf_put8_chk(n, NRF_RX_PW_P0, 0x0);
    nrf_set_addr(n, NRF_RX_ADDR_P0, 0x0, nrf_default_addr_nbytes);

    // set pipe 1 to rxaddr
    nrf_put8_chk(n, NRF_RX_PW_P1, c.nbytes);
    nrf_set_addr(n, NRF_RX_ADDR_P1, rxaddr, nrf_default_addr_nbytes);

    // zero all other pipes
    nrf_put8_chk(n, NRF_RX_PW_P2, 0);
    nrf_put8_chk(n, NRF_RX_PW_P3, 0);
    nrf_put8_chk(n, NRF_RX_PW_P4, 0);
    nrf_put8_chk(n, NRF_RX_PW_P5, 0);

    // setup channel, RF_CH
    nrf_put8_chk(n, NRF_RF_CH, c.channel);

    // setup data rate and power
    nrf_put8_chk(n, NRF_RF_SETUP, nrf_default_data_rate | nrf_default_db);

    // flush so that no stray packets are queued
    nrf_tx_flush(n);
    nrf_rx_flush(n);

    nrf_put8(n, NRF_STATUS, ~0);

    assert(!nrf_tx_fifo_full(n));
    assert(nrf_tx_fifo_empty(n));
    assert(!nrf_rx_fifo_full(n));
    assert(nrf_rx_fifo_empty(n));

    assert(!nrf_has_rx_intr(n));
    assert(!nrf_has_tx_intr(n));
    assert(pipeid_empty(nrf_rx_get_pipeid(n)));
    assert(!nrf_rx_has_packet(n));

    nrf_put8_chk(n, NRF_FEATURE, 0);

    // flush so that no stray packets are queued
    nrf_tx_flush(n);
    nrf_rx_flush(n);

    // power on and wait
    nrf_or8(n, NRF_CONFIG, PWR_UP << 1);
    delay_ms(2);

    // switch to RX mode
    nrf_put8_chk(n, NRF_CONFIG, rx_config);

    //set CE pin to high, p23, timing from p22
    gpio_set_on(c.ce_pin);
    delay_us(140);

    // should be true after setup.
    if(acked_p) {
        nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
        nrf_opt_assert(n, nrf_pipe_is_enabled(n, 0));
        nrf_opt_assert(n, nrf_pipe_is_enabled(n, 1));
        nrf_opt_assert(n, nrf_pipe_is_acked(n, 0));
        nrf_opt_assert(n, nrf_pipe_is_acked(n, 1));
        nrf_opt_assert(n, nrf_tx_fifo_empty(n));
    } else {
        nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
        nrf_opt_assert(n, !nrf_pipe_is_enabled(n, 0));
        nrf_opt_assert(n, nrf_pipe_is_enabled(n, 1));
        nrf_opt_assert(n, !nrf_pipe_is_acked(n, 1));
        nrf_opt_assert(n, nrf_tx_fifo_empty(n));
    }
    return n;
}

int nrf_tx_send_ack(nrf_t *n, uint32_t txaddr, 
    const void *msg, unsigned nbytes) {

    // default config for acked state.
    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    nrf_opt_assert(n, nrf_pipe_is_enabled(n, 0));
    nrf_opt_assert(n, nrf_pipe_is_enabled(n, 1));
    nrf_opt_assert(n, nrf_pipe_is_acked(n, 0));
    nrf_opt_assert(n, nrf_pipe_is_acked(n, 1));
    nrf_opt_assert(n, nrf_tx_fifo_empty(n));

    // if interrupts not enabled: make sure we check for packets.
    while(nrf_get_pkts(n))
        ;

    int res = 0;

    // 1. set device to TX mode
    nrf_put8_chk(n, NRF_CONFIG, tx_config);

    // 2. set TX addr and pipe 0 for ack
    nrf_set_addr(n, NRF_TX_ADDR, txaddr, nrf_default_addr_nbytes);

    nrf_put8_chk(n, NRF_RX_PW_P0, nbytes);
    nrf_set_addr(n, NRF_RX_ADDR_P0, txaddr, nrf_default_addr_nbytes);

    // 3. write message to device
    nrf_putn(n, NRF_W_TX_PAYLOAD, msg, nbytes);

    // 4. pulse the CE pin to actually switch to TX mode
    gpio_set_off(n->config.ce_pin);
    delay_us(140);
    gpio_set_on(n->config.ce_pin);
    delay_us(140);

    // 5. wait until the TX fifo is empty
    while(!nrf_tx_fifo_empty(n))
        ;
    
    // 6. check/clear the TX interrupt
    if (nrf_has_tx_intr(n)) {
        res = nbytes;
        nrf_tx_intr_clr(n);
    } else if (nrf_has_max_rt_intr(n)) {
        res = 0;
        nrf_rt_intr_clr(n);
    }

    // 7. set device back to RX mode
    gpio_set_off(n->config.ce_pin);
    delay_us(140);
    nrf_put8_chk(n, NRF_CONFIG, rx_config);
    gpio_set_on(n->config.ce_pin);
    delay_us(140);

    // uint8_t cnt = nrf_get8(n, NRF_OBSERVE_TX);
    // n->tot_retrans  += bits_get(cnt,0,3);

    // tx interrupt better be cleared.
    nrf_opt_assert(n, !nrf_has_tx_intr(n));
    // better be back in rx mode.
    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    return res;
}

int nrf_tx_send_noack(nrf_t *n, uint32_t txaddr, 
    const void *msg, unsigned nbytes) {

    // default state for no-ack config.
    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    nrf_opt_assert(n, !nrf_pipe_is_enabled(n, 0));
    nrf_opt_assert(n, nrf_pipe_is_enabled(n, 1));
    nrf_opt_assert(n, !nrf_pipe_is_acked(n, 1));
    nrf_opt_assert(n, nrf_tx_fifo_empty(n));

    // if interrupts not enabled: make sure we check for packets.
    while(nrf_get_pkts(n))
        ;

    // 1. set device to TX mode
    nrf_put8_chk(n, NRF_CONFIG, tx_config);

    // 2. set TX addr
    nrf_set_addr(n, NRF_TX_ADDR, txaddr, nrf_default_addr_nbytes);

    // 3. write message to device
    nrf_putn(n, NRF_W_TX_PAYLOAD, msg, nbytes);

    // 4. pulse the CE pin to actually switch to TX mode
    gpio_set_off(n->config.ce_pin);
    delay_us(140);
    gpio_set_on(n->config.ce_pin);
    delay_us(140);

    // 5. wait until the TX fifo is empty
    while(!nrf_tx_fifo_empty(n))
        ;
    
    // 6. clear the TX interrupt
    nrf_tx_intr_clr(n);

    // 7. set device back to RX mode
    gpio_set_off(n->config.ce_pin);
    delay_us(140);
    nrf_put8_chk(n, NRF_CONFIG, rx_config);
    gpio_set_on(n->config.ce_pin);
    delay_us(140);

    int res = 1;

    // tx interrupt better be cleared.
    nrf_opt_assert(n, !nrf_has_tx_intr(n));
    // better be back in rx mode.
    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    return res;
}

int nrf_get_pkts(nrf_t *n) {
    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);

    // TODO:
    // data sheet gives the sequence to follow to get packets.
    // p63: 
    //    1. read packet through spi.
    //    2. clear IRQ.
    //    3. read fifo status to see if more packets: 
    //       if so, repeat from (1) --- we need to do this now in case
    //       a packet arrives b/n (1) and (2)
    // done when: nrf_rx_fifo_empty(n)

    if(!nrf_rx_has_packet(n)) 
        return 0;

    unsigned res = 0;
    while(!nrf_rx_fifo_empty(n)) {

        unsigned pipen = nrf_rx_get_pipeid(n);
        if(pipen == NRF_PIPEID_EMPTY)
            panic("impossible: empty pipeid: %b\n", pipen);
        
        uint8_t msg[NRF_PKT_MAX];

        uint8_t status = nrf_getn(n, NRF_R_RX_PAYLOAD, msg, n->config.nbytes);
        assert(pipeid_get(status) == pipen);

        if(!cq_push_n(&n->recvq, msg, n->config.nbytes))
            panic("not enough space left for message on pipe=%d\n", pipen);

        nrf_rx_intr_clr(n);
        res++;
    }

    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    return res;
}
