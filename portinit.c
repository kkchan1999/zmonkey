#include "portinit.h"
#include <string.h>

int port_init(uint16_t port, struct rte_mempool **mbuf_pool, uint16_t num_rx_queue, uint16_t num_tx_queue)
{

    uint16_t nb_rxd = RX_RING_SIZE;
    uint16_t nb_txd = TX_RING_SIZE;
    int retval;
    uint16_t q;
    bool is_virtio_port = false;
    struct rte_eth_dev_info dev_info;
    struct rte_eth_txconf txconf;

    if (!rte_eth_dev_is_valid_port(port))
        return -1;

    char *dev_name = (char *)malloc(sizeof(char) * RTE_DEV_NAME_MAX_LEN);
    retval = rte_eth_dev_get_name_by_port(port, dev_name);
    if (retval != 0)
    {
        printf("failed to find port[%d] name\n", port);
        return 0;
    }

    retval = rte_eth_dev_info_get(port, &dev_info);

    if (retval != 0)
    {
        printf("Error during getting device (port %u) info: %s\n",
               port, strerror(-retval));
        return retval;
    }

    struct rte_eth_conf port_conf = {
        .rxmode = {
            .mtu = RTE_ETHER_MAX_LEN,
            .mq_mode = RTE_ETH_MQ_RX_RSS,
        },
        .rx_adv_conf = {
            .rss_conf = {
                .rss_key = NULL,
                .rss_hf = dev_info.flow_type_rss_offloads & RTE_ETH_RSS_PROTO_MASK, //能用什么直接全部用用上了...
            },
        },
        .txmode = {
            .mq_mode = RTE_ETH_MQ_TX_NONE,
        }};

    printf("\n\ninitializing port %d with(rxq:%d txq:%d)...\n", port, num_rx_queue, num_tx_queue);


    if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE)
    {
        printf("port[%u] support TX mbuf fast free offload.\n", port);
        port_conf.txmode.offloads |= RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;
    }

    if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MT_LOCKFREE)
    {
        printf("port[%u] support TX MT lock free offload.\n", port);
        port_conf.txmode.offloads |= RTE_ETH_TX_OFFLOAD_MT_LOCKFREE;
    }

    /* Configure the Ethernet device. */
    retval = rte_eth_dev_configure(port, num_rx_queue, num_tx_queue, &port_conf);
    if (retval != 0)
        return retval;

    retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
    if (retval != 0)
        return retval;

    /* Allocate and set up multiple RX queue per Ethernet port. */
    for (q = 0; q < num_rx_queue; q++)
    {
        retval = rte_eth_rx_queue_setup(port, q, nb_rxd,
                                        rte_eth_dev_socket_id(port), NULL, mbuf_pool[q]);
        if (retval < 0)
            return retval;
    }

    txconf = dev_info.default_txconf;
    txconf.offloads = port_conf.txmode.offloads;

    /* Allocate and set up multiple TX queue per Ethernet port. */
    for (q = 0; q < num_tx_queue; q++)
    {
        retval = rte_eth_tx_queue_setup(port, q, nb_txd,
                                        rte_eth_dev_socket_id(port), &txconf);
        if (retval < 0)
            return retval;
    }

    /* Start the Ethernet port. */
    retval = rte_eth_dev_start(port);
    if (retval < 0)
        return retval;

     /* Wait for Link up
    struct rte_eth_link link;
    do
    {
        retval = rte_eth_link_get_nowait(port, &link);
        if (retval < 0)
        {
            printf("Failed link get (port %u): %s\n",
                   port, rte_strerror(-retval));
            return retval;
        }
        else if (link.link_status)
            break;

        printf("Waiting for Link up on port %" PRIu16 "\n", port);
        sleep(1);
    } while (!link.link_status);
    */

    /* Display the port MAC address. */
    struct rte_ether_addr addr;
    retval = rte_eth_macaddr_get(port, &addr);
    if (retval != 0)
        return retval;

    printf("Port[%u] MAC: %02" PRIx8 ":%02" PRIx8 ":%02" PRIx8
           ":%02" PRIx8 ":%02" PRIx8 ":%02" PRIx8 "\n",
           port,
           addr.addr_bytes[0], addr.addr_bytes[1],
           addr.addr_bytes[2], addr.addr_bytes[3],
           addr.addr_bytes[4], addr.addr_bytes[5]);

    /* Enable RX in promiscuous mode for the Ethernet device. */

    retval = rte_eth_promiscuous_enable(port);
    if (retval != 0)
        return retval;

    return 0;
}
