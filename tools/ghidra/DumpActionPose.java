// Bank $87 imported at $8000. Dump the action-to-composed-pose boundary.
import java.io.*;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.lang.Register;

public class DumpActionPose extends GhidraScript {
    public void run() throws Exception {
        for (String name : new String[]{"M", "X"}) {
            Register reg = currentProgram.getRegister(name);
            if (reg != null) currentProgram.getProgramContext().setValue(
                reg, toAddr(0xaec3), toAddr(0xb37b), BigInteger.ZERO);
        }
        clearListing(toAddr(0xaec3), toAddr(0xb37b));
        for (long pc : new long[]{0xaec3,0xaf95,0xafa2}) {
            disassemble(toAddr(pc));
            createLabel(toAddr(pc), pc == 0xaec3 ? "ResolveActionPose" :
                pc == 0xaf95 ? "InitializePlayerAppearance" : "SeedTenPlayerAppearance", true);
            setPlateComment(toAddr(pc), "Action/animation C-port verification boundary; raw ROM resource composition.");
        }
        File dir = new File(getScriptArgs()[0]); dir.mkdirs();
        try (PrintWriter out = new PrintWriter(new File(dir,"action_pose_bank87.txt"),"UTF-8")) {
            for(long pc=0xaec3;pc<=0xb37b;++pc) {
                Instruction ins=currentProgram.getListing().getInstructionAt(toAddr(pc));
                if(ins!=null) out.printf("$87:%04X  %s%n",pc,ins.toString());
            }
        }
    }
}
