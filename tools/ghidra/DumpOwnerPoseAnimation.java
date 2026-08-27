// args: output-directory bankHex. Reproducible bounded instruction census.
import java.io.*;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;

public class DumpOwnerPoseAnimation extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs(); boolean upper=args[1].equals("87");
        int first=upper?0xAD5B:0xE4F5,last=upper?0xAEC2:0xE544;
        clearListing(toAddr(first),toAddr(last));
        for(String name:new String[]{"M","X"}) {
            Register reg=currentProgram.getRegister(name);
            if(reg!=null)currentProgram.getProgramContext().setValue(reg,toAddr(first),toAddr(last),BigInteger.ZERO);
        }
        disassemble(toAddr(first));
        if(upper) {
            disassemble(toAddr(0xAE89));
            createLabel(toAddr(0xAD5B),"SpecialAnimationCadence",true);
            setPlateComment(toAddr(0xAD5B),"nba_player_animation_step_channels / animation_channel_advance: mode 2 lower reset, upper states 7/13/18; DP C6 is byte-swapped delta; actor +44 doubles as upper duration. docs/owner-pose-animation-plan.md.");
            createLabel(toAddr(0xADBE),"HeldBallEightPhaseCadence",true);
            setPlateComment(toAddr(0xADBE),"Upper state 18: actor +B0 low 15 bits target (0..7), high bit decreasing phase. One step only; normalize target/timer, preserve wrapped accumulator subtraction and 07F6 RNG order. Reaching target chooses opposite half and toggles traversal direction.");
            createLabel(toAddr(0xAE89),"HeldBallTwoPhaseCadence",true);
            setPlateComment(toAddr(0xAE89),"Upper state 13: random 0/1 stored to BOTH +3A/+3C, then reset +42 and random duration +44. Lower resource +2C was ALREADY resolved at AC38; shared phase affects next call. Asset-pack descriptors, not captured art.");
        } else {
            createLabel(toAddr(first),"LatchedOwnerPose",true);
            setPlateComment(toAddr(first),"nba_gameplay_owner_latched_pose: controller +16, ball state09F6/dead0968 select CPU vs human branch. CPU compares +8A-0011 via N flag, writes base+38=13/18 and facing+4E from requested+50 (xor4 for18). Caller proximity must return LATCHED, not generic fallback.");
        }
        File dir=new File(args[0]);dir.mkdirs();
        try(PrintWriter out=new PrintWriter(new File(dir,"owner_pose_bank"+args[1]+".txt"),"UTF-8")) {
            for(int pc=first;pc<=last;++pc) {
                Instruction ins=currentProgram.getListing().getInstructionAt(toAddr(pc));
                if(ins!=null)out.printf("$%s:%04X [%d] %s%n",args[1],pc,ins.getLength(),ins);
            }
        }
    }
}
