// args: output directory, optional census JSON path. Program bank_85.bin.
import java.io.*;
import java.math.BigInteger;
import java.util.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;
public class DumpCourtPresentation extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();
        int[][] all={{0x8e1c,0x8ee5},{0x8ee6,0x90c3},{0x9192,0x93f4}};
        for(int[] r:all) {
            clearListing(toAddr(r[0]),toAddr(r[1]));
            for(String name:new String[]{"M","X"}) {
                Register reg=currentProgram.getRegister(name);
                if(reg!=null)currentProgram.getProgramContext().setValue(reg,toAddr(r[0]),toAddr(r[1]),BigInteger.ZERO);
            }
            disassemble(toAddr(r[0]));
        }
        // Cross-range traversal can stop at pre-existing instructions while
        // the next range is being cleared. Seed the disconnected row tail.
        disassemble(toAddr(0x8fd4));
        disassemble(toAddr(0x901a));
        disassemble(toAddr(0x92c0));
        for(int seed:new int[]{0x9276,0x92ca,0x92f9,0x9312,0x932f,0x9352,0x93a3})
            disassemble(toAddr(seed));
        String[][] comments={
            {"8E1C","CourtPresentationCaller","78-instruction audit: resolver A9D0, existing CC10 whistle/audio, camera9192, portable 8E28 state writes, streamer8EE6, existing sprite rendererA357. Child bodies are separate proof scopes."},
            {"8E28","CourtBasketAndWindowPlacement","nba_court_presentation_update: period926 and cameraX choose team anchor ->3FEF. Left/middle/right bands set087C/E/0880/2; middle retains087E. 1000 ROM replays incl56 controlled. Runtime binding+telemetry; downstream BG1 window/rim renderer is separate scope."},
            {"8EE6","StreamCourtCircularMap","nba_court_stream_update: horizontal columns first, vertical rows second; circular VRAM wrap; at most3 rows/pass. Source A0:8006 header148x52 (NOT114x52). Native DMA descriptors/99 scratch words replayed. Runtime uses full1184x416 asset panorama, no screenshot art or DMA emulator."},
            {"8FD4","StreamCourtRows","33 words gathered backwards from column-major ROM map, two horizontal DMA pieces; scratch offsets0/42/84, limitC6. Tests cover vertical wrap901A/901D and all220 streamer instruction starts."},
            {"9192","CameraTargetAndApproach","nba_gameplay_camera_update: this goal reverifies212 instructions in seven bounded slices, not all272. 60 init/no-team/orientation/height/fraction correction instructions remain pending. Existing500 replay successes are NOT full-branch proof."},
            {"9352","CameraAdaptiveAxisApproach","Verified73 instruction starts through93F4: one-pixel dead zone, max22 request, prior displacement+2 acceleration, immediate deceleration. No floating-point smoothing substitute."}
        };
        for(String[] c:comments){createLabel(toAddr(Integer.parseInt(c[0],16)),c[1],true);setPlateComment(toAddr(Integer.parseInt(c[0],16)),c[2]+" C port: src/nba_court_presentation.c.");}
        int[][] spans={{0x8e1c,0x8ee5},{0x8ee6,0x90c3},{0x91cb,0x91de},{0x91fb,0x9218},
            {0x9230,0x92bf},{0x92ca,0x92e3},{0x92f9,0x932e},{0x932f,0x9348},{0x9352,0x93f4}};
        Map<String,List<Long>> census=new LinkedHashMap<>();
        census.put("wrapper",new ArrayList<>());census.put("stream",new ArrayList<>());census.put("core",new ArrayList<>());
        File dir=new File(args[0]);dir.mkdirs();
        try(PrintWriter out=new PrintWriter(new File(dir,"court_presentation_bank85.txt"),"UTF-8")) {
            for(int i=0;i<spans.length;++i) {
                int count=0,next=spans[i][0];
                for(int pc=spans[i][0];pc<=spans[i][1];++pc) {
                    Instruction ins=currentProgram.getListing().getInstructionAt(toAddr(pc));
                    if(ins==null)continue;
                    if(pc!=next)throw new Exception("Decode gap at "+Integer.toHexString(next));
                    next=pc+ins.getLength();++count;
                    out.printf("$85:%04X [%d] %s%n",pc,ins.getLength(),ins);
                    census.get(i==0?"wrapper":i==1?"stream":"core").add(0x850000L+pc);
                }
                if(next!=spans[i][1]+1)throw new Exception("Incomplete range");
                out.printf("# %04X-%04X count=%d%n",spans[i][0],spans[i][1],count);
            }
        }
        if(args.length>1)try(PrintWriter out=new PrintWriter(args[1],"UTF-8")) {
            out.println("{");int i=0;
            for(Map.Entry<String,List<Long>> e:census.entrySet())out.println("  \""+e.getKey()+"\": "+e.getValue()+(++i<3?",":""));
            out.println("}");
        }
    }
}
