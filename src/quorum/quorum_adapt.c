/**
 * quorum_adapt.c - Quorum-based adaptation module
 * 
 * Detects coordinated attacks and triggers adaptive responses:
 * - Increase morphing frequency
 * - Add response delays
 * - Fake errors to confuse attackers
 * - Disconnect suspected coordinated attacks
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "quorum_adapt.h"
#include "utils.h"

// Known attack signatures
static const char* brute_force_signatures[] = {
    "Failed password for",
    "Invalid user",
    "Received disconnect",
    "Connection refused"
};

static const char* exploitation_signatures[] = {
    "buffer overflow",
    "injection",
    "shellcode",
    "ROP gadget",
    "privilege escalation"
};

static const char* reconnaissance_signatures[] = {
    "nmap",
    "masscan",
    "shodan",
    "censys",
    "port scan",
    "service discovery"
};

// Current threat metrics
static uint32_t current_morph_frequency_minutes = 360;  // Default: 6 hours
static uint32_t current_command_delay_ms = 0;           // Default: none
static bool emergency_mode = false;

/**
 * Create attack pattern
 */
attack_pattern_t* create_attack_pattern(const char* name) {
    attack_pattern_t* pattern = (attack_pattern_t*)malloc(sizeof(attack_pattern_t));
    if (!pattern) return NULL;

    memset(pattern, 0, sizeof(attack_pattern_t));
    pattern->pattern_name = name;
    pattern->severity = 5;
    pattern->first_seen = time(NULL);
    pattern->last_seen = time(NULL);

    return pattern;
}

/**
 * Create attacker profile
 */
attacker_profile_t* create_attacker_profile(const char* ip) {
    attacker_profile_t* profile = (attacker_profile_t*)malloc(sizeof(attacker_profile_t));
    if (!profile) return NULL;

    memset(profile, 0, sizeof(attacker_profile_t));
    if (ip) strncpy(profile->ip_address, ip, 31);
    profile->first_contact = time(NULL);
    profile->last_contact = time(NULL);

    return profile;
}

/**
 * Calculate threat score for an attacker (0.0-1.0)
 */
float calculate_threat_score(attacker_profile_t* attacker) {
    if (!attacker) return 0.0f;

    float score = 0.0f;

    // Frequency of attempts (higher = more threatening)
    if (attacker->total_attempts > 0) {
        score += (float)(attacker->total_attempts % 100) / 100.0f;
    }

    // Success rate (exploits are very bad)
    if (attacker->successful_exploits > 0) {
        score += (float)(attacker->successful_exploits * 10) / 100.0f;
    }

    // Is part of coordinated attack?
    if (attacker->is_coordinated) {
        score += 0.3f;
    }

    return score > 1.0f ? 1.0f : score;
}

/**
 * Detect if attackers are working together
 */
void detect_coordination(attacker_profile_t** attackers, int attacker_count) {
    if (!attackers || attacker_count < 2) return;

    // Look for similar patterns happening at similar times
    for (int i = 0; i < attacker_count; i++) {
        for (int j = i + 1; j < attacker_count; j++) {
            // Check if patterns overlap in time (within 5 minutes)
            time_t time_diff = abs((int)(attackers[i]->last_contact - attackers[j]->last_contact));
            
            // Similar pattern types and close timing = coordinated
            if (time_diff < 300 && attackers[i]->pattern_count > 0 && 
                attackers[j]->pattern_count > 0) {
                attackers[i]->is_coordinated = true;
                attackers[j]->is_coordinated = true;

                char msg[256];
                snprintf(msg, sizeof(msg), "Coordinated attack detected: %s and %s",
                         attackers[i]->ip_address, attackers[j]->ip_address);
                log_event_level(LOG_WARN, msg);
            }
        }
    }
}

/**
 * Check if there's a coordinated attack in progress
 */
bool is_coordinated_attack(attacker_profile_t** attackers, int attacker_count) {
    if (!attackers || attacker_count == 0) return false;

    int coordinated_count = 0;
    for (int i = 0; i < attacker_count; i++) {
        if (attackers[i]->is_coordinated) {
            coordinated_count++;
        }
    }

    // 2+ coordinated attackers = coordinated attack
    return coordinated_count >= 2;
}

/**
 * Assess overall threat level
 */
threat_assessment_t* assess_threat_level(attacker_profile_t** attackers, int attacker_count) {
    threat_assessment_t* threat = (threat_assessment_t*)malloc(sizeof(threat_assessment_t));
    if (!threat) return NULL;

    memset(threat, 0, sizeof(threat_assessment_t));
    threat->assessment_time = time(NULL);
    threat->total_unique_attackers = attacker_count;

    if (attacker_count == 0) {
        threat->overall_threat_level = 0.0f;
        return threat;
    }

    // Calculate average threat score
    float total_score = 0.0f;
    int coordinated = 0;

    for (int i = 0; i < attacker_count; i++) {
        total_score += calculate_threat_score(attackers[i]);
        if (attackers[i]->is_coordinated) coordinated++;
    }

    threat->overall_threat_level = total_score / attacker_count;
    threat->coordinated_attack_count = coordinated / 2;  // Pairs
    threat->should_trigger_rapid_morph = (threat->overall_threat_level > 0.6f) ||
                                         is_coordinated_attack(attackers, attacker_count);

    return threat;
}

/**
 * Get appropriate response action
 */
response_action_t get_appropriate_response(threat_assessment_t* threat) {
    if (!threat) return RESPONSE_NONE;

    if (threat->should_trigger_rapid_morph) {
        return RESPONSE_INCREASE_MORPHING_FREQUENCY;
    }

    if (threat->overall_threat_level > 0.7f) {
        return RESPONSE_FAKE_ERRORS;
    }

    if (threat->overall_threat_level > 0.5f) {
        return RESPONSE_ADD_DELAYS;
    }

    if (threat->coordinated_attack_count > 0) {
        return RESPONSE_DISCONNECT;
    }

    return RESPONSE_NONE;
}

/**
 * Update attack pattern with new log entry
 */
void update_attack_pattern(attack_pattern_t* pattern, const char* log_entry) {
    if (!pattern || !log_entry) return;

    pattern->occurrence_count++;
    pattern->last_seen = time(NULL);

    // Increase severity based on frequency
    if (pattern->occurrence_count > 100) pattern->severity = 9;
    else if (pattern->occurrence_count > 50) pattern->severity = 8;
    else if (pattern->occurrence_count > 20) pattern->severity = 7;
    else if (pattern->occurrence_count > 10) pattern->severity = 6;
}

/**
 * Trigger emergency morphing
 */
void trigger_emergency_morph(void) {
    char msg[256];
    snprintf(msg, sizeof(msg), "Emergency morphing triggered due to coordinated attack");
    log_event_level(LOG_WARN, msg);
    
    // In real implementation, would:
    // 1. Immediately call morph engine
    // 2. Change all device fingerprints
    // 3. Reset all sessions
    // 4. Clear attack logs
    
    emergency_mode = true;
}

/**
 * Increase morphing frequency
 */
void increase_morphing_frequency(uint32_t new_frequency_minutes) {
    if (new_frequency_minutes < 1) new_frequency_minutes = 1;
    
    current_morph_frequency_minutes = new_frequency_minutes;
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Morphing frequency increased to every %u minutes",
             new_frequency_minutes);
    log_event_level(LOG_WARN, msg);

    // In real implementation, would update systemd timer
}

/**
 * Add command delays
 */
void add_command_delays(uint32_t delay_ms) {
    current_command_delay_ms = delay_ms;
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Adding %u ms delays to all command responses", delay_ms);
    log_event_level(LOG_WARN, msg);
}

/**
 * Simulate errors for specific attacker IP
 */
void simulate_errors_for_attacker(const char* ip) {
    if (!ip) return;

    char msg[256];
    snprintf(msg, sizeof(msg), "Enabling error simulation for attacker: %s", ip);
    log_event_level(LOG_WARN, msg);

    // In real implementation, would:
    // 1. Track attacker IP
    // 2. Return errors to their commands
    // 3. Appear system is broken/offline to them
    // 4. Log all their attempts
}

/**
 * Log attack intelligence
 */
void log_attack_intelligence(attack_pattern_t* pattern, attacker_profile_t* attacker) {
    if (!pattern || !attacker) return;

    char intel_file[512];
    snprintf(intel_file, sizeof(intel_file), "build/attack-intel-%ld.log", time(NULL));

    FILE* f = fopen(intel_file, "a");
    if (!f) return;

    fprintf(f, "=== Attack Intelligence ===\n");
    fprintf(f, "Timestamp: %ld\n", time(NULL));
    fprintf(f, "Attacker IP: %s\n", attacker->ip_address);
    fprintf(f, "Pattern: %s\n", pattern->pattern_name);
    fprintf(f, "Severity: %u/10\n", pattern->severity);
    fprintf(f, "Occurrences: %u\n", pattern->occurrence_count);
    fprintf(f, "Attacker Attempts: %u\n", attacker->total_attempts);
    fprintf(f, "Successful Exploits: %u\n", attacker->successful_exploits);
    fprintf(f, "Coordinated: %s\n", attacker->is_coordinated ? "YES" : "NO");
    fprintf(f, "\n");

    fclose(f);
}

/**
 * Free attack pattern
 */
void free_attack_pattern(attack_pattern_t* pattern) {
    if (pattern) {
        free(pattern);
    }
}

/**
 * Free attacker profile
 */
void free_attacker_profile(attacker_profile_t* profile) {
    if (profile) {
        free(profile);
    }
}

/**
 * Export current threat metrics
 */
uint32_t get_current_morph_frequency(void) {
    return current_morph_frequency_minutes;
}

uint32_t get_current_command_delay(void) {
    return current_command_delay_ms;
}

bool is_in_emergency_mode(void) {
    return emergency_mode;
}

void reset_emergency_mode(void) {
    emergency_mode = false;
    current_morph_frequency_minutes = 360;  // Back to 6 hours
    current_command_delay_ms = 0;
    char msg[256];
    snprintf(msg, sizeof(msg), "Emergency mode disabled, returning to normal operation");
    log_event_level(LOG_INFO, msg);
}
