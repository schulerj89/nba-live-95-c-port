// Dump and label the bounded pause timeout/resume path in bank $86.
// args: outputDirectory
import java.io.*;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;

public class DumpTimeoutResume extends GhidraScript {
    public void run() throws Exception {
        String[] args = getScriptArgs();
        long first = 0x818d, last = 0x85b5;
        clearListing(toAddr(first), toAddr(last));
        for (String name : new String[] { "M", "X" }) {
            Register reg = currentProgram.getRegister(name);
            if (reg != null) {
                currentProgram.getProgramContext().setValue(
                    reg, toAddr(first), toAddr(last), BigInteger.ZERO);
            }
        }
        disassemble(toAddr(first));
        for (long seed : new long[] { 0x818d, 0x81d7, 0x8230, 0x8300,
                0x8369, 0x83d9, 0x844e, 0x8468, 0x8498, 0x8546 }) {
            disassemble(toAddr(seed));
        }

        createLabel(toAddr(0x8300), "PauseMenuTimeoutResume", true);
        setPlateComment(toAddr(0x8300),
            "Bounded pause menu dispatch. Timeout selection reaches $844E; " +
            "successful timeout decrements one side counter then grants stamina at $8468.");
        createLabel(toAddr(0x844e), "PauseTimeoutConfirm", true);
        createLabel(toAddr(0x8468), "PauseTimeoutStaminaGrant", true);
        createLabel(toAddr(0x8498), "PauseMenuReturn", true);

        File dir = new File(args[0]);
        dir.mkdirs();
        try (PrintWriter out = new PrintWriter(
                new File(dir, "timeout_resume_bank86.txt"), "UTF-8")) {
            int count = 0;
            for (long pc = first; pc <= last; ++pc) {
                Instruction ins = currentProgram.getListing().getInstructionAt(toAddr(pc));
                if (ins != null) {
                    out.printf("$86:%04X [%d] %s%n", pc, ins.getLength(), ins);
                    ++count;
                }
            }
            out.printf("# 818D-85B5 decoded_instruction_count=%d%n", count);
        }
    }
}
