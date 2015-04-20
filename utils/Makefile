PROTOC ?= protoc
BUFFERS = brush

all: $(addsuffix _pb2.py,$(BUFFERS))

%_pb2.py: ../game/%.proto
	cd ../game/; $(PROTOC) --python_out=../utils/ $(notdir $<)
