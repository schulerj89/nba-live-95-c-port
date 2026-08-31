// Bounded native controller/team-identity decode in a private bank project.
// Args: output directory, bank hex (85/86/87). No shared project mutations.
import java.io.*;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;

public class DumpControllerOwnership extends GhidraScript {
    private void decode(int first, int last, int... seeds) throws Exception {
        clearListing(toAddr(first), toAddr(last));
        for (String name : new String[]{"M", "X"}) {
            Register reg = currentProgram.getRegister(name);
            if (reg != null) currentProgram.getProgramContext().setValue(
                reg, toAddr(first), toAddr(last), BigInteger.ZERO);
        }
        for (int seed : seeds) disassemble(toAddr(seed));
    }
    public void run() throws Exception {
        File dir = new File(getScriptArgs()[0]); dir.mkdirs();
        int bank = Integer.parseInt(getScriptArgs()[1], 16);
        if (bank == 0x85) {
            decode(0xef3a, 0xefec, 0xef3a);
        } else if (bank == 0x86) {
            decode(0xbc9b, 0xbd1e, 0xbc9b);
            decode(0xda85, 0xdc6b, 0xda85, 0xda8d);
            decode(0xe208, 0xe389, 0xe208, 0xe24c);
        } else if (bank == 0x87) {
            decode(0x9106, 0x92a5, 0x9106);
            decode(0x9b30, 0x9b37, 0x9b30);
        } else throw new Exception("unsupported bank");
        try (PrintWriter out = new PrintWriter(new File(dir,
                String.format("controller_ownership_bank%02x.txt", bank)), "UTF-8")) {
            for (int pc = 0x8000; pc <= 0xffff; ++pc) {
                Instruction ins = currentProgram.getListing().getInstructionAt(toAddr(pc));
                if (ins != null) out.printf("$%02X:%04X [%d] %s%n", bank, pc, ins.getLength(), ins);
            }
        }
    }
}
