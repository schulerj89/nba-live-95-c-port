import java.io.File;
import java.io.PrintWriter;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;

/** CPU role-rebuild reaction reload at $85:B95C and its three callers. */
public class DumpCpuReaction extends GhidraScript {
    public void run() throws Exception {
        String[] args = getScriptArgs();
        int[][] ranges = new int[][] {
            {0xb95c, 0xb9d1}, {0xbd0d, 0xbe05}
        };
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
        try (PrintWriter out = new PrintWriter(
                new File(args[0], "cpu_reaction_bank85.txt"), "UTF-8")) {
            for (int[] range : ranges) {
                for (int pc = range[0]; pc <= range[1]; ++pc) {
                    Instruction ins = currentProgram.getListing().getInstructionAt(toAddr(pc));
                    if (ins != null) out.printf("$85:%04X [%d] %s%n",
                        pc, ins.getLength(), ins);
                }
            }
        }
    }
}
