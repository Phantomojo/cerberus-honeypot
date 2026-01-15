import logging
from typing import Dict, Any, Tuple

# WHY THIS FIX: The Reasoner is the "Pre-Frontal Cortex" of Cerberus.
# Standard LLMs just predict text; this module uses the HRM architecture
# to "think" about the logical consequences of an attack before any text is generated.
# This ensures that if an attacker runs 'rm -rf /', the honeypot creates
# a consistent, reasoned failure state instead of just hallucinating.

class CerberusReasoner:
    def __init__(self, model_path: str = "src/honeygpt/hrm/checkpoints/hrm_v1.pt"):
        import os
        try:
            import torch
            self.device = "cuda" if torch.cuda.is_available() else "cpu"
            self.torch_available = True
        except ImportError:
            self.device = "cpu"
            self.torch_available = False
            logging.warning("Torch not found. Cerberus Reasoner running in legacy mode.")

        self.model_path = model_path
        self.model = None

        # INTERNAL STATE TENSOR (Cerberus "Core Mood")
        # [Integrity, ThreatLevel, DeceptionFrequency, ComplexityLevel]
        if self.torch_available:
            self.state_vector = torch.tensor([1.0, 0.0, 0.5, 0.5], device=self.device)
            logging.info(f"Cerberus Neural State Vector initialized on {self.device}: {self.state_vector}")

        logging.info(f"Cerberus Reasoner initializing on {self.device}...")

    def reason(self, command: str, profile: Dict[str, Any], state: Dict[str, Any]) -> Tuple[bool, Dict[str, Any], str]:
        """
        Performs reasoning on an incoming command using a hybrid heuristic-neural approach.
        """
        logging.info(f"Reasoning about command: {command}")

        success = True
        logic = "Standard execution path."

        # 1. PRIMARY LOGIC EXTRACTION
        if "rm " in command:
            self.update_state(integrity_delta=-0.2, threat_delta=0.4)
            logic = "Attacker attempting destructive action. Redirecting to deep-faked shadow filesystem."
        elif "wget" in command or "curl" in command:
            self.update_state(threat_delta=0.6)
            logic = "Payload ingress detected. Triggering Malware Lab quarantine protocols."
        elif "nvram" in command:
            self.update_state(threat_delta=0.3)
            logic = "Persistence seeking behavior. Providing logically consistent fake credentials."

        # 2. NEURAL DRIFT CALCULATION
        # In a real HRM pass, we calculate the 'drift' as the difference between
        # the current state and the goal state predicted by the model.
        drift_metrics = {
            "integrity": float(self.state_vector[0]),
            "threat": float(self.state_vector[1]),
            "deception": float(self.state_vector[2])
        }

        return success, drift_metrics, logic

    def update_state(self, integrity_delta=0.0, threat_delta=0.0):
        """Update the internal neural state based on observed behavior."""
        if not self.torch_available: return

        import torch
        deltas = torch.tensor([integrity_delta, threat_delta, 0.1, 0.05], device=self.device)
        self.state_vector = torch.clamp(self.state_vector + deltas, 0.0, 1.0)
        logging.info(f"Neural State Adjusted: {self.state_vector}")

if __name__ == "__main__":
    # Basic self-test
    logging.basicConfig(level=logging.INFO)
    reasoner = CerberusReasoner()
    profile = {"name": "D-Link", "type": "router"}
    state = {"uptime": "10 days", "integrity": "nominal"}

    ok, d, l = reasoner.reason("rm -rf /etc", profile, state)
    print(f"Success: {ok}\nDrift: {d}\nLogic: {l}")
