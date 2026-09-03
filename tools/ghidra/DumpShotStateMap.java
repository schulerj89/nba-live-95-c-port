// Reproduce shot-state census and named C/ROM correspondence.
// args: outDir bankHex ranges(first-last:...) entryPCs(colon-separated)
import java.io.*;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;

public class DumpShotStateMap extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();
        String[] spans=args[2].split(":");
        for(String span:spans) {
            String[] ends=span.split("-");
            long first=Long.parseLong(ends[0],16),last=Long.parseLong(ends[1],16);
            clearListing(toAddr(first),toAddr(last));
            for(String name:new String[]{"M","X"}) {
                Register reg=currentProgram.getRegister(name);
                if(reg!=null)currentProgram.getProgramContext().setValue(reg,toAddr(first),toAddr(last),BigInteger.ZERO);
            }
        }
        for(String seed:args[3].split(":")) {
            long pc=Long.parseLong(seed,16);disassemble(toAddr(pc));
            createLabel(toAddr(pc),"ShotStateMap_"+args[1]+"_"+seed,true);
            setPlateComment(toAddr(pc),"Shot-state mapping. Count decoded instructions, excluding data; labels alone do not claim verification. C port: src/nba_shot_state.c.");
        }
        String[][] maps={
            {"85","A081","MadeShotRunCounters","nba_shot_momentum_make: increment shooter actor +B2; clear opposing five +B2/+B4. Runs BEFORE score increment. 342 writer replays: tests/fixtures/shot-state-witnesses.json."},
            {"85","A0B8","TrailingTeamCpuAssistance","nba_shot_momentum_make: 09C0 is CPU Assistance, NOT generic hot streak. 17C1 enabled and clock <7200; pre-basket left deficit >=3 selects 0, right deficit >=2 selects 5; otherwise FFFF."},
            {"85","EDC6","ShotStateClockTick","nba_shot_clock_step: independent 60-Hz clock/09C2/0930/092C writer, including live-82 run-clock thresholds. NMI writes during fatigue replay are retained and reconstructed, not ignored."},
            {"86","8468","TimeoutStaminaGrant4096","nba_shot_stamina_fixed_grant: all 24 stats records, 16-bit wrap then unsigned 7FFF clamp. LDA #1000 overwrites earlier shift/min result. Caller is pause-menu timeout index 0 at 844E. Helper replayed; timeout UI caller integration deferred."},
            {"86","DA49","RosterStaminaInit","nba_shot_stamina_init: 24 statistics records at 40EB+slot*40, field +18=7FFF. Not actor animation-queue +18."},
            {"86","DD80","PeriodAssistanceReset","nba_shot_momentum_reset: 093E and 09C0 become FFFF. Initial game adopted; later period orchestration deferred."},
            {"86","B625","NaturalSpecialShotSelector","cpu_start_rom_shot / nba_special_shot_select: F5E4 clear-lane result, movement, range, facing and appearance decide mode 12/17. 47 additional natural ROM inputs replay; unforced C special at 50338 releases 50366. Not frequency/whole-game parity."},
            {"87","985D","RosterStaminaGrant","nba_shot_stamina_grant: caller A added to all 24 stats +18; wrapped 16-bit unsigned clamp. Timeout/period caller orchestration deferred."},
            {"87","98EA","ActiveRosterFatigueTick","nba_shot_fatigue_step: live-state signed gate, 09C2>=60 unsigned, active mapping 3435, boost actor+72, option17E7. Stats +1A increments even when fatigue OFF; calls 996A."},
            {"87","996A","AllRosterStaminaRecovery","nba_shot_stamina_recover: 24 roster pointers3471/3473, rating byte+35 and quarter-length17B1 select table8799C3. Add doubled value, N-flag clamp. Runs even fatigue OFF. Asset278/roster251; no emulator data at runtime."},
            {"87","8DF3","FatigueTimerInit","nba_shot_fatigue_timer_init: 09C2=1000. Separate witness from DA49 initialization."},
            {"87","8EF3","DispatchRosterFatigue","nba_tipoff_update invokes nba_shot_fatigue_step before due actor pass. ROM writer replay plus runtime binding checks; initial tip presentation remains its existing bounded handoff."}
        };
        for(String[] map:maps)if(map[0].equalsIgnoreCase(args[1])) {
            long pc=Long.parseLong(map[1],16);
            createLabel(toAddr(pc),map[2],true);
            setPlateComment(toAddr(pc),map[3]);
        }
        File dir=new File(args[0]);dir.mkdirs();
        try(PrintWriter out=new PrintWriter(new File(dir,"shot_state_bank"+args[1]+".txt"),"UTF-8")) {
            for(String span:spans) {
                String[] ends=span.split("-");int count=0;
                for(long pc=Long.parseLong(ends[0],16);pc<=Long.parseLong(ends[1],16);++pc) {
                    Instruction ins=currentProgram.getListing().getInstructionAt(toAddr(pc));
                    if(ins!=null){out.printf("$%s:%04X [%d] %s%n",args[1],pc,ins.getLength(),ins);++count;}
                }
                out.printf("# %s decoded_instruction_count=%d%n",span,count);
            }
        }
    }
}
