// Focused bank-$86 shot graph, including the previously truncated launch tail.
import java.io.*;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.lang.Register;

public class DumpShotCompletion extends GhidraScript {
    public void run() throws Exception {
        long[][] ranges={{0x9cdb,0xa5af},{0xb625,0xbaa1}};
        for(long[] r:ranges) {
            for(String name:new String[]{"M","X"}) {
                Register reg=currentProgram.getRegister(name);
                if(reg!=null) currentProgram.getProgramContext().setValue(
                    reg,toAddr(r[0]),toAddr(r[1]),BigInteger.ZERO);
            }
            clearListing(toAddr(r[0]),toAddr(r[1]));
        }
        long[] pcs={0x9cdb,0x9d6e,0x9d7a,0x9da6,0x9db2,0x9ed8,0xa110,0xa1bd,
                    0xa3d4,0xa455,0xa561,0xb625,0xb6d3,0xb769,0xb86c,0xb979,0xba54};
        String[] names={"LaunchSideEffects","CompleteShotLaunch","ShotReleaseFacing",
            "SpecialShotLaunchEntry","ShotDetachState","ShotQualityInputs","ShotQualityRoll","ShotVelocityCore",
            "HumanFreeThrowLaunch","ShotLaunchReturnTail","ShotValueArc","SpecialShotSelector",
            "OrdinaryShotStart","OrdinaryShotExecutor","WindupButtonGate",
            "SpecialShotMode17Executor","SpecialShotAirborne"};
        String[] comments={
            "nba_shot_launch: A=0 attempts only; made-stat increment branches excluded from verification.",
            "nba_shot_launch: ordinary entry installs facing and upper pose 17; common return A476. Complete-shot witnesses compare persistent state, stats and RNG.",
            "nba_shot_action_release_facing: now adopted by ordinary launch; special entry skips this block.",
            "nba_shot_launch special_entry: retain special upper pose 14/15; no facing snap. Eleven controlled nested calls replayed.",
            "nba_shot_launch: detach ownership, identity, latches and timeout. NMI DEC at 85:EE30 is recorded separately from launch-owned STA 0930.",
            "shot_quality: roster 36/37/49, difficulty, range, timing, stamina, defense, movement and hot team. A02A doubles the word index; pack preserves resulting table overread data.",
            "nba_shot_launch: ordered RNG and signed miss offsets from asset 277; made basket remains a physics outcome.",
            "launch_velocity: fractional deltas, signed division, fresh SEC for integer Z and ADC carry preserved.",
            "nba_shot_launch: human FT aim/power and manual bypass replayed with controlled inputs; user controls not enabled.",
            "nba_shot_launch: attempts via 9CDB, actor mode 0B, persistent outputs through RTL A476. Caller owns timer/flag cleanup.",
            "nba_gameplay_shot_value: origin 0900/0902 arc test called even for FT; only writes value 3 beyond arc.",
            "cpu_start_rom_shot re-evaluates F5E4; nba_special_shot_select starts B629. Lane/movement/distance/facing/appearance witnesses in special-shot-witnesses.json.",
            "nba_shot_action_start: preserved ordinary fallback after failed special selector.",
            "cpu_update_rom_shooter: ordinary mode-12 path; existing shot-action/shot-branch witnesses.",
            "nba_shot_action_windup_button: separate CPU and human-button paths; shot-branch witnesses.",
            "nba_special_shot_step: mode17 owner loss, attachment, activity/jump, cancellation and phase gate. Release output stops before nested 9DA6, verified separately. Runtime checks both basket sides.",
            "nba_special_shot_step: delayed facing turn; upper phase 3 calls special launch 9DA6 then clears caller timer/flags at BA4A. Rare path verified by controlled genuine ROM calls, not natural-frequency proof."
        };
        for(int i=0;i<pcs.length;++i) {
            disassemble(toAddr(pcs[i]));
            createLabel(toAddr(pcs[i]),names[i],true);
            setPlateComment(toAddr(pcs[i]), comments[i]+" C port: src/nba_shot_launch.c, src/nba_shot_action.c, src/nba_tipoff.c; replay tools/verify_complete_shot_vectors.py and tools/verify_special_shot_vectors.py.");
        }
        File dir=new File(getScriptArgs()[0]);dir.mkdirs();
        try(PrintWriter out=new PrintWriter(new File(dir,"shot_completion_bank86.txt"),"UTF-8")) {
            for(long[] r:ranges) for(long pc=r[0];pc<=r[1];++pc) {
                Instruction ins=currentProgram.getListing().getInstructionAt(toAddr(pc));
                if(ins!=null)out.printf("$86:%04X  %s%n",pc,ins.toString());
            }
        }
    }
}
