import java.io.File;
import java.io.PrintWriter;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;

/** Normal CPU control-mode-three continuation at $86:F23F-$F2C9. */
public class DumpCpuModeThreeRole extends GhidraScript {
    public void run() throws Exception {
        String[] args = getScriptArgs();
        int first = 0xf23f;
        int last = 0xf2c9;
        clearListing(toAddr(first), toAddr(last));
        for (String name : new String[] {"ctx_MF", "ctx_XF", "ctx_EF"}) {
            Register register = currentProgram.getRegister(name);
            if (register == null) throw new IllegalStateException("missing " + name);
            currentProgram.getProgramContext().setValue(
                register, toAddr(first), toAddr(last), BigInteger.ZERO);
        }
        disassemble(toAddr(first));
        try (PrintWriter out = new PrintWriter(
                new File(args[0], "cpu_mode_three_role_bank86.txt"), "UTF-8")) {
            for (int pc = first; pc <= last; ++pc) {
                Instruction ins = currentProgram.getListing().getInstructionAt(toAddr(pc));
                if (ins != null) out.printf("$86:%04X [%d] %s%n",
                    pc, ins.getLength(), ins);
            }
        }
    }
}
