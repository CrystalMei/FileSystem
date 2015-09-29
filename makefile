all: BDS BDC_command BDC_random IDS_FCFS IDS_SSTF IDS_C_LOOK FS FC clear
clear: 
	rm *.o
BDS: BDS.o
	g++ BDS.o -o BDS
BDS.o:
	g++ -c BDS.c
BDC_command: BDC_command.o
	g++ BDC_command.o -o BDC_command
BDC_command.o:
	g++ -c BDC_command.c
BDC_random: BDC_random.o
	g++ BDC_random.o -o PBDC_random
BDC_random.o:
	g++ -c BDC_random.c
IDS_FCFS: IDS_FCFS.o
	g++ IDS_FCFS.o -o IDS_FCFS
IDS_FCFS.o:
	g++ -c IDS_FCFS.c
IDS_SSTF: IDS_SSTF.o
	g++ IDS_SSTF.o -o IDS_SSTF
IDS_SSTF.o: 
	g++ -c IDS_SSTF.c
IDS_C_LOOK: IDS_C_LOOK.o
	g++ IDS_C_LOOK.o -o IDS_C_LOOK
IDS_C_LOOK.o:
	g++ -c IDS_C_LOOK.c
FS: FS.o
	g++ FS.o -o FS
FS.o:
	g++ -c FS.c
FC: FC.o
	g++ FC.o -o FC
FC.o:
	g++ -c FC.c 
