import pytest
from main import HoneyGPTProxy
from prompts import PromptManager
from llm_client import LLMClient

# WHY THIS FIX: Critical verification that the AI layer behaves
# predictably even during simulation/mock mode.

def test_prompt_generation():
    pm = PromptManager({"name": "Test Router"})
    prompt = pm.build_prompt("ls -la", "System is up.")
    assert "Test Router" in prompt
    assert "ls -la" in prompt
    assert "RESPONSE (JSON format" in prompt

def test_llm_mock_responses():
    client = LLMClient() # Will run in mock mode without key
    resp = client.query("ls")
    assert "bin" in resp
    assert "etc" in resp

def test_proxy_command_handling():
    proxy = HoneyGPTProxy()
    # Mock the LLM to return valid JSON
    proxy.llm.query = lambda x: '{"output": "fake_file.txt", "impact_score": 1, "state_change": {}}'

    out = proxy.handle_command("ls")
    assert out == "fake_file.txt"

if __name__ == "__main__":
    pytest.main([__file__])
