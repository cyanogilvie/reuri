if {[catch {package require dedup} err]} {
	puts stderr "dedup package not found: $err"
	exit 1
}
puts [dedup::pkgconfig get packagedir,runtime]
