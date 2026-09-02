import java.io.File;
import java.io.PrintWriter;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;

/** Literal phase, attachment, bounce and sprite submission references. */
public class DumpDribble extends GhidraScript {
    public void run() throws Exception {
        String[] args = getScriptArgs();
        int bank = Integer.parseInt(args[1], 16);
        int[][] ranges;
        if (bank == 0x80) ranges = new int[][] {
            {0xaf1e, 0xb0ab}, {0xb0ac, 0xb0fe}, {0xb0ff, 0xb11f},
            {0xb344, 0xb498}, {0xfbff, 0xfc68}, {0xfc69, 0xfc7f},
            {0xfc80, 0xfca1}};
        else if (bank == 0x85) ranges = new int[][] {
            {0x9a24, 0x9a77}, {0xa4f2, 0xa5f3}, {0xa7a1, 0xa7c7}};
        else if (bank == 0x86) ranges = new int[][] {{0xe545, 0xe592}};
        else if (bank == 0x87) ranges = new int[][] {
            {0xa357, 0xa3a4}, {0xa3a5, 0xa4e0},
            {0xa61e, 0xa6a8},
            {0xad5b, 0xae88}, {0xae89, 0xaec2},
            {0xb649, 0xb669}, {0xb66a, 0xb67b},
            {0xb832, 0xb952}, {0xb953, 0xb995}};
        else throw new IllegalArgumentException("unexpected dribble bank");
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
                String.format("dribble_bank%02x.txt", bank)), "UTF-8")) {
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
