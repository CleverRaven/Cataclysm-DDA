	.file	"t.cpp"
# GNU C++11 (GCC) version 5.3.0 (i686-pc-linux-gnu)
#	compiled by GNU C version 5.3.0, GMP version 6.0.0, MPFR version 3.1.3-p4, MPC version 1.0.3
# GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
# angegebene Optionen: 
# -iprefix /packages/gcc/5.3.0/usr/bin/../lib/gcc/i686-pc-linux-gnu/5.3.0/
# -D_GNU_SOURCE t.cpp -mtune=generic -march=pentiumpro -O3 -std=c++11
# -fverbose-asm
# angeschaltete Optionen:  -faggressive-loop-optimizations -falign-labels
# -fasynchronous-unwind-tables -fauto-inc-dec -fbranch-count-reg
# -fcaller-saves -fchkp-check-incomplete-type -fchkp-check-read
# -fchkp-check-write -fchkp-instrument-calls -fchkp-narrow-bounds
# -fchkp-optimize -fchkp-store-bounds -fchkp-use-static-bounds
# -fchkp-use-static-const-bounds -fchkp-use-wrappers
# -fcombine-stack-adjustments -fcommon -fcompare-elim -fcprop-registers
# -fcrossjumping -fcse-follow-jumps -fdefer-pop
# -fdelete-null-pointer-checks -fdevirtualize -fdevirtualize-speculatively
# -fdwarf2-cfi-asm -fearly-inlining -feliminate-unused-debug-types
# -fexceptions -fexpensive-optimizations -fforward-propagate -ffunction-cse
# -fgcse -fgcse-after-reload -fgcse-lm -fgnu-runtime -fgnu-unique
# -fguess-branch-probability -fhoist-adjacent-loads -fident -fif-conversion
# -fif-conversion2 -findirect-inlining -finline -finline-atomics
# -finline-functions -finline-functions-called-once
# -finline-small-functions -fipa-cp -fipa-cp-alignment -fipa-cp-clone
# -fipa-icf -fipa-icf-functions -fipa-icf-variables -fipa-profile
# -fipa-pure-const -fipa-ra -fipa-reference -fipa-sra -fira-hoist-pressure
# -fira-share-save-slots -fira-share-spill-slots
# -fisolate-erroneous-paths-dereference -fivopts -fkeep-static-consts
# -fleading-underscore -flifetime-dse -flra-remat -flto-odr-type-merging
# -fmath-errno -fmerge-constants -fmerge-debug-strings
# -fmove-loop-invariants -fomit-frame-pointer -foptimize-sibling-calls
# -foptimize-strlen -fpartial-inlining -fpcc-struct-return -fpeephole
# -fpeephole2 -fpredictive-commoning -fprefetch-loop-arrays -free
# -freorder-blocks -freorder-blocks-and-partition -freorder-functions
# -frerun-cse-after-loop -fsched-critical-path-heuristic
# -fsched-dep-count-heuristic -fsched-group-heuristic -fsched-interblock
# -fsched-last-insn-heuristic -fsched-rank-heuristic -fsched-spec
# -fsched-spec-insn-heuristic -fsched-stalled-insns-dep -fschedule-fusion
# -fschedule-insns2 -fsemantic-interposition -fshow-column -fshrink-wrap
# -fsigned-zeros -fsplit-ivs-in-unroller -fsplit-wide-types -fssa-phiopt
# -fstdarg-opt -fstrict-aliasing -fstrict-overflow
# -fstrict-volatile-bitfields -fsync-libcalls -fthread-jumps
# -ftoplevel-reorder -ftrapping-math -ftree-bit-ccp -ftree-builtin-call-dce
# -ftree-ccp -ftree-ch -ftree-coalesce-vars -ftree-copy-prop
# -ftree-copyrename -ftree-cselim -ftree-dce -ftree-dominator-opts
# -ftree-dse -ftree-forwprop -ftree-fre -ftree-loop-distribute-patterns
# -ftree-loop-if-convert -ftree-loop-im -ftree-loop-ivcanon
# -ftree-loop-optimize -ftree-loop-vectorize -ftree-parallelize-loops=
# -ftree-partial-pre -ftree-phiprop -ftree-pre -ftree-pta -ftree-reassoc
# -ftree-scev-cprop -ftree-sink -ftree-slp-vectorize -ftree-slsr -ftree-sra
# -ftree-switch-conversion -ftree-tail-merge -ftree-ter -ftree-vrp
# -funit-at-a-time -funswitch-loops -funwind-tables -fverbose-asm
# -fzero-initialized-in-bss -m32 -m80387 -m96bit-long-double
# -malign-stringops -mavx256-split-unaligned-load
# -mavx256-split-unaligned-store -mfancy-math-387 -mfp-ret-in-387 -mglibc
# -mieee-fp -mlong-double-80 -mno-red-zone -mno-sse4 -mpush-args -msahf
# -mtls-direct-seg-refs -mvzeroupper

	.section	.text.unlikely,"ax",@progbits
.LCOLDB0:
	.text
.LHOTB0:
	.p2align 4,,15
	.globl	_Z4findiR6int_idI3fooE
	.type	_Z4findiR6int_idI3fooE, @function
_Z4findiR6int_idI3fooE:
.LFB1234:
	.cfi_startproc
	subl	$12, %esp	#,
	.cfi_def_cfa_offset 16
	movl	other_id, %edx	# other_id, other_id
	movl	20(%esp), %eax	# id, id
	movl	%edx, (%eax)	# other_id, *id_2(D)
	call	_Z5frandv	#
	cmpl	16(%esp), %eax	# i, D.31171
	sete	%al	#, D.31172
	addl	$12, %esp	#,
	.cfi_def_cfa_offset 4
	ret
	.cfi_endproc
.LFE1234:
	.size	_Z4findiR6int_idI3fooE, .-_Z4findiR6int_idI3fooE
	.section	.text.unlikely
.LCOLDE0:
	.text
.LHOTE0:
	.section	.text.unlikely
.LCOLDB1:
	.text
.LHOTB1:
	.p2align 4,,15
	.globl	_Z5test2v
	.type	_Z5test2v, @function
_Z5test2v:
.LFB1235:
	.cfi_startproc
	pushl	%ebx	#
	.cfi_def_cfa_offset 8
	.cfi_offset 3, -8
	subl	$8, %esp	#,
	.cfi_def_cfa_offset 16
	cmpb	$0, _ZGVZ5test2vE3sid	#, MEM[(char *)&_ZGVZ5test2vE3sid]
	movl	16(%esp), %ebx	# .result_ptr, .result_ptr
	je	.L13	#,
.L5:
	movl	other_id, %eax	# other_id, other_id
	movl	%eax, _ZZ5test2vE3sid	# other_id, sid
	call	_Z5frandv	#
	testl	%eax, %eax	# D.31176
	jne	.L14	#,
	movl	_ZZ5test2vE3sid, %eax	# sid, sid
	movl	%eax, (%ebx)	# sid, <retval>
	addl	$8, %esp	#,
	.cfi_remember_state
	.cfi_def_cfa_offset 8
	movl	%ebx, %eax	# .result_ptr,
	popl	%ebx	#
	.cfi_restore 3
	.cfi_def_cfa_offset 4
	ret	$4	#
	.p2align 4,,10
	.p2align 3
.L13:
	.cfi_restore_state
	subl	$12, %esp	#,
	.cfi_def_cfa_offset 28
	pushl	$_ZGVZ5test2vE3sid	#
	.cfi_def_cfa_offset 32
	call	__cxa_guard_acquire	#
	addl	$16, %esp	#,
	.cfi_def_cfa_offset 16
	testl	%eax, %eax	# D.31176
	je	.L5	#,
	subl	$12, %esp	#,
	.cfi_def_cfa_offset 28
	pushl	$_ZGVZ5test2vE3sid	#
	.cfi_def_cfa_offset 32
	call	__cxa_guard_release	#
	addl	$16, %esp	#,
	.cfi_def_cfa_offset 16
	jmp	.L5	#
.L14:
	subl	$12, %esp	#,
	.cfi_def_cfa_offset 28
	pushl	$2	#
	.cfi_def_cfa_offset 32
	call	exit	#
	.cfi_endproc
.LFE1235:
	.size	_Z5test2v, .-_Z5test2v
	.section	.text.unlikely
.LCOLDE1:
	.text
.LHOTE1:
	.section	.text.unlikely
.LCOLDB2:
	.section	.text.startup,"ax",@progbits
.LHOTB2:
	.p2align 4,,15
	.globl	main
	.type	main, @function
main:
.LFB1236:
	.cfi_startproc
	leal	4(%esp), %ecx	#,
	.cfi_def_cfa 1, 0
	andl	$-16, %esp	#,
	pushl	-4(%ecx)	#
	pushl	%ebp	#
	.cfi_escape 0x10,0x5,0x2,0x75,0
	movl	%esp, %ebp	#,
	pushl	%ecx	#
	.cfi_escape 0xf,0x3,0x75,0x7c,0x6
	leal	-12(%ebp), %eax	#, tmp89
	subl	$32, %esp	#,
	pushl	%eax	# tmp89
	call	_Z5test2v	#
	pushl	-12(%ebp)	#
	call	_Z4sink6int_idI3fooE	#
	movl	-4(%ebp), %ecx	#,
	.cfi_def_cfa 1, 0
	addl	$16, %esp	#,
	xorl	%eax, %eax	#
	leave
	.cfi_restore 5
	leal	-4(%ecx), %esp	#,
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE1236:
	.size	main, .-main
	.section	.text.unlikely
.LCOLDE2:
	.section	.text.startup
.LHOTE2:
	.local	_ZZ5test2vE3sid
	.comm	_ZZ5test2vE3sid,4,4
	.local	_ZGVZ5test2vE3sid
	.comm	_ZGVZ5test2vE3sid,8,8
	.ident	"GCC: (GNU) 5.3.0"
	.section	.note.GNU-stack,"",@progbits
