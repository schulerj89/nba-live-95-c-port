// Ghidra headless script for NBA Live '95 (USA) LoROM 65816 analysis.

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;

public class SnesEntryPoints extends GhidraScript {
    private void mark(String name, long offset) throws Exception {
        Address address = toAddr(offset);
        addEntryPoint(address);
        disassemble(address);
        if (getFunctionAt(address) == null) {
            createFunction(address, name);
        }
        println(String.format("SNES Entry %-16s @ %s", name, address));
    }

    @Override
    public void run() throws Exception {
        // Bank $82 is imported into Ghidra's 16-bit address space at $8000.
        mark("setup_intro", 0xF15CL);
        mark("intro_stage_e", 0xF2EAL);
        mark("intro_stage_a", 0xF36AL);
        mark("intro_stage_sports", 0xF408L);
        mark("intro_stage_hold", 0xF469L);
        mark("intro_flash", 0xF4C4L);
        mark("draw_e", 0xF4F6L);
        mark("draw_a", 0xF512L);
        mark("draw_sports", 0xF52EL);
        mark("intro_step_loop", 0xF56DL);
        mark("intro_color_step", 0xF5E7L);
        mark("mode7_matrix_commit", 0x94D5L);
        mark("mode7_matrix_init", 0x94DFL);
        mark("mode7_register_update", 0x962DL);
        mark("intro_palette_step", 0xF64AL);
        mark("post_ea_sequence", 0xAC0EL);
        mark("post_ea_sequence_engine", 0xABE0L);
        mark("post_ea_table_item_dispatch", 0xABCEL);
        mark("post_ea_sequence_script_a", 0xA9D1L);
        mark("post_ea_sequence_script_b", 0xAB69L);
        mark("post_ea_sequence_setup", 0xACFBL);
        mark("post_ea_sequence_dispatch", 0xAD15L);
    }
}
