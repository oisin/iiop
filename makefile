# Use `gmake dep' to make the dependency list in this makefile
# use `gmake all' to make the entire load. Object files can be made 
# independently.
#
# Caveats:
# This makefile uses gmake specific constructs to make compilation on
# three CPU types simple. It cannot be used with vanilla make or Sun make.
#
# Change Log:
#
# 01dec97    omh    file creation
TARGETLD = /usr/bin/ld
TARGETNM = /usr/bin/nm
TARGETCC = /usr/bin/cc
TARGETMUNCH = /bin/cat

OBJECTS = ./iiop/reader.o ./iiop/init.o ./iiop/iiopmsgp.o       \
          ./iiop/libgiop.o  ./common/fdset.o ./common/skts.o        \
          ./common/fd_q_map.o ./tcpagent/selecter.o                 \
          ./tcpagent/tcpnotify.o  ./tcpagent/detector.o             \
          ./tcpagent/iiopagent.o  

all: IIOP_m blob

IIOP: $(OBJECTS)
	$(TARGETLD) -r $(OBJECTS) -o IIOP

IIOP_m: IIOP __c.c __c.o
	$(TARGETLD) -r IIOP __c.o -o IIOP_m

blob: __c2.c __c2.o
	$(TARGETLD) -r IIOP $(TARGETOBJS)/orb_srcs.o __c2.o -o BLOB_m
	cp BLOB_m $(TARGETOBJS)/
	$(RM) __c2.c __c2.o

__c2.c:
	$(TARGETNM) IIOP $(TARGETOBJS)/orb_srcs.o | $(TARGETMUNCH) > __c2.c


__c.c:
	$(TARGETNM) $(OBJECTS) | $(TARGETMUNCH) > __c.c

.c.o:
	$(TARGETCC) -c $(TARGETCOPTS) -traditional $<

.cc.o:
	$(TARGETCC) -c$(TARGETOPTS) $<

dep:
	makedepend ) $(SOURCES)

clean:
	/bin/rm -f IIOP IIOP_m
	/bin/rm -f $(OBJECTS)
	/bin/rm -f __c.c __c.o  __c2.c __c2.o
	/bin/rm -f BLOB_m

scrub:	clean
	/bin/rm -f *.bak *~ 
	

# DO NOT DELETE THIS LINE -- make depend depends on it.
