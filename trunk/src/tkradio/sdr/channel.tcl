# -*- mode: Tcl; tab-width: 8; -*-
#
# Copyright (C) 2016 by Roger E Critchlow Jr, Cambridge, MA, USA.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
# 

package provide sdr::channel 1.0

package require Tcl
package require snit

##
## channel handlers
##
namespace eval ::sdr {}

snit::type sdr::connection {
    option -host
    option -port
    option -status

    variable data -array {
	channel {}
	channel-status {}
	spectrum-listeners {}
	bandscope-listeners {}
	audio-listeners {}
	answer-listeners {}
	status-listeners {}
    }

    ##
    ## life cycle
    ##
    constructor {args} {
	$self configure {*}$args
    }
	
    destructor {} {
	$self disconnect
    }

    ##
    ## listener management
    ##
    method add-listener {queue prefix} {
	if { ! [info exists data($queue-listeners)]} {
	    error "no connection listeners to $queue"
	}
	if {[lsearch $data($queue-listeners) $prefix] >= 0} {
	    error "$prefix is already listening to $queue"
	}
	lappend data($queue-listeners) $prefix
    }

    method sub-listener {queue prefix} {
	if { ! [info exists data($queue-listeners)]} {
	    error "no connection listeners to $queue"
	}
	set i [lsearch $data($queue-listeners) $prefix]
	if {$i < 0} {
	    error "$prefix is not listening to $queue"
	}
	set data($queue-listeners) [lreplace $data($queue-listeners) $i $i]
    }
    
    method process {queue args} {
	foreach prefix $data($queue-listeners) {
	    {*}$prefix {*}$args
	}
    }
    
    ##
    ## connection management
    ##
    method connect {} {
	if {[catch {socket $options(-host) $options(-port)} data(channel)]} {
	    $self process status "connection to $options(-host) $options(-port) refused: $error\n$::errorInfo"
	}
	chan configure $data(channel) -blocking 0 -encoding binary -translation binary
	fileevent $data(channel) readable [mymethod reader]
	# fileevent $data(channel) writable [mymethod writer]
    }

    method disconnect {} {
	fileevent $data(channel) readable {}
	# fileevent $data(channel) writable {}
	close $data(channel)
    }

    ##
    ## reader
    ##
    method reader {} {
	while {1} {
	    if {[catch {read $data(channel)} buffer]} {
		$self process status "error on read $data(channel), $error, $::errorInfo"
		return
	    }
	    while {[set len [string length $buffer]] > 0} {
		switch [string index $buffer 0] {
		    \0 {		# spectrum buffer
			if {[binary scan $buffer cccSSSIS type version subversion samples main sub sr lo] != 8} {
			    $self process status "misread spectrum header"
			    return
			}
			# puts stderr "spectrum t $type, v $version, subv $subversion, samples $samples, main $main, sub $sub, sr $sr, lo $lo"
			set end [expr {15+$samples-1}]
			if {$len < $end} {
			    $self process status "need longer spectrum buffer: $len < $end"
			    return
			}
			set data [string range $buffer 15 $end]
			set buffer [string range $buffer $end+1 end]
			$self process spectrum $main $sub $sr $lo $data
			continue
		    }
		    \1 {		# audio
			if {[binary scan $buffer cccS type version subversion samples] != 4} {
			    $self process status "misread audio header"
			    return
			}
			# puts "audio buffer type $type, version $version, subversion $subversion, samples $samples"
			set end [expr {5+$samples-1}]
			if {$len < $end} {
			    # puts stderr "audio buffer type $type, version $version, subversion $subversion, samples $samples"
			    $self process status "need a longer audio string: $len < $end"
			    return
			}
			set data [string range $buffer 5 $end]
			set buffer [string range $buffer $end+1 end]
			$self process audio $data
			continue
		    }
		    \2 {			# bandscope
			$self process status "unhandled bandscope buffer"
			return
		    }
		    \3 {			# rtp reply buffer
			$self process status "unhandled rtp-reply buffer"
			return
		    }
		    4 {		# answer buffer
			if {$len < 3} {
			    $self process status "need a longer answer string: $len < 3"
			    return
			}
			set samples [scan [string range $buffer 1 2] %d]
			set end [expr {3+$samples}]
			if {$len < $end} {
			    $self process status "need a longer answer string: $len < $end"
			    return
			}
			set data [string range $buffer 3 $end-1]
			set buffer [string range $buffer $end end]
			$self process answer $data
			continue
		    }
		    default {
			binary scan $buffer c type
			$self process status "unknown buffer type $type"
			return
		    }
		}
	    }
	    if {[catch {
		if {[eof $data(channel)]} {
		    $self process status "eof on read $data(channel)"
		    return
		}
	    } error]} {
		$self process status "error testing eof $data(channel)"
		return
	    }
	    if {[catch {
		if {[fblocked $data(channel)]} {
		    # puts "::sdr::reader $radio $data(channel) -> fblocked is true"
		    return;
		}
	    } error]} {
		$self process status "error testing fblocked $data(channel)"
		return
	    }
	}
    }

    method writer {} {
	puts stderr "::sdr::writer $data(channel)"
    }
}
