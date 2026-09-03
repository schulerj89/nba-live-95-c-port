import java.io.File;
import java.io.PrintWriter;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;

/** Original out-of-bounds overlay and shared text/canvas routines. */
public class DumpOutOfBounds extends GhidraScript {
    public void run() throws Exception {
        String[] args = getScriptArgs();
        int bank = Integer.parseInt(args[1], 16);
        int[][] ranges;
        if (bank == 0x81) ranges = new int[][] {
            {0x9f54, 0x9fd3}, {0x9fd4, 0x9fde}, {0x9fdf, 0xa01e},
            {0xa01f, 0xa03c}, {0xa03d, 0xa05e}, {0xa05f, 0xa1e6}, {0xa1e7, 0xa241}};
        else if (bank == 0x83) ranges = new int[][] {
            {0xcc10, 0xcc7a}, {0xda12, 0xda8b}, {0xda8c, 0xdb9c}, {0xebdb, 0xed46}};
        else if (bank == 0x85) ranges = new int[][] {{0x93f5, 0x945e}};
        else if (bank == 0x87) ranges = new int[][] {{0x92ed, 0x93dc}};
        else throw new IllegalArgumentException("unexpected OOB bank");
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
                String.format("oob_bank%02x.txt", bank)), "UTF-8")) {
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
