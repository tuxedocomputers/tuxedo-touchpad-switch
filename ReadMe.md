0x22 0x00 0x37 0x03 0x23 0x00 0x04 0x00 0x07 0x03
 [^^   ^^] (?max?) length of report? in little endian
            ^ 2 bits reserved + report type
             ^ report id (defined by touchpad controller?
                 ^ reserved
                  ^ opcode (0x2 = GET_REPORT, 0x3 = SET_REPORT)
                     [^^   ^^   ^^   ^^   ^^] ?i2c-hid stuff?
                                           ^ ?report id again?
                                               ^^ report/payload: defined by touchpad controller? 0x03 enabled (0x02 auch, keine ahnung was das unterste bit tut), 0x00 disabled
