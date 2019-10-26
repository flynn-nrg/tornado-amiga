EXAMPLES = 3dtest basstest contest custloop devlist dsptest fxtest livespec modtest multi netradio rectest speakers spectrum synth writewav plugins

.PHONY: all clean $(EXAMPLES)

all: $(EXAMPLES)

clean:
	@$(foreach x,$(EXAMPLES),$(MAKE) -C $(x) clean;)

$(EXAMPLES):
	$(MAKE) -C $@


BASSLIBS := $(wildcard libbass*.dylib)

i386: $(foreach x,$(BASSLIBS),i386/$(x))

x86_64: $(foreach x,$(BASSLIBS),x86_64/$(x))

i386/%: %
	@mkdir -p i386
	-lipo $* -extract i386 -output $@

x86_64/%: %
	@mkdir -p x86_64
	-lipo $* -extract x86_64 -output $@
