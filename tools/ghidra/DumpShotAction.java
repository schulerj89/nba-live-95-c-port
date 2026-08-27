// Bank $86 binary at $8000: action recovery and mode-12 shot executor.
import java.io.*;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.lang.Register;
public class DumpShotAction extends GhidraScript {
    public void run() throws Exception {
        boolean facing=currentProgram.getName().contains("85");
        long[][] ranges=facing ? new long[][]{{0xf02d,0xf099}} :
            new long[][]{{0x9846,0x99c3},{0x9d7a,0x9da7},{0xb625,0xb978}};
        for(long[] r:ranges) {
            for(String name:new String[]{"M","X"}) {
                Register reg=currentProgram.getRegister(name);
                if(reg!=null) currentProgram.getProgramContext().setValue(reg,toAddr(r[0]),toAddr(r[1]),BigInteger.ZERO);
            }
            clearListing(toAddr(r[0]),toAddr(r[1]));
        }
        long[] pcs=facing ? new long[]{0xf02d} : new long[]{0x9846,0x9861,0x986d,0xb625,0xb6d3,0xb769,0xb7cd,0xb7f7,0xb84c,0xb867,0xb86c,0xb886,0xb890,0xb8c0,0xb8c9,0xb8ca};
        String[] names=facing ? new String[]{"QuantizeShotFacing"} : new String[]{"RestoreActionMode","ClearActionFlags","RimHangAfterFinish","InitializeShot","StandardShotActionStart","ExecuteShot","ShotWindupTimer","ShotStationarySidestep","ShotJumpLowerInstall","ShotOwnerLost","ShotWindupButtonGate","LatchPumpFake","CancelPumpFake","ShotReleaseCleanup","ShotWindupReturn","ShotFacingAndRelease"};
        for(int i=0;i<pcs.length;++i) {
            disassemble(toAddr(pcs[i])); createLabel(toAddr(pcs[i]),names[i],true);
            setPlateComment(toAddr(pcs[i]),"C-port action verification: "+names[i]);
        }
        if(!facing) {
            disassemble(toAddr(0x9d7a));
            createLabel(toAddr(0x9d7a),"ShotReleaseBasketFacing",true);
            setPlateComment(toAddr(0x9d7a),"Verified facing helper only; full 9D6E launch caller remains unadopted.");
        }
        File dir=new File(getScriptArgs()[0]);dir.mkdirs();
        try(PrintWriter out=new PrintWriter(new File(dir,facing ? "shot_facing_bank85.txt" : "shot_action_bank86.txt"),"UTF-8")) {
            for(long[] r:ranges) for(long pc=r[0];pc<=r[1];++pc) {
                Instruction ins=currentProgram.getListing().getInstructionAt(toAddr(pc));
                if(ins!=null)out.printf("$%02X:%04X  %s%n",facing?0x85:0x86,pc,ins.toString());
            }
        }
    }
}
