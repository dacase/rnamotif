test:	test.slack test.strict
test.slack: 	\
	nanlin		\
	pk1		\
	qu+tr		\
	score.1		\
	score.2		\
	trna		\
	mp.ends		\
	efn		\
	sprintf		\
	bulge		\
	getbest

test.strict:	\
	nanlin.strict	\
	pk1.strict	\
	qu+tr.strict	\
	score.1.strict	\
	score.2.strict	\
	trna.strict	\
	mp.ends.strict	\
	efn.strict	\
	sprintf.strict	\
	bulge.strict	\
	getbest.strict

nanlin:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamotif -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

pk1:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamotif -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

qu+tr:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamotif -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

score.1:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamotif -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

score.2:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamotif -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

trna:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamotif -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

mp.ends:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamotif -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

efn:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamotif -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

sprintf:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamotif -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

bulge:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamotif -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

getbest:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamotif -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test


nanlin.strict:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamotif -sh -context -Dctx_maxlen=5 -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

pk1.strict:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamotif -sh -context -Dctx_maxlen=5 -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

qu+tr.strict:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamotif -sh -context -Dctx_maxlen=5 -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

score.1.strict:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamotif -sh -context -Dctx_maxlen=5 -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

score.2.strict:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamotif -sh -context -Dctx_maxlen=5 -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

trna.strict:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamotif -sh -context -Dctx_maxlen=5 -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

mp.ends.strict:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamotif -sh -context -Dctx_maxlen=5 -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

efn.strict:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamotif -sh -context -Dctx_maxlen=5 -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

sprintf.strict:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamotif -sh -context -Dctx_maxlen=5 -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

bulge.strict:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamotif -sh -context -Dctx_maxlen=5 -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

getbest.strict:
	@echo =====================================================
	@echo   "Running $@ test:"
	@echo
	@../src/rnamotif -sh -context -Dctx_maxlen=5 -descr $@.descr gbrna.111.0.fastn | ../src/rmfmt -l > $@.test
	@-diff $@.chk $@.test && echo "  PASSED" \
		|| echo "   FAILED (possibly; see if diffs above look OK)"
	@-rm -f $@.test

