#######################################
Logger - Laptop Communications Protocol
#######################################

To wake up the datalogger from power-save mode send a space character.
Spaces preceding the command character are otherwise ignored by the Arduino.
Allow a few milliseconds for the hardware to be able to read incoming data.


Every request from the laptop will trigger a response.

In addition to responses the embedded code may output messages - in particular
log output from events if direct reporting is enabled.  These lines are always
preceded with '_'

get_protocol_version
####################

Lets the laptop application know what protocol the Arduino program is running.

Send
====

=== ===
'V' NUL
=== ===

Receive
=======

=== ===== ===
'V' '0.0' NUL
=== ===== ===

list event names
################

Request the names defined in the embedded code for each of the supported events.

Send
====

=== ===
'n' NUL
=== ===



Receive
=======

=== ====== ======= ====== ======= === ====== ======= ===
'n' name 0 NEWLINE name 1 NEWLINE ... name N NEWLINE NUL
=== ====== ======= ====== ======= === ====== ======= ===



trigger_event
#############

Manually trigger a set of events.

Send
====

=== ========== ===
'e' event_mask NUL
=== ========== ===

`event_mask` is a uint16_t whose bits correspond with the events defined in
embedded code's `EventSchedule`

Receive
=======

=== ===
'e' NUL
=== ===


enable_event
############

If bit is set corresponding event will trigger according to its schedule.
This doesn't affect the manually triggered events.

Send
====

=== ========== ===
'E' event_mask NUL
=== ========== ===

`event_mask` is a uint16_t whose bits correspond with the events defined in
embedded code's `EventSchedule`

Receive
=======

=== ===
'e' NUL
=== ===


report wait for next event
##########################

Request the length of the delay until the next scehduled event.
Also gives which event will occur at this time.

Send
====

=== ===
'w' NUL
=== ===



Receive
=======

=== ========== ===
'w' event_mask NUL
=== ========== ===



`event_mask` is a uint16_t whose bits correspond with the events defined in
embedded code's `EventSchedule`



get time
########

Lets the embedded code report its understanding of the current time and date.

This will be Zulu time and precise to hundreths of a second.  The

Send
====

=== ===
't' NUL
=== ===

Receive
=======

=== ======================== ===
't' `YYYY-MM-DD_HH:MM:SS.HH` NUL
=== ======================== ===

The underscore is replaced with a space.

E.g. '2017-08-11 13:01:34.56'



set time
########

Sets the current time and date in the embedded system.  The hundredths
of seconds field is set to 0

This will be Zulu time.

Send
====

=== ===================== ===
'T' `YYYY-MM-DD_HH:MM:SS` NUL
=== ===================== ===

Receive
=======

=== ===
'T' NUL
=== ===

adjust time
###########

Sets the current time and date in the embedded system.  The hundredths
of seconds field is set to 0

This will be Zulu time.

Send
====

=== ==== ======= ===
'a' sign `SS.HH` NUL
=== ==== ======= ===

If sign == '+' increase the time by that much, if it is '-' then decrement by
this amount.

Receive
=======

=== ==== ===
'a' okay NUL
=== ==== ===

if okay == '1' then change worked.  Otherwise try again in a little bit.
We might have hit an hour boundary and the code would be a bit tricky.



list files
##########

Request the names, sizes and datestamps of the file in the root of the SD card.
Directories are flagged by having a size of '/'



Send
====

=== ===
'L' NUL
=== ===



Receive
=======

=== == ====== ===== ==== ===== ========== == ====== ===== ==== ===== ========== == === ====== ===== ==== ===== ========== == ===
'L' NL name 0 SPACE size SPACE date stamp NL name 1 SPACE size SPACE date stamp NL ... name N SPACE size SPACE date stamp NL NUL
=== == ====== ===== ==== ===== ========== == ====== ===== ==== ===== ========== == === ====== ===== ==== ===== ========== == ===


remove file
###########

Request the given file is deleted


Send
====

=== ========= ===
'R' file name NUL
=== ========= ===



Receive
=======

=== ===
'R' NUL
=== ===


advance active file
###################

Stop logging to the current active file and start on the next available.


Send
====

=== ===
'a' NUL
=== ===



Receive
=======

=== ========= ===
'a' file name NUL
=== ========= ===

get active file
###############

Get the name of the file currently being written to.


Send
====

=== ===
'A' NUL
=== ===



Receive
=======

=== ========= ===
'A' file name NUL
=== ========= ===


get EEPROM text
###############

Dump the text stored in the eeprom.


Send
====

=== ===
'M' NUL
=== ===



Receive
=======

=== ==== ===
'M' text NUL
=== ==== ===


'wwwwwwnn'

The last two characters are digits.

Switch to the next file on reset, or when the current file reaches a given size.
The next filename is stored in nvram.

new_active_file
'Nwwwwwwwnn'


get_logger_time
'T'


set_logger_time
'Stime'


list_all_files
'L'


show_file_head
'Hfilename'


show_file_tail
'Tfilename'


set_eeprom_text
'Qtext'

Stored on the Arduino card.  Not changed once a system is installed.

get_eeprom_text
'E'


get_wait_time
'W'


execute_dry_run
'X'

