#ifndef NETWORK_H
#define NETWORK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_IP_ADDR 32
#define MAX_INTERFACE_NAME 16
#define MAX_ROUTING_ENTRIES 10
#define MAX_ARP_ENTRIES 20

// Network interface structure
typedef struct {
    char name[MAX_INTERFACE_NAME]; // eth0, wlan0, wan0, etc.
    char ip_address[MAX_IP_ADDR];  // 192.168.1.1
    char netmask[MAX_IP_ADDR];     // 255.255.255.0
    char gateway[MAX_IP_ADDR];     // 192.168.1.254
    uint32_t mtu;                  // MTU size (1500, 1492, etc.)
    bool is_primary;               // Primary interface?
} network_interface_t;

// Routing table entry
typedef struct {
    char destination[MAX_IP_ADDR];      // 0.0.0.0/0 or 192.168.1.0/24
    char gateway[MAX_IP_ADDR];          // Via gateway IP
    char interface[MAX_INTERFACE_NAME]; // eth0, wlan0, etc.
    uint32_t metric;                    // Route metric (1-9999)
} routing_entry_t;

// ARP cache entry
typedef struct {
    char ip[MAX_IP_ADDR];               // IP address
    char mac[32];                       // MAC address
    char interface[MAX_INTERFACE_NAME]; // Interface
    bool is_permanent;                  // Permanent or dynamic entry?
} arp_entry_t;

// Network configuration container
typedef struct {
    network_interface_t interfaces[5]; // Up to 5 interfaces
    int interface_count;
    routing_entry_t routing_table[MAX_ROUTING_ENTRIES];
    int routing_count;
    arp_entry_t arp_cache[MAX_ARP_ENTRIES];
    int arp_count;
} network_config_t;

// Network variation functions
network_config_t* create_network_config(const char* base_ip);
void generate_interface_variations(network_config_t* config);
void generate_routing_variations(network_config_t* config);
void generate_arp_variations(network_config_t* config);
void randomize_interface_names(network_config_t* config);
void randomize_interface_mtus(network_config_t* config);
char* get_random_interface_name(void);
char* serialize_network_config(network_config_t* config);
void free_network_config(network_config_t* config);

// Generate network data for Cowrie
int generate_ifconfig_output(network_config_t* config, char* output, size_t output_size);
int generate_route_output(network_config_t* config, char* output, size_t output_size);
int generate_arp_output(network_config_t* config, char* output, size_t output_size);
int generate_netstat_output(network_config_t* config, char* output, size_t output_size);

#endif // NETWORK_H
