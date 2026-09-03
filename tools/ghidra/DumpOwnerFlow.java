// args: output-directory bankHex. Exact census and persistent C mapping.
import java.io.*;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;
public class DumpOwnerFlow extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();boolean upper=args[1].equals("87");
        int[][] spans=upper?new int[][]{{0xAD86,0xADBD},{0x8E7F,0x8E9D}}:
            new int[][]{{0xE545,0xE592},{0xF34F,0xF439},{0xC217,0xC222},{0xCB5E,0xCB69}};
        for(int[] span:spans) {
            clearListing(toAddr(span[0]),toAddr(span[1]));
            for(String name:new String[]{"M","X"}) {
                Register r=currentProgram.getRegister(name);
                if(r!=null)currentProgram.getProgramContext().setValue(r,toAddr(span[0]),toAddr(span[1]),BigInteger.ZERO);
            }
            disassemble(toAddr(span[0]));
        }
        String[][] maps=upper?new String[][]{
            {"AD86","IdleThreePoseCadence","nba_player_animation_step_channels: ordinary live state7 now integrated. XBA(C6) accumulator, reject RNG&3 zero, upper phase=choice-1, next RNG duration+1000. Asset-pack resources; 22 decoded instructions."},
            {"8E7F","PhysicsAndDecisionDeltas","C6=elapsed physical ticks; C8=elapsed<<4. Runtime C6=2 => C8=20 hex, NOT 2. 1857 natural owner calls confirmed C8=20. Input context, not new full scheduler coverage."}
        }:new String[][]{
            {"E545","UnlatchedOwnerPoseAndReverse","nba_player_owner_unlatched_pose: +50 -> +4E, velocity direction gap modulo8 selects9/11. E574/E586 call B37C for opposite current upper pose: reverse lower phase, restore upper. Preserve resource IDs until cadence. Replaces former incomplete two-word proof."},
            {"F34F","OrdinaryOwnerCaller","nba_owner_flow_run / cpu_owner_flow_call: flags -> held-ball stop -> BASE pair+74 pose -> ownership/live82 gates -> signed wrapped countdown -> CPU -> optional formation -> receiver. Independent caller replay includes every child input/output boundary; child bodies retain separate verification."},
            {"F38A","HeldBallStopBeforePose","0968 nonzero and09F6>=2: F02D velocity-facing unless8; clear both velocity components BEFORE E4A7. C binds facing+4E, not display+52."},
            {"F3B7","OwnerBasePairPose","+74 is doubled actor index into879C7B. Not current+76 nor cached assignment_actor. Negative skips pose call."},
            {"F3F6","LostOwnerReturn","Set mode1, behavior47, timer0, flags0, RTL. Never fall through to mode1 AI during this dispatch."},
            {"F40B","OwnerTimerReloadAndCpu","Preserve timer-C8 overshoot, add40+roster byte3F. Human skips CPU. CPU nonlocal return skips F42C onward; otherwise formation iff+7A=0 then receiver. Formation acceleration must precede receiver lead."},
            {"C217","KnockdownCoarseFacing","nba_gameplay_contact_facing: F02D coarse direction XOR4, not F3C3 fine pass direction. Three natural calls confirm DP AA/AE are victim velocities. Supplemental regression fix, not part of the 145 census."},
            {"CB5E","PoseContactCoarseFacing","Same F02D then XOR4 facing in the alternate pose-contact path. Static/recomp mapping; no natural CB5E witness claimed."}
        };
        for(String[] m:maps) {
            createLabel(toAddr(Integer.parseInt(m[0],16)),m[1],true);
            setPlateComment(toAddr(Integer.parseInt(m[0],16)),m[2]+" C port: src/nba_owner_flow.c.");
        }
        File dir=new File(args[0]);dir.mkdirs();
        try(PrintWriter out=new PrintWriter(new File(dir,"owner_flow_bank"+args[1]+".txt"),"UTF-8")) {
            for(int[] span:spans) {
                int count=0;
                for(int pc=span[0];pc<=span[1];++pc) {
                    Instruction ins=currentProgram.getListing().getInstructionAt(toAddr(pc));
                    if(ins!=null){out.printf("$%s:%04X [%d] %s%n",args[1],pc,ins.getLength(),ins);++count;}
                }
                out.printf("# %04X-%04X count=%d%n",span[0],span[1],count);
            }
        }
    }
}
