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
        // Emulation-mode reset & interrupt vectors in bank 00
        mark("reset", 0x00800DL);
        mark("nmi_emulation", 0x008156L);
        mark("irq_emulation", 0x008600L);

        // Native-mode 65816 vectors in bank 00
        mark("cop_native", 0x008172L);
        mark("brk_native", 0x008174L);
        mark("abort_native", 0x008176L);
        mark("nmi_native", 0x008156L);
        mark("irq_native", 0x008600L);

        // Subsystems & Game loop entries
        mark("boot_init", 0x808020L);
        mark("game_loop_entry", 0x80DA91L);
        mark("setup_intro", 0x82F15CL);
    }
}
