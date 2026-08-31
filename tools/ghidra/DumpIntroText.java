// Bounded font/canvas routines used by $80:FD9E-$FF3E, with indirect entries.
import java.io.File;
import java.io.PrintWriter;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Instruction;
public class DumpIntroText extends GhidraScript {
 public void run() throws Exception {
  long[] entries={0x9756,0x99ec,0x9b09,0x9f54,0x9fdf,0xa05f,0xa163,0xa1e7};
  for(long p:entries)disassemble(toAddr(p));
  // $81:9AF9 is the shift-dispatch table; each target owns complete code.
  for(int i=0;i<8;i++)disassemble(toAddr(getShort(toAddr(0x9af9+i*2))&0xffff));
  try(PrintWriter out=new PrintWriter(new File(getScriptArgs()[0]),"UTF-8")) {
   for(Instruction i:currentProgram.getListing().getInstructions(toAddr(0x9756),true)) {
    if(i.getAddress().getOffset()>0xa241)break;
    out.printf("81:%04X  %s%n",i.getAddress().getOffset(),i.toString());
   }
  }
 }
}
