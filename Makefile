SWWORK=../..

SRCVUTIL=$(SWWORK)/libsrc/vutil
SRCVUTILPLUS=$(SWWORK)/libsrc/vutilplus
SRCSOCKET=$(SWWORK)/libsrc/socket
SRCISO8583=$(SWWORK)/libsrc/iso8583

COMPILER=g++ -g -DSUNOS -I$(SWWORK)/base/include -I$(SRCVUTIL) -I$(SRCVUTILPLUS) -I$(SRCSOCKET) \
		-I$(SRCISO8583) -I$(SWWORK)/base

D=$(SWWORK)/obj
L=$(SWWORK)/lib
EXEC=$(SWWORK)/bin
ADDLIB=-lclntsh -lm -lpthread -ldl /usr/lib/libsocket.so -lnsl -lresolv -laio -lrt -lstdc++

all: socket

clean: cl_socket


#_M_A_K_E___S_C_R_I_P_T___O_F___L_I_B_R_A_R_Y___S_O_C_K_E_T_
.PHONY: socket
LIBSOCKET=$(L)/libsocket.a
SOCKETOBJS=$(D)/listeningsocket.o $(D)/monitoringprovider.o $(D)/socket.hpux.o \
	$(D)/svcprv_st.o $(D)/vsock2.o
SRC=$(SWWORK)/libsrc/socket

socket:$(LIBSOCKET)
$(LIBSOCKET): $(SOCKETOBJS)
	ar r $(LIBSOCKET) $(SOCKETOBJS)
	@echo "------------------------------------------------------------------"

$(D)/listeningsocket.o: $(SRC)/listeningsocket.cpp
	$(COMPILER) -c $^ -o $@
$(D)/monitoringprovider.o: 	$(SRC)/monitoringprovider.cpp
	$(COMPILER) -c $^ -o $@
$(D)/socket.hpux.o: 	$(SRC)/socket.hpux.cpp
	$(COMPILER) -c $^ -o $@
$(D)/svcprv_st.o: 	$(SRC)/svcprv_st.cpp
	$(COMPILER) -c $^ -o $@
$(D)/vsock2.o: 	$(SRC)/vsock2.cpp
	$(COMPILER) -c $^ -o $@

cl_socket:
	rm -f $(SOCKETOBJS) $(LIBSOCKET)
