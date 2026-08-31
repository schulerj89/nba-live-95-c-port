"""Additional source-address precondition for native F1 callbacks only."""
from setup_spc_state_contract_v4 import validate, schema, pending_dsp, check
from setup_spc_state_contract_v4 import control_boundary as previous_control_boundary

def control_boundary(s,source_pc,value):
    previous_control_boundary(s,source_pc,value)
    # Pinned Spc.Instructions.cpp: opcode8F -> Addr_DirImm -> MOV_Imm.
    # Spc::GetDirectAddress maps operandF1 to01F1 when PS.P(bit20) is set.
    # MOV_Imm preserves PS. Both the write callback and following-instruction
    # capture of an actual00F1 write therefore require P0. This restricts only
    # the native CPU callback evidence, not the standalone F1 hardware commit
    # API (which accepts no CPU PS and must retain write-disabled semantics).
    check((int(s['spc.ps'])&0x20)==0,'F1 native direct operand requires SPC PS.P=0')
