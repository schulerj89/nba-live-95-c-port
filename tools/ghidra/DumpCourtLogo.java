import java.io.File;
import java.io.PrintWriter;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;

/** Fresh bounded instruction listings for the home-court asset upload. */
public class DumpCourtLogo extends GhidraScript {
    public void run() throws Exception {
        String[] args = getScriptArgs();
        int bank = Integer.parseInt(args[1], 16);
        int[][] ranges;
        if (bank == 0x84) ranges = new int[][] {{0xe4d8, 0xe5b2}};
        else if (bank == 0x80) ranges = new int[][] {{0x8ba1, 0x8bcf}, {0x8bd0, 0x8c2a}};
        else if (bank == 0x85) ranges = new int[][] {
            {0x8bbf, 0x8c4e}, {0x8ee6, 0x8fd3},
            {0x8fd4, 0x90c3}, {0x90c4, 0x9179}};
        else throw new IllegalArgumentException("unexpected court bank");
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
                String.format("court_bank%02x.txt", bank)), "UTF-8")) {
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
