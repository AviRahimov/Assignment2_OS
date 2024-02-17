.PHONY: all PartA PartB clean

all: PartA PartB

PartA:
	$(MAKE) -C PartA

PartB:
	$(MAKE) -C PartB

clean:
	$(MAKE) -C PartA clean
	$(MAKE) -C PartB clean
