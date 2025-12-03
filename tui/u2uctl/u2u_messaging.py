#!/usr/bin/python3



class Message():
    def __init__(self):
        self.topic_list = [ "HAIL____", "HELP____", "SET_LCD_", "SET_OLED", "GET_SNSR", "SET_ENCR", "GET_ENCR", "SET_LED_", "SET_TIME", "GET_TIME", "SET_DATE", "GET_DATE", "RSERVD_0", "RSERVD_1", "RSERVD_2", "RSERVD_3", "RSERVD_4"]
        self.RQS_r       = "RS"
        self.RQS_q       = "RQ"
        self.RQS_i       = "RI"
        self.RQS_n       = "RN"
        self.RQS_a       = "RA"
        self.r_flags = [self.RQS_r, self.RQS_q, self.RQS_i, self.RQS_n, self.RQS_a]

        self.active     = False
        self.port       = None
        self.sender     = "" 
        self.receiver   = "" 
        self.rqs        = "" 
        self.topic      = "" 
        self.topic_ix   = ""  #Numeric value (as str) to represent index of topic_list
        self.chapter    = "" 
        self.lenght     = None 
        self.payload    = "" 
        self.hops       = None
        self.crc_rx     = None
        self.crc_val    = None
        self.raw        = None
        self.ready      = 0
        self.required_segments = [self.sender, self.receiver, self.rqs, self.topic, self.chapter, self.payload]

    def _assign_segments(self, port, message_raw):
        #print_v(f"from Messages: {self.segments}")
        self.raw        = message_raw
        self.active     = True
        self.port       = port
        self.sender     = self.segments[2]
        self.receiver   = self.segments[3]
        self.rqs        = self.segments[4]
        self.topic      = self.segments[5]
        self.chapter    = self.segments[6]
        self.lenght     = self.segments[7]
        self.payload    = self.segments[8]
        self.hops       = self.segments[9]
        self.crc_rx     = self.segments[10]


    def __str__(self):
        return f"sender: {self.sender}, receiver: {self.receiver}, rqs: {self.rqs}, topic: {self.topic}, chapter: {self.chapter}, payload: {self.payload}"

    def parse(self, port, message_raw):
        logger.info(f"parse_message:: message: {message_raw}.")
        if (message_raw==None):
            return None
        else:
            self.segments = message_raw.split(':')
            crc_buffer = ':'.join(self.segments[:-2]) + ':'
            self.crc_val = get_crc(crc_buffer)

            if len(self.segments)>12: # If payload contains ':' they will be split apart.
                new_self.segments = []
                new_self.segments = new_self.segments + self.segments[:8]
                new_payload = ':'.join(self.segments[8:-3])
                new_self.segments.append(new_payload)
                self.segments = new_self.segments + self.segments[-3:]
        self._assign_segments(port,  message_raw)


if __name__ == "__main__":
    exit(0)
