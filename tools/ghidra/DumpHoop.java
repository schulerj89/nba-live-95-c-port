import java.io.File;
import java.io.PrintWriter;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;

/** Gameplay basket scroll, window and raster-interrupt source. */
public class DumpHoop extends GhidraScript {
    public void run() throws Exception {
        String[] args = getScriptArgs();
        int bank = Integer.parseInt(args[1], 16);
        int[][] ranges;
        if (bank == 0x80) ranges = new int[][] {{0x8410, 0x84a0}};
        else if (bank == 0x85) ranges = new int[][] {
            {0x8d95, 0x8daf}, {0x8e28, 0x8ee5},
            {0xeeee, 0xef13}, {0xef14, 0xef2d}, {0xef2e, 0xef39}};
        else if (bank == 0x87) ranges = new int[][] {{0xa73b, 0xa845}};
        else throw new IllegalArgumentException("unexpected hoop bank");
        for (int[] range : ranges) {
            clearListing(toAddr(range[0]), toAddr(range[1]));
            for (String name : new String[] {"ctx_MF", "ctx_XF", "ctx_EF"}) {
                Register register = currentProgram.getRegister(name);
                if (register == null) throw new IllegalStateException("missing " + name);
                currentProgram.getProgramContext().setValue(
                    register, toAddr(range[0]), toAddr(range[1]), BigInteger.ZERO);
            }
            disassemble(toAddr(range[0]));
        }
        try (PrintWriter out = new PrintWriter(new File(args[0],
                String.format("hoop_bank%02x.txt", bank)), "UTF-8")) {
            for (int[] range : ranges) {
                for (int pc = range[0]; pc <= range[1]; ++pc) {
                    Instruction ins = currentProgram.getListing().getInstructionAt(toAddr(pc));
                    if (ins != null) out.printf("$%02X:%04X [%d] %s%n",
                        bank, pc, ins.getLength(), ins);
                }
            }
        }
    }
}
