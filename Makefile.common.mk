
$(OUTDIR)/%.o: %.c | $(OUTDIR)
	@echo "[CC]" $< "->" $@
	@($(CC) $< $(CFLAGS) -c -o $@)

$(OUTDIR)/%.o: %.cc | $(OUTDIR)
	@echo "[CXX]" $< "->" $@
	@($(CXX) $< $(CXXFLAGS) -c -o $@)

$(OUTDIR)/%.o: %.S | $(OUTDIR)
	@echo "[CC]" $< "->" $@
	@($(CC) $< $(CFLAGS) -c -o $@)

$(OUTDIR):
	@echo "[MKDIR]" $@
	@mkdir -p $@