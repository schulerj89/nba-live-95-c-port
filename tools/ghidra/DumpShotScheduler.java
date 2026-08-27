import java.io.*;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;

public class DumpShotScheduler extends GhidraScript {
    public void run() throws Exception {
        for(String name:new String[]{"M","X"}) {
            Register r=currentProgram.getRegister(name);
            if(r!=null)currentProgram.getProgramContext().setValue(r,toAddr(0xedc0),toAddr(0xee80),BigInteger.ZERO);
        }
        clearListing(toAddr(0xedc0),toAddr(0xee80));
        disassemble(toAddr(0xee25));
        disassemble(toAddr(0xee30));
        createLabel(toAddr(0xee30),"InterruptShotFlightCountdown",true);
        setPlateComment(toAddr(0xee30),"Observed Mesen $0930 writes while $86:9D6E is interrupted. Not an instruction in the launch routine.");
        File dir=new File(getScriptArgs()[0]);dir.mkdirs();
        try(PrintWriter out=new PrintWriter(new File(dir,"shot_scheduler_bank85.txt"),"UTF-8")) {
            for(long pc=0xedc0;pc<=0xee80;++pc) {
                Instruction ins=currentProgram.getListing().getInstructionAt(toAddr(pc));
                if(ins!=null)out.printf("$85:%04X  %s%n",pc,ins.toString());
            }
        }
    }
}
