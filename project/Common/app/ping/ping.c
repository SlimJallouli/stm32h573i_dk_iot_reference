#if (defined(ETHERNET) || defined(MXCHIP))
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/icmp.h"
#include "lwip/ip4.h"
#include "lwip/netdb.h"
#include "lwip/inet_chksum.h"
#include "lwip/sys.h"
#endif

#if defined(ST67W6X_NCP)
#define u16_t uint16_t
#include "w6x_lwip_port.h"
#endif

#include "logging.h"
#include "sys_evt.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

#include "kvstore.h"

#define PING_INTERVAL_MS   1000
#define PING_PAYLOAD_SIZE  32
#define PING_TIMEOUT_MS    1000
#define PING_COUNT         4  // Limit number of pings to 4

static u16_t ping_seq_num;

static void resolve_hostname(const char *hostname, ip_addr_t *addr)
{
    struct hostent *host_info = lwip_gethostbyname(hostname);
    if (host_info && host_info->h_addr_list[0])
    {
        memcpy(addr, host_info->h_addr_list[0], sizeof(ip_addr_t));
        LogInfo("Resolved hostname %s to IP: %s", hostname, inet_ntoa(*addr));
    }
    else
    {
        LogError("Failed to resolve hostname: %s", hostname);
        vTaskDelete(NULL);
    }
}

static void ping_prepare_echo(struct icmp_echo_hdr *iecho, u16_t len)
{
	size_t i;
	size_t data_len = len - sizeof(struct icmp_echo_hdr);
	ICMPH_TYPE_SET(iecho, ICMP_ECHO);
	ICMPH_CODE_SET(iecho, 0);
	iecho->chksum = 0;
	iecho->id = 0xAFAF;
	iecho->seqno = htons(++ping_seq_num);

	for (i = 0; i < data_len; i++)
	{
		((char*) iecho)[sizeof(struct icmp_echo_hdr) + i] = (char) i;
	}

	iecho->chksum = inet_chksum(iecho, len);
}

static void ping_send(int s, ip_addr_t *addr)
{
    struct icmp_echo_hdr *iecho;
    struct sockaddr_in to;
    size_t ping_size = sizeof(struct icmp_echo_hdr) + PING_PAYLOAD_SIZE;

    LogDebug("Allocating memory for ICMP Echo Request (size: %zu)", ping_size);
    iecho = (struct icmp_echo_hdr*) mem_malloc((mem_size_t) ping_size);

    if (!iecho)
    {
        LogError("Failed to allocate memory for ICMP Echo Request");
        return;
    }

    ping_prepare_echo(iecho, (u16_t) ping_size);
    LogDebug("Prepared ICMP Echo Request: ID = %x, Seq = %d", iecho->id, htons(iecho->seqno));

    to.sin_len = sizeof(to);
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = ipaddr_addr(ipaddr_ntoa(addr));

    LogDebug("Sending ICMP Echo Request to %s", inet_ntoa(to.sin_addr));
    lwip_sendto(s, iecho, ping_size, 0, (struct sockaddr*) &to, sizeof(to));

    LogDebug("ICMP Echo Request sent successfully");
    mem_free(iecho);
}

static void ping_recv(int s)
{
    char buf[64];
    int fromlen, len;
    struct sockaddr_in from;
    struct ip_hdr *iphdr;
    struct icmp_echo_hdr *iecho;
    uint32_t ping_time_sent = sys_now();  // Record send time
    uint32_t ping_time_received;

    while ((len = lwip_recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*)&from, (socklen_t*)&fromlen)) > 0)
    {
        ping_time_received = sys_now();  // Record receive time
        uint32_t round_trip_time = ping_time_received - ping_time_sent;

        iphdr = (struct ip_hdr*) buf;
        iecho = (struct icmp_echo_hdr*) (buf + (IPH_HL(iphdr) * 4));

        if (len >= (int) (sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr)))
        {
            if ((iecho->id == 0xAFAF) && (iecho->seqno == htons(ping_seq_num)))
            {
                // Extract TTL from IP header
                uint8_t ttl = iphdr->_ttl;
                LogInfo("Reply from %s: bytes=%d time=%dms TTL=%d", inet_ntoa(from.sin_addr), len, round_trip_time, ttl);
                return;
            }
        }
    }

    LogError("Ping reply reception failed or timed out");
}

void ping_task(void *pvParameters)
{
    int s;
    int timeout = PING_TIMEOUT_MS;
    ip_addr_t ping_target;
    char *PING_TARGET_HOST;
    size_t TARGET_HOST_LENGTH;
    int ping_count = 0;  // Track number of pings

    LogInfo("%s started", __func__);
    (void) pvParameters;

    /* Wait until connected to the network */
    LogInfo("Waiting for network connection...");
    (void) xEventGroupWaitBits(xSystemEvents, EVT_MASK_NET_CONNECTED, pdFALSE, pdTRUE, portMAX_DELAY);

    LogInfo("Network connected");

    PING_TARGET_HOST = KVStore_getStringHeap(CS_CORE_MQTT_ENDPOINT, &TARGET_HOST_LENGTH);
    resolve_hostname(PING_TARGET_HOST, &ping_target);

    if ((s = lwip_socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
    {
        LogError("Failed to create socket");
        vTaskDelete(NULL);
    }

    LogDebug("Setting socket receive timeout to %d ms", timeout);
    lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    while (ping_count < PING_COUNT)  // Limit to PING_COUNT pings
    {
        LogDebug("Sending ping %d...", ping_count + 1);
        ping_send(s, &ping_target);

        LogDebug("Waiting for ping reply...");
        ping_recv(s);

        ping_count++;  // Increment ping counter
        vTaskDelay(pdMS_TO_TICKS(PING_INTERVAL_MS));
    }

    LogInfo("Ping process completed. Terminating task.");
    vTaskDelete(NULL);  // Cleanly exit task after PING_COUNT pings
}
