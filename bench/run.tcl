#!/usr/bin/env cfkit8.6
# vim: ft=tcl foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4

set big	[string repeat a [expr {int(1e8)}]]	;# Allocate 100MB to pre-expand the zippy pool
unset big

set here	[file dirname [file normalize [info script]]]
tcl::tm::path add $here

package require platform
package require bench

proc with_chan {var create use} {
	upvar 1 $var h
	set h	[uplevel 1 [list if 1 $create]]
	try {
		uplevel 1 [list if 1 $use]
	} on return {r o} - on break {r o} - on continue {r o} {
		dict incr o -level 1
		return -options $o $r
	} finally {
		if {[info exists h] && $h in [chan names]} {
			catch {close $h}
		}
	}
}

proc readtext fn { with_chan h {open $fn r} {read $h} }


proc benchmark_mode script {
	uplevel 1 $script
}

proc main {} {
	try {
		set here	[file dirname [file normalize [info script]]]
		# Ensure that we load the version from our source repo even if the system already has a version
		set srcver	[lindex $::argv 0]
		benchmark_mode {
			puts "[string repeat - 80]\nStarting benchmarks\n"
			bench::run_benchmarks $here {*}[lrange $::argv 1 end]
		}
	} on ok {} {
		exit 0
	} trap {BENCH BAD_RESULT} {errmsg options} {
		puts stderr $errmsg
		exit 1
	} trap {BENCH BAD_CODE} {errmsg options} {
		puts stderr $errmsg
		exit 1
	} trap {BENCH INVALID_ARG} {errmsg options} {
		puts stderr $errmsg
		exit 1
	} trap exit code {
		exit $code
	} on error {errmsg options} {
		puts stderr "Unhandled error from benchmark_mode: [dict get $options -errorinfo]"
		exit 2
	}
}

main

# vim: ft=tcl foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4
