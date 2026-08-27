// Complete M0/X0 animation lifecycle, including branches absent in live traces.
// Bank-$87 binary imported at $8000; argument: output directory.
import java.io.*;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.lang.Register;

public class DumpActionAnimation extends GhidraScript {
    public void run() throws Exception {
        long[][] ranges = {{0xa9d0,0xa9e2},{0xaab2,0xaec2},{0xb37c,0xb571}};
        String[] names = {"AnimationPreStep", "AnimationCadence", "ReverseAction"};
        for (int i=0; i<ranges.length; ++i) {
            long[] range = ranges[i];
            for (String name : new String[]{"M", "X"}) {
                Register reg = currentProgram.getRegister(name);
                if (reg != null) currentProgram.getProgramContext().setValue(
                    reg, toAddr(range[0]), toAddr(range[1]), BigInteger.ZERO);
            }
            clearListing(toAddr(range[0]),toAddr(range[1]));
            disassemble(toAddr(range[0]));
            createLabel(toAddr(range[0]), names[i], true);
        }
        long[] entries={0xad5b,0xae89,0xb3bd,0xb47a,0xb4db,0xb538,0xb555};
        String[] labels={"SpecialUpperCadence","SpecialUpperState13","InstallBothActionChannels",
            "InstallUpperActionChannel","InstallLowerActionChannel",
            "CancelUpperAction","CancelLowerAction"};
        for(int i=0;i<entries.length;++i) {
            disassemble(toAddr(entries[i]));
            createLabel(toAddr(entries[i]),labels[i],true);
            setPlateComment(toAddr(entries[i]), "C-port action/animation verification: " + labels[i]);
        }
        File outDir=new File(getScriptArgs()[0]); outDir.mkdirs();
        try(PrintWriter out=new PrintWriter(new File(outDir,"action_animation_bank87.txt"),"UTF-8")) {
            for(long[] range:ranges) for(long pc=range[0];pc<=range[1];++pc) {
                Instruction ins=currentProgram.getListing().getInstructionAt(toAddr(pc));
                if(ins!=null) out.printf("$87:%04X  %s%n",pc,ins.toString());
            }
        }
    }
}
