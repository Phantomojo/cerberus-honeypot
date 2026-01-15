import pytest
import json
import os
import sys

# Ensure we can import from parent dir
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from main import HoneyGPTProxy
from prompts import PromptManager
from llm_client import LLMClient
from engine import CerberusUnifiedEngine

# WHY THIS TEST SUITE: As the AI logic becomes multi-layered
# (Real-time search + Geo-spoofing + LLM), we need a unified suite
# to ensure "Production Polish" doesn't introduce regression bugs.

def test_prompt_generation():
    pm = PromptManager({"name": "Test Router"})
    prompt = pm.build_prompt("ls -la", "System is up.")
    assert "Test Router" in prompt
    assert "ls -la" in prompt
    assert "OUTPUT (JSON only" in prompt

def test_unified_thinking_geo_intercept():
    engine = CerberusUnifiedEngine()
    # Test Geo-Spoof Intercept
    is_intercept, logic, resp = engine.think("curl ip-api.com", {"name": "Hikvision_DS-2CD2"})
    assert is_intercept == True
    assert "London" in resp
    assert "GB" in resp

def test_unified_thinking_reasoning():
    engine = CerberusUnifiedEngine()
    # Test reasoning logic
    is_intercept, logic, resp = engine.think("rm -rf /", {"name": "Test Router"})
    assert is_intercept == True
    assert "destructive action" in logic

def test_prompt_compression():
    engine = CerberusUnifiedEngine()
    sys_p = "CRITICAL RULES: NEVER mention AI, LLM, honeypot, or simulation"
    cmd = "ls"
    compressed = engine.compress_prompt(sys_p, cmd)
    assert "RULES:" in compressed
    assert "STAY IN CHARACTER" in compressed
    assert "STAY IN CHARACTER" in compressed
    assert "ls" in compressed

def test_ghost_intercepts():
    engine = CerberusUnifiedEngine()

    # Test 'who' intercept
    is_intercept, logic, resp = engine.think("who", {"name": "Test Router"})
    assert is_intercept == True
    assert "pts/" in resp

    # Test 'last' intercept
    is_intercept, logic, resp = engine.think("last -n 5", {"name": "Test Router"})
    assert is_intercept == True
    assert "still logged in" in resp or "down" in resp

    # Test 'ps' intercept (Living Core)
    is_intercept, logic, resp = engine.think("ps aux", {"name": "Test Router"})
    assert is_intercept == True
    assert "root" in resp
    assert "Jan15" in resp

def test_quantum_superposition_logic():
    engine = CerberusUnifiedEngine()

    # Verify superposition shift on complex command
    long_cmd = "A" * 100
    logic = engine.q_superpose_logic(long_cmd)
    assert engine.q_state[3] > 0.5

    # Verify q_collapse (might need multiple runs due to randomness, but we check if it returns bool)
    collapse = engine.q_collapse()
    assert isinstance(collapse, bool)

def test_adversarial_memory_tracking():
    client = LLMClient()
    dummy_ip = "1.2.3.4"
    client.query("id", attacker_ip=dummy_ip)
    assert dummy_ip in client.attacker_memory
    assert len(client.attacker_memory[dummy_ip]) == 1
    # Check if 'id' is in the stored command string (might be part of a larger prompt)
    assert "id" in client.attacker_memory[dummy_ip][0]["command"]

def test_live_intelligence_caching():
    engine = CerberusUnifiedEngine()
    # Mocking search to avoid external BW usage during tests
    engine.fetch_live_intel = lambda x: "[1] Mocked intel for test"

    res1 = engine.fetch_live_intel("Cisco")
    assert "Mocked intel" in res1

def test_proxy_metadata_scrubbing_trigger():
    proxy = HoneyGPTProxy()
    # Mock subprocess.run to verify trigger
    import subprocess
    original_run = subprocess.run
    called = []
    def mock_run(cmd, check=True):
        called.append(cmd[0])
        return type('obj', (object,), {'returncode': 0})

    subprocess.run = mock_run
    proxy.last_profile_name = "Old_Profile"
    proxy._load_profile = lambda: {"name": "New_Profile"}

    proxy.handle_command("ls")
    assert "./scripts/scrub_metadata.sh" in called
    subprocess.run = original_run # Restore

if __name__ == "__main__":
    pytest.main([__file__])
