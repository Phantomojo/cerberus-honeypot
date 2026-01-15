#ifndef QUORUM_ADAPT_H
#define QUORUM_ADAPT_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define MAX_ATTACK_PATTERNS 50
#define MAX_ATTACKERS 100

// Attack pattern signature
typedef struct {
    const char* pattern_name;
    const char* signature[10]; // Multiple strings that identify this pattern
    int signature_count;
    uint32_t severity; // 1-10, 10 being most severe
    uint32_t occurrence_count;
    time_t first_seen;
    time_t last_seen;
} attack_pattern_t;

// Attacker profile
typedef struct {
    char ip_address[32];
    uint32_t total_attempts;
    uint32_t failed_attempts;
    uint32_t successful_exploits;
    time_t first_contact;
    time_t last_contact;
    bool is_coordinated; // Part of coordinated attack?
    attack_pattern_t* patterns[10];
    int pattern_count;
} attacker_profile_t;

// Global threat assessment
typedef struct {
    uint32_t total_unique_attackers;
    uint32_t coordinated_attack_count;
    float overall_threat_level; // 0.0-1.0
    time_t assessment_time;
    bool should_trigger_rapid_morph;
} threat_assessment_t;

// Response action
typedef enum {
    RESPONSE_NONE,
    RESPONSE_INCREASE_MORPHING_FREQUENCY,
    RESPONSE_ADD_DELAYS,
    RESPONSE_FAKE_ERRORS,
    RESPONSE_DISCONNECT,
    RESPONSE_HONEYPOT_LOCK
} response_action_t;

// Functions
attack_pattern_t* create_attack_pattern(const char* name);
attacker_profile_t* create_attacker_profile(const char* ip);
threat_assessment_t* assess_threat_level(attacker_profile_t** attackers, int attacker_count);
void detect_coordination(attacker_profile_t** attackers, int attacker_count);
void update_attack_pattern(attack_pattern_t* pattern, const char* log_entry);
bool is_coordinated_attack(attacker_profile_t** attackers, int attacker_count);
response_action_t get_appropriate_response(threat_assessment_t* threat);
void trigger_emergency_morph(void);
void increase_morphing_frequency(uint32_t new_frequency_minutes);
void add_command_delays(uint32_t delay_ms);
void simulate_errors_for_attacker(const char* ip);
void log_attack_intelligence(attack_pattern_t* pattern, attacker_profile_t* attacker);
float calculate_threat_score(attacker_profile_t* attacker);
void free_attack_pattern(attack_pattern_t* pattern);
void free_attacker_profile(attacker_profile_t* profile);

#endif // QUORUM_ADAPT_H
