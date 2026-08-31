import java.io.*;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;

public class DumpHumanPassPose extends GhidraScript {
 public void run() throws Exception {
  int bank=Integer.parseInt(getScriptArgs()[1],16);
  int[][] ranges=bank==0x86?new int[][]{{0xaf1d,0xaf65}}:
   new int[][]{{0xaec3,0xaf74},{0xb649,0xb669},{0xb832,0xb952}};
  for(int[] range:ranges){
   clearListing(toAddr(range[0]),toAddr(range[1]));
   for(String name:new String[]{"M","X"}){
    Register r=currentProgram.getRegister(name);
    if(r!=null)currentProgram.getProgramContext().setValue(r,toAddr(range[0]),toAddr(range[1]),BigInteger.ZERO);
   }
   disassemble(toAddr(range[0]));
  }
  if(bank==0x86)disassemble(toAddr(0xaf30));
  try(PrintWriter out=new PrintWriter(new File(getScriptArgs()[0],String.format("human_pass_pose_bank%02x.txt",bank)),"UTF-8")){
   for(int[] range:ranges)for(int pc=range[0];pc<=range[1];++pc){
    Instruction ins=currentProgram.getListing().getInstructionAt(toAddr(pc));
    if(ins!=null)out.printf("$%02X:%04X [%d] %s%n",bank,pc,ins.getLength(),ins);
   }
  }
 }
}
