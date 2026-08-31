"""Pinned reference profile, reconstructed without candidate/native timestamps.

Only the first normal NMI is owned. Auto-joypad catch-up call sites are the
reference software's lazy observations, not a universal physical timing claim.
Actual controller device effects are deliberately refused.
"""
from bootstrap_trace_protocol_v2 import require

class Clock:
    def __init__(self):
        self.master=186;self.h=186;self.line=0;self.odd=False
        self.refresh_at=538;self.refreshes=0;self.spc=4;self.due=[]
        self.enabled=False;self.flag=False;self.need=False;self.delay=0
        self.irq=False;self.open_bus=0
        self.auto=False;self.active=False;self.disabled=True
        self.start=0;self.next=0;self.strobe=False

    def set_strobe(self,value):
        require(value==self.strobe,'controller device latch is outside checkpoint')

    def catchup(self):
        if self.master<self.start:return
        if self.disabled:
            if self.master-self.start>=256:self.set_strobe(False)
            return
        while self.next<=self.master:
            step=(self.next-self.start)//128;self.next+=128
            if step==0:self.set_strobe(self.auto)
            elif step==1:
                require(not self.auto,'auto controller register reset outside checkpoint')
                self.disabled=True;self.active=False
            elif step==2:self.set_strobe(False)
            elif not self.auto:step=34
            else:require(False,'auto controller serial read/shift outside checkpoint')
            if step>=34:
                self.disabled=True;self.active=False;self.set_strobe(False);return
        if not self.auto and self.master-self.start>=384:
            self.disabled=True;self.active=False;self.set_strobe(False)

    def tick(self):
        self.master+=2;self.h+=2
        if self.h==self.refresh_at:
            self.refreshes+=1
            for _ in range(20):self.tick()
        if self.h>=1364 or(self.h==1360 and self.line==240 and self.odd):
            self.h=0;self.line+=1
            if self.line==262:self.line=0;self.odd=not self.odd
            self.refresh_at=538-(self.master&7)
            self.catchup()
            if self.line==225:
                self.start=((self.master+130+255)//256)*256-128
                self.next=self.start;self.disabled=False
        if self.h==2:
            if self.line==225:self.flag=True
            elif self.line==0:self.flag=False
        if self.h==6 and self.line==225 and self.enabled:self.delay=1
        target=self.master*2050560//21477270
        while self.spc+1<target:
            self.spc+=2;self.due.append((self.master,self.spc))

    def advance(self,n):
        require(type(n)is int and n>=0 and n%2==0,'profile cycle duration')
        for _ in range(n//2):self.tick()

    def cpu_cycle(self,locked):
        if self.delay:
            self.delay-=1
            if self.delay==0:
                if locked:self.delay=1;self.need=False
                else:self.need=True

    def control_write(self,value):
        require(not value&0x30,'H/V IRQ modes outside checkpoint')
        automatic=bool(value&1);enabled=bool(value&128)
        if automatic!=self.auto:
            self.catchup()
            if self.start<=self.master and self.master-self.start<256:
                self.set_strobe(automatic)
        self.auto=automatic
        if self.flag and enabled and not self.enabled:self.delay=2
        self.enabled=enabled;self.irq=False

    def read(self,address):
        if address==0x4210:
            value=(128 if self.flag else 0)|2|(self.open_bus&112)
            if self.flag and(self.h>=6 or self.line!=225):self.flag=False
        else:
            require(address==0x4211,'owned internal register read')
            value=(128 if self.irq else 0)|(self.open_bus&127);self.irq=False
        return value

    def projection(self):
        return {'cpu.needNmi':self.need,'cpu.nmiFlagCounter':self.delay,
                'internalRegisters.enableNmi':self.enabled,'internalRegisters.nmiFlag':self.flag,
                'internalRegisters.irqFlag':self.irq,'internalRegisters.enableAutoJoypadRead':self.auto,
                'internalRegisters.autoReadActive':self.active,'internalRegisters.autoReadDisabled':self.disabled,
                'internalRegisters.autoReadClockStart':self.start,'internalRegisters.autoReadNextClock':self.next,
                'controlManager.autoReadStrobe':self.strobe,'memoryManager.openBus':self.open_bus}
