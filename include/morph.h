#ifndef MORPH_H
#define MORPH_H

#define MAX_PROFILES 10
#define MAX_PROFILE_NAME 64
#define MAX_BANNER_SIZE 512
#define MAX_PATH_SIZE 512
#define MAX_KERNEL_VERSION 128
#define MAX_MAC_ADDR 32

typedef struct {
    char name[MAX_PROFILE_NAME];
    char ssh_banner[MAX_BANNER_SIZE];
    char telnet_banner[MAX_BANNER_SIZE];
    char router_html_path[MAX_PATH_SIZE];
    char camera_html_path[MAX_PATH_SIZE];
    char kernel_version[MAX_KERNEL_VERSION];
    char arch[MAX_PROFILE_NAME];
    char mac_address[MAX_MAC_ADDR];
    int memory_mb;
    int cpu_mhz;
} device_profile_t;

// Profile management
int load_profiles(const char* config_file);
int get_profile_count(void);
device_profile_t* get_profile(int index);
int get_current_profile_index(void);
int set_current_profile(int index);

// Morphing functions
int morph_device(void);
int morph_cowrie_banners(const device_profile_t* profile);
int morph_router_html(const device_profile_t* profile);
int morph_camera_html(const device_profile_t* profile);
int save_current_profile(const char* state_file);

// Session variation functions
void generate_random_mac(char* mac_out, size_t size, const char* vendor_prefix);
int generate_session_variations(const device_profile_t* profile);

// Initialization
int init_morph_engine(const char* config_file, const char* state_file);

#endif // MORPH_H

