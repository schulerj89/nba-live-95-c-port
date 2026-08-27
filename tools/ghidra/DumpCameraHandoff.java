// args: output directory; current program bank85, bank86 or bank87.
import java.io.*;
import java.math.BigInteger;
import java.util.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;
public class DumpCameraHandoff extends GhidraScript {
    public void run() throws Exception {
        int bank=currentProgram.getName().contains("87")?0x87:
            currentProgram.getName().contains("86")?0x86:0x85;
        int[][] ranges=bank==0x85?new int[][]{{0x8b98,0x8bbe},{0x9192,0x93f4}}:
            bank==0x86?new int[][]{{0xd392,0xd39c},{0xe1a6,0xe1ab}}:
            new int[][]{{0x95ac,0x95de},{0xa9d0,0xa9e2}};
        for(int[] r:ranges) {
            clearListing(toAddr(r[0]),toAddr(r[1]));
            for(String n:new String[]{"M","X"}) {
                Register reg=currentProgram.getRegister(n);
                if(reg!=null)currentProgram.getProgramContext().setValue(reg,toAddr(r[0]),toAddr(r[1]),BigInteger.ZERO);
            }
        }
        for(int[] r:ranges)disassemble(toAddr(r[0]));
        if(bank==0x85)for(int p:new int[]{0x9197,0x919f,0x91cb,0x91eb,0x91f9,0x9276,0x92c0,0x92ca,0x92e9,0x92f9,0x9312,0x932f,0x934e,0x9352,0x93a3})disassemble(toAddr(p));
        String[][] names=bank==0x85?new String[][]{
            {"8b98","CameraInitialSubjectPlacement","13 instructions: clear4A54, copy pointer0940 or ball3EEB XY, call9192. Native first pass directly places camera, preserving previous/command fields."},
            {"9192","CameraCompleteTargetAndHistory","Full camera state: initialization4A54, edge fractions, basket-sign orientation, no-team projected centering and alternate ball height08BC/08CC. See camera-handoff-plan.md; caller inputs are an independent proof boundary."}
        }:bank==0x86?new String[][]{
            {"d392","TipAcquisitionClearLiveState","If whistle09B6 is zero, clear0936. Native write callback nextPC D39D, frame220. Bounded connector, not proof of complete acquisition dispatcher."},
            {"e1a6","TipInstallPresentationLiveState","Install0936=0081 before tip. Native write callback nextPC E1AC. Distinct from shot-flight0001 used by camera height gate."}
        }:new String[][]{
            {"95ac","CameraPresentationCadence","Resolve093E before waiting for unsigned0564>=2; reset0564 then copy the previously resolved0940 subject. Portable frame credit, not busy-wait emulation."},
            {"95bb","CameraCopySubjectAndDispatch","Copy 16.16 XY from0940 (zero substitutes ball3EEB) to4A56..4A5C, then presentation wrapper8E1C."},
            {"a9d0","CameraResolveActorOrBall","Signed093E<0 clears0940; otherwise table879C7B selects an actor record. Do not infer the selector solely from a presentation frame count."}
        };
        for(String[] n:names){createLabel(toAddr(Integer.parseInt(n[0],16)),n[1],true);setPlateComment(toAddr(Integer.parseInt(n[0],16)),n[2]);}
        File dir=new File(getScriptArgs()[0]);dir.mkdirs();
        try(PrintWriter out=new PrintWriter(new File(dir,String.format("camera_bank%02x.txt",bank)),"UTF-8")) {
            for(int[] r:ranges) {
                int next=r[0],count=0;
                for(int pc=r[0];pc<=r[1];pc++) {
                    Instruction ins=currentProgram.getListing().getInstructionAt(toAddr(pc));
                    if(ins==null)continue;
                    if(pc!=next)throw new Exception("decode gap "+Integer.toHexString(next));
                    next=pc+ins.getLength();count++;
                    out.printf("$%02X:%04X [%d] %s%n",bank,pc,ins.getLength(),ins);
                }
                if(next!=r[1]+1)throw new Exception("incomplete range");
                out.printf("# %04X-%04X count=%d%n",r[0],r[1],count);
            }
        }
    }
}
