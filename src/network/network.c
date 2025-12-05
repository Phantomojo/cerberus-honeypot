#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "network.h"
#include "utils.h"
#include "security_utils.h"

// Possible interface names (eth, wlan, wan, etc.)
static const char* interface_names[] = {
    "eth0", "eth1", "eth2",
    "wlan0", "wlan1", "wlan2",
    "wan0", "wan1",
    "enp0s3", "enp0s8", "enp0s9",  // Newer naming convention
    "ens0", "ens1",
    "em0", "em1",
    "ppp0", "ppp1"
};
static const int interface_names_count = sizeof(interface_names) / sizeof(interface_names[0]);

// Helper: Parse IP address into octets
static bool parse_ip(const char* ip, uint8_t* octets) {
    if (!ip || !octets) return false;
    
    // Validate IP address format first
    if (sec_validate_ip_address(ip) != SEC_VALID) {
        return false;
    }
    
    int count = sscanf(ip, "%hhu.%hhu.%hhu.%hhu",
                      &octets[0], &octets[1], &octets[2], &octets[3]);
    return count == 4;
}

// Helper: Format IP from octets
static void format_ip(uint8_t* octets, char* ip_out, size_t size) {
    if (!ip_out || size < 16) return;
    snprintf(ip_out, size, "%d.%d.%d.%d",
             octets[0], octets[1], octets[2], octets[3]);
}

// Helper: Generate random IP in subnet
static void generate_ip_in_subnet(const char* base_ip, char* ip_out, size_t size) {
    uint8_t octets[4];
    if (!parse_ip(base_ip, octets)) {
        strncpy(ip_out, base_ip, size - 1);
        return;
    }

    // Vary last octet (1-254, avoiding network and broadcast)
    octets[3] = (rand() % 253) + 1;
    format_ip(octets, ip_out, size);
}

// Helper: Generate subnet mask variations
static void get_subnet_mask(int prefix_len, char* mask_out, size_t size) {
    (void)prefix_len;  // Reserved for CIDR-based mask selection
    // Common subnet masks
    static const char* masks[] = {
        "255.0.0.0",           // /8
        "255.128.0.0",         // /9
        "255.192.0.0",         // /10
        "255.224.0.0",         // /11
        "255.240.0.0",         // /12
        "255.248.0.0",         // /13
        "255.252.0.0",         // /14
        "255.254.0.0",         // /15
        "255.255.0.0",         // /16
        "255.255.128.0",       // /17
        "255.255.192.0",       // /18
        "255.255.224.0",       // /19
        "255.255.240.0",       // /20
        "255.255.248.0",       // /21
        "255.255.252.0",       // /22
        "255.255.254.0",       // /23
        "255.255.255.0",       // /24 (most common for IoT)
        "255.255.255.128",     // /25
        "255.255.255.192",     // /26
        "255.255.255.224",     // /27
        "255.255.255.240",     // /28
        "255.255.255.248",     // /29
        "255.255.255.252",     // /30
        "255.255.255.254"      // /31
    };

    int mask_count = sizeof(masks) / sizeof(masks[0]);
    // Use /24 as most common for IoT, occasionally vary
    if (rand() % 100 < 20) {
        strncpy(mask_out, masks[rand() % mask_count], size - 1);
    } else {
        strncpy(mask_out, "255.255.255.0", size - 1);
    }
    mask_out[size - 1] = '\0';
}

// Get random interface name
char* get_random_interface_name(void) {
    return (char*)interface_names[rand() % interface_names_count];
}

// Create network configuration
network_config_t* create_network_config(const char* base_ip) {
    // Validate input
    if (sec_validate_ip_address(base_ip) != SEC_VALID) {
        return NULL;
    }
    
    network_config_t* config = (network_config_t*)malloc(sizeof(network_config_t));
    if (!config) return NULL;

    memset(config, 0, sizeof(network_config_t));

    // Primary interface
    config->interface_count = 1;
    
    // Safe string operations with validation
    const char* iface_name = get_random_interface_name();
    if (sec_safe_strcpy(config->interfaces[0].name, iface_name, MAX_INTERFACE_NAME) != SEC_VALID) {
        free(config);
        return NULL;
    }
    
    generate_ip_in_subnet(base_ip, config->interfaces[0].ip_address, MAX_IP_ADDR);
    get_subnet_mask(24, config->interfaces[0].netmask, MAX_IP_ADDR);
    
    if (sec_safe_strcpy(config->interfaces[0].gateway, base_ip, MAX_IP_ADDR) != SEC_VALID) {
        free(config);
        return NULL;
    }
    
    config->interfaces[0].mtu = 1500;
    config->interfaces[0].is_primary = true;

    // 20% chance of secondary interface
    if (rand() % 100 < 20) {
        config->interface_count = 2;
        strncpy(config->interfaces[1].name, get_random_interface_name(), MAX_INTERFACE_NAME - 1);
        snprintf(config->interfaces[1].ip_address, MAX_IP_ADDR, "10.0.%d.1", rand() % 256);
        strncpy(config->interfaces[1].netmask, "255.255.255.0", MAX_IP_ADDR - 1);
        strncpy(config->interfaces[1].gateway, "10.0.0.254", MAX_IP_ADDR - 1);
        config->interfaces[1].mtu = 1500;
        config->interfaces[1].is_primary = false;
    }

    return config;
}

// Generate interface variations
void generate_interface_variations(network_config_t* config) {
    if (!config || config->interface_count == 0) return;

    // Randomize MTU sizes occasionally
    if (rand() % 100 < 30) {
        randomize_interface_mtus(config);
    }
}

// Generate routing table variations
void generate_routing_variations(network_config_t* config) {
    if (!config || config->interface_count == 0) return;

    config->routing_count = 0;
    
    // Use temporary buffers to avoid aliasing warnings with snprintf
    char tmp_gateway[MAX_IP_ADDR];
    char tmp_interface[MAX_INTERFACE_NAME];
    snprintf(tmp_gateway, sizeof(tmp_gateway), "%s", config->interfaces[0].gateway);
    snprintf(tmp_interface, sizeof(tmp_interface), "%s", config->interfaces[0].name);

    // Default gateway route
    if (config->routing_count < MAX_ROUTING_ENTRIES) {
        snprintf(config->routing_table[config->routing_count].destination,
                 sizeof(config->routing_table[0].destination), "%s", "0.0.0.0/0");
        snprintf(config->routing_table[config->routing_count].gateway,
                 sizeof(config->routing_table[0].gateway), "%s", tmp_gateway);
        snprintf(config->routing_table[config->routing_count].interface,
                 sizeof(config->routing_table[0].interface), "%s", tmp_interface);
        config->routing_table[config->routing_count].metric = 0;
        config->routing_count++;
    }

    // Local network route
    if (config->routing_count < MAX_ROUTING_ENTRIES) {
        uint8_t octets[4];
        parse_ip(config->interfaces[0].ip_address, octets);
        snprintf(config->routing_table[config->routing_count].destination,
                 sizeof(config->routing_table[0].destination),
                 "%d.%d.%d.0/24", octets[0], octets[1], octets[2]);
        snprintf(config->routing_table[config->routing_count].gateway,
                 sizeof(config->routing_table[0].gateway),
                 "%d.%d.%d.1", octets[0], octets[1], octets[2]);
        snprintf(config->routing_table[config->routing_count].interface,
                 sizeof(config->routing_table[0].interface), "%s", tmp_interface);
        config->routing_table[config->routing_count].metric = 1;
        config->routing_count++;
    }

    // Optional additional routes (real IoT devices have these)
    for (int i = 0; i < 2 && config->routing_count < MAX_ROUTING_ENTRIES; i++) {
        if (rand() % 100 < 40) {
            snprintf(config->routing_table[config->routing_count].destination,
                     sizeof(config->routing_table[0].destination),
                     "10.%d.%d.0/24", rand() % 256, rand() % 256);
            snprintf(config->routing_table[config->routing_count].gateway,
                     sizeof(config->routing_table[0].gateway),
                     "%d.%d.%d.254", rand() % 256, rand() % 256, rand() % 256);
            snprintf(config->routing_table[config->routing_count].interface,
                     sizeof(config->routing_table[0].interface), "%s", tmp_interface);
            config->routing_table[config->routing_count].metric = 2 + i;
            config->routing_count++;
        }
    }
}

// Generate ARP cache variations
void generate_arp_variations(network_config_t* config) {
    if (!config || config->interface_count == 0) return;

    config->arp_count = 0;
    
    // Use temporary buffers to avoid aliasing warnings with snprintf
    char tmp_gateway[MAX_IP_ADDR];
    char tmp_interface[MAX_INTERFACE_NAME];
    char tmp_ip[MAX_IP_ADDR];
    snprintf(tmp_gateway, sizeof(tmp_gateway), "%s", config->interfaces[0].gateway);
    snprintf(tmp_interface, sizeof(tmp_interface), "%s", config->interfaces[0].name);
    snprintf(tmp_ip, sizeof(tmp_ip), "%s", config->interfaces[0].ip_address);

    // Add gateway to ARP (always present)
    if (config->arp_count < MAX_ARP_ENTRIES) {
        snprintf(config->arp_cache[config->arp_count].ip,
                 sizeof(config->arp_cache[0].ip), "%s", tmp_gateway);
        snprintf(config->arp_cache[config->arp_count].mac, 32,
                 "%02x:%02x:%02x:%02x:%02x:%02x",
                 rand() % 256, rand() % 256, rand() % 256,
                 rand() % 256, rand() % 256, rand() % 256);
        snprintf(config->arp_cache[config->arp_count].interface,
                 sizeof(config->arp_cache[0].interface), "%s", tmp_interface);
        config->arp_cache[config->arp_count].is_permanent = true;
        config->arp_count++;
    }

    // Add some random IPs (dynamic ARP entries)
    int random_entries = 2 + (rand() % 5);
    for (int i = 0; i < random_entries && config->arp_count < MAX_ARP_ENTRIES; i++) {
        uint8_t octets[4];
        parse_ip(tmp_ip, octets);
        octets[3] = (rand() % 253) + 1;  // Avoid .0 and .255

        format_ip(octets, config->arp_cache[config->arp_count].ip,
                  sizeof(config->arp_cache[0].ip));
        snprintf(config->arp_cache[config->arp_count].mac, 32,
                 "%02x:%02x:%02x:%02x:%02x:%02x",
                 rand() % 256, rand() % 256, rand() % 256,
                 rand() % 256, rand() % 256, rand() % 256);
        snprintf(config->arp_cache[config->arp_count].interface,
                 sizeof(config->arp_cache[0].interface), "%s", tmp_interface);
        config->arp_cache[config->arp_count].is_permanent = false;
        config->arp_count++;
    }
}

// Randomize interface names
void randomize_interface_names(network_config_t* config) {
    if (!config) return;
    for (int i = 0; i < config->interface_count; i++) {
        snprintf(config->interfaces[i].name, sizeof(config->interfaces[0].name), 
                 "%s", get_random_interface_name());
    }
}

// Randomize MTU sizes
void randomize_interface_mtus(network_config_t* config) {
    if (!config) return;

    static const uint32_t common_mtus[] = { 1500, 1492, 1480, 1472, 1460, 1454, 1400, 1280, 576 };
    static const int mtu_count = sizeof(common_mtus) / sizeof(common_mtus[0]);

    for (int i = 0; i < config->interface_count; i++) {
        config->interfaces[i].mtu = common_mtus[rand() % mtu_count];
    }
}

// Generate ifconfig output
int generate_ifconfig_output(network_config_t* config, char* output, size_t output_size) {
    if (!config || !output || output_size < 512) return -1;

    output[0] = '\0';
    char buffer[2048];

    for (int i = 0; i < config->interface_count; i++) {
        network_interface_t* iface = &config->interfaces[i];
        snprintf(buffer, sizeof(buffer),
                 "%s: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu %u\n"
                 "\tinet %s  netmask %s  broadcast %s\n"
                 "\tether %02x:%02x:%02x:%02x:%02x:%02x  txqueuelen %d\n"
                 "\tRX packets:%u bytes:%u\n"
                 "\tTX packets:%u bytes:%u\n\n",
                 iface->name,
                 iface->mtu,
                 iface->ip_address,
                 iface->netmask,
                 "192.168.1.255",  // Would compute actual broadcast
                 rand() % 256, rand() % 256, rand() % 256,
                 rand() % 256, rand() % 256, rand() % 256,
                 1000 + (rand() % 500),
                 1000 + (rand() % 100000),
                 50000 + (rand() % 5000000),
                 500 + (rand() % 50000),
                 20000 + (rand() % 2000000));

        if (strlen(output) + strlen(buffer) < output_size) {
            strcat(output, buffer);
        }
    }

    return strlen(output);
}

// Generate route output
int generate_route_output(network_config_t* config, char* output, size_t output_size) {
    if (!config || !output || output_size < 512) return -1;

    output[0] = '\0';
    snprintf(output, output_size,
             "Kernel IP routing table\n"
             "Destination     Gateway         Genmask         Flags Metric Ref    Use Iface\n");

    char buffer[256];
    for (int i = 0; i < config->routing_count && i < MAX_ROUTING_ENTRIES; i++) {
        routing_entry_t* route = &config->routing_table[i];
        snprintf(buffer, sizeof(buffer),
                 "%-15s %-15s %-15s %-5s %-6u %-4d %-3d %s\n",
                 route->destination,
                 route->gateway,
                 "255.255.255.0",  // Simplified
                 "UG",
                 route->metric,
                 0,
                 0,
                 route->interface);

        if (strlen(output) + strlen(buffer) < output_size) {
            strcat(output, buffer);
        }
    }

    return strlen(output);
}

// Generate ARP output
int generate_arp_output(network_config_t* config, char* output, size_t output_size) {
    if (!config || !output || output_size < 512) return -1;

    output[0] = '\0';
    snprintf(output, output_size,
             "Address                  HWtype  HWaddress           Flags Mask            Iface\n");

    char buffer[256];
    for (int i = 0; i < config->arp_count && i < MAX_ARP_ENTRIES; i++) {
        arp_entry_t* arp = &config->arp_cache[i];
        snprintf(buffer, sizeof(buffer),
                 "%-24s %-7s %-19s %-5s %-15s %s\n",
                 arp->ip,
                 "ether",
                 arp->mac,
                 arp->is_permanent ? "PERM" : "C",
                 "*",
                 arp->interface);

        if (strlen(output) + strlen(buffer) < output_size) {
            strcat(output, buffer);
        }
    }

    return strlen(output);
}

// Generate netstat output (simplified)
int generate_netstat_output(network_config_t* config, char* output, size_t output_size) {
    if (!output || output_size < 256) return -1;

    snprintf(output, output_size,
             "Active Internet connections (servers and established)\n"
             "Proto Recv-Q Send-Q Local Address           Foreign Address         State\n"
             "tcp        0      0 %s:ssh                 0.0.0.0:*               LISTEN\n"
             "tcp        0      0 %s:80                  0.0.0.0:*               LISTEN\n"
             "tcp6       0      0 [::]:ssh                [::]:*                  LISTEN\n"
             "tcp6       0      0 [::]:http               [::]:*                  LISTEN\n",
             config->interfaces[0].ip_address,
             config->interfaces[0].ip_address);

    return strlen(output);
}

// Serialize network config to JSON
char* serialize_network_config(network_config_t* config) {
    if (!config) return NULL;

    char* json = malloc(4096);
    if (!json) return NULL;

    snprintf(json, 4096,
             "{\n"
             "  \"interfaces\": [\n");

    for (int i = 0; i < config->interface_count; i++) {
        network_interface_t* iface = &config->interfaces[i];
        char entry[512];
        snprintf(entry, sizeof(entry),
                 "    {\n"
                 "      \"name\": \"%s\",\n"
                 "      \"ip\": \"%s\",\n"
                 "      \"netmask\": \"%s\",\n"
                 "      \"gateway\": \"%s\",\n"
                 "      \"mtu\": %u,\n"
                 "      \"primary\": %s\n"
                 "    }%s\n",
                 iface->name, iface->ip_address, iface->netmask,
                 iface->gateway, iface->mtu,
                 iface->is_primary ? "true" : "false",
                 i < config->interface_count - 1 ? "," : "");
        strcat(json, entry);
    }

    strcat(json, "  ]\n}\n");
    return json;
}

// Free network configuration
void free_network_config(network_config_t* config) {
    if (config) {
        free(config);
    }
}
