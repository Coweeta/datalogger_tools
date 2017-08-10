get_protocol_version
'V'

output
'1'


read_whole_file
'Rfilename'


delete_file
'Dfilename'


report_active_file
'A'

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

