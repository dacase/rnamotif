test: nanlin pk1 qu+tr score.1 score.2 trna mp.ends efn

nanlin:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamot -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

pk1:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamot -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
#	@-rm -f $@.test

qu+tr:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamot -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

score.1:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamot -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

score.2:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamot -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

trna:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamot -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

mp.ends:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamot -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

efn:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamot -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

