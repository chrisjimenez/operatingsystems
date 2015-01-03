/////////////////////////////////////////////////////////////////////////////
PROJECT 3 README


To compile:
first…
gcc –m32 -c tlb.c

second…
gcc –m32 –o memory page.c tlb.o cpu.o mmu.o kernel.o

OR 

make proj3


To Run:

.\proj3