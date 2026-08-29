// Fresh tip contact/receiver/deflection/possession census. args: outDir
import java.io.*;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;
public class DumpTipoffFlow extends GhidraScript {
    public void run() throws Exception {
        int bank=currentProgram.getName().contains("85")?0x85:0x86;
        int[][] ranges=bank==0x86?new int[][]{
            {0x99c4,0x9c44},{0x9c45,0x9c6e},{0xb04c,0xb0e1},{0xccfc,0xd43d},
            {0xe056,0xe0ab},{0xec32,0xecf8},{0xecf9,0xee75},
            {0xe39a,0xe3ca},{0xe3e1,0xe4a6},{0xf43a,0xf4f1},
            {0xf4f2,0xf51f},{0xf520,0xf54e},{0xf54f,0xf58e},
            {0xf58f,0xf5ba},{0xf5bb,0xf60a},{0xf60b,0xf653},
            {0xf654,0xf668},{0x844e,0x8467}}:
            new int[][]{{0xb100,0xb28b}};
        for(int[] r:ranges) {
            clearListing(toAddr(r[0]),toAddr(r[1]));
            for(String n:new String[]{"M","X"}) {
                Register reg=currentProgram.getRegister(n);
                if(reg!=null)currentProgram.getProgramContext().setValue(reg,toAddr(r[0]),toAddr(r[1]),BigInteger.ZERO);
            }
            disassemble(toAddr(r[0]));
        }
        if(bank==0x86)for(int p:new int[]{0xd25a,0xd3c6,0xd3fd,0xb0d8,0xcf01,0xcf9d,0xcfa0})disassemble(toAddr(p));
        // These selected intervals are code-only. Decode branch alternatives
        // missed by the older trace-only listing, and reject partial tails.
        for(int[] r:ranges){int p=r[0];while(p<=r[1]){
            Instruction ins=currentProgram.getListing().getInstructionAt(toAddr(p));
            if(ins==null){disassemble(toAddr(p));ins=currentProgram.getListing().getInstructionAt(toAddr(p));}
            if(ins==null)throw new Exception("decode gap at "+Integer.toHexString(p));
            p+=ins.getLength();
        }if(p!=r[1]+1)throw new Exception("range ends inside instruction "+Integer.toHexString(r[1]));}
        String[][] labels=bank==0x86?new String[][]{
            {"ec32","JumpReachDecision","nba_jump_reach_decide in src/nba_jump_reach.c. EC32-EE75 has239 instruction starts. Native decision projection checks XYZ velocities, RNG and ordered child requests; animation channel replay is separate. NOT integrated into ordinary tip-off scheduler yet; do not claim full WRAM or whole-game equivalence. docs/jump-reach-differential.md."},
            {"ecf9","JumpReachFreeBallContinuation","Receiver/distance/height/rating/threshold gates. ED32 and EDEF are B9 0A 00 (LDA000A,Y): Y=003C, so read WRAM0046, NOT team-context X. ED3D/EDFA select roster3C/3D. Table86EE76-EF05 packed in asset280. CPU callers F787/F892 and shared0046 writer history still require runtime integration."},
            {"eca1","JumpReachVerticalLaunch","Magnitude(ballVZ)<0040 gives0258. Otherwise exactly one CEE7 draw: low2bits0 rejects, bit1 selects0210 vs0258. State0081 requests B3BD state32. Non-tip continuation also requests upper33/lower1F or upper34/lower32. Child request is not child effect proof."},
            {"e056","BallInitializationPrefix","nba_tip_ball_initialize / nba_tipoff_init. Actual start E056; E054 is inside preceding JMP at E053. 30 instruction starts,20 mapped words. Natural and poisoned-input full-WRAM differential witnesses: tests/fixtures/ball-initialization*.json. Startup velocity0258, height0050, native object-list links and sentinel globals. Later toss/jump dispatcher not certified by this prefix."},
            {"d3c6","TipContactState81","Temporary catch -> B04C. DP C2 remains ball record10 while96/9A swap; save/restore original ball Z, inhibit other center. Native wrapper replay in tip-acquisition.json."},
            {"d365","TipOrPossessionCompletion","Transfer/receiver/state distinguish temporary tip from actual possession. Whistle guards state clear. All28 starts replayed in tip-completion.json."},
            {"b04c","TipSelectReceiverAndLaunch","Native tip receiver uses RNG low bit plus team base. Copies DP C2 to0942, calls99C4 for launch; not a fixed actor8."}
        }:new String[][]{};
        for(String[] row:labels){createLabel(toAddr(Integer.parseInt(row[0],16)),row[1],true);setPlateComment(toAddr(Integer.parseInt(row[0],16)),row[2]);}
        File dir=new File(getScriptArgs()[0]);dir.mkdirs();
        try(PrintWriter out=new PrintWriter(new File(dir,String.format("tip_flow_bank%02x.txt",bank)),"UTF-8")) {
            for(int[] r:ranges){int count=0;for(int p=r[0];p<=r[1];p++){
                Instruction ins=currentProgram.getListing().getInstructionAt(toAddr(p));
                if(ins!=null){out.printf("$%02X:%04X [%d] %s%n",bank,p,ins.getLength(),ins);count++;}
            }out.printf("# %04X-%04X count=%d (contiguous code)%n",r[0],r[1],count);}
        }
    }
}
