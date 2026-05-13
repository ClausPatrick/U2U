#!/usr/bin/python3
#data_processing.py for U2U Version 5

""" ********************** """
""" *** DATA PROCESSING ** """
""" ********************** """

from datetime import datetime
import os
import os.path
from pathlib import Path
import sys
import re
import time
import sqlite3
import logging
import logging_config
import math
import json

logger = logging.getLogger("data_processing")

STATE = "message_log.state"


def clean_segment(segment):
    segment = segment.replace('[', '').replace(']', '')
    segment = segment.replace('(', '').replace(')', '')
    segment = segment.replace('-', '').replace(':', '').replace(' ', '')
    return segment

class Data_Processor:
    def __init__ (self, hostname):
        self.hostname = hostname
        self.get_paths()
        self.db_name = os.path.splitext(os.path.basename(self.db_path))[0]
        if os.path.exists(self.db_path):
            logger.debug("DB:: database exists.")
            self.conn = sqlite3.connect(self.db_path)
            self.db_cursor = self.conn.cursor()
        else:
            logger.debug("DB:: Creating it as it is not found.")
            self.conn = sqlite3.connect(self.db_path)
            self.db_cursor = self.conn.cursor()
            self.create_messages_table()

        self.sender_list = []
        self.receiver_list = []
        self.nr_old_files = 0
        self.log_read_state = STATE
    
    def get_paths(self):
        with open("paths.json", "r") as f:
            paths_file = json.load(f)
        self.message_log  = paths_file["paths"]["message_log"]
        self.log_dir      = paths_file["paths"]["log_dir"]
        self.db_path      = paths_file["paths"]["db_path"]
        self.topology     = paths_file["paths"]["topology"]

""" Messages are appended to file by u2u_node and saving the state based on file's inode ensures that only new files are read and it is not interfered with the write process
and also only completed lines are read. """
    def _load_state(self):
        try:
            with open(self.log_read_state,'r') as f:
                inode, offset, partial = f.read().split('|',2)
                return int(inode), int(offset), partial
        except:
            return None, 0, ''

    def _save_state(self, inode, offset, partial):
        with open(self.log_read_state, 'w') as f:
            f.write(f"{inode}|{offset}|{partial}")

    def _read_complete_lines(self):
        prev_inode, prev_off, prev_partial = self._load_state()
        st = os.stat(self.message_log)
        cur_inode = st.st_ino
        size = st.st_size

        # reset if rotated/truncated
        start = 0 if prev_inode != cur_inode or prev_off > size else prev_off

        with open(self.message_log, 'rb') as f:
            f.seek(start)
            data = f.read()

        if not data and not prev_partial:
            self._save_state(cur_inode, start, '')
            return []

        text = (prev_partial.encode() + data).decode(errors='replace')
        if '\n' in text:
            full, partial = text.rsplit('\n', 1)
            lines = full.split('\n')
            # compute new offset: start + bytes up to end of last newline
            bytes_up_to = (prev_partial.encode() + full.encode()).__len__()
            new_offset = start + bytes_up_to
        else:
            lines = []
            partial = text
            new_offset = start  # don't advance offset; nothing fully consumed

        self._save_state(cur_inode, new_offset, partial)
        return lines


    def process_messages(self): # External entry point 
        lines = self._read_complete_lines()
        line_count = len(lines)
        info_text = f"Parsing and adding {line_count} to database. "
        if (line_count >= 0 and line_count < 100):
            info_text += "This will go quick."
        elif (line_count >= 100 and line_count < 250):
            info_text += "This will take a bit."
        elif (line_count >= 250 and line_count < 500):
            info_text += "This will take a while."
        elif (line_count >= 500 and line_count < 1000):
            info_text += "This will take a long while."
        elif (line_count >= 1000 and line_count < 2000):
            info_text += "This will take long."
            print(info_text)
        elif (line_count >= 2000):
            info_text += "Sit back, be patient, lots of messages to parse."
            print(info_text)

        logger.info(info_text)
        for line in lines:
            if line != '':
                segments = self.parse_message_log_line(line)
                if segments != None:
                    self.write_db(segments)
                    line_count += 1
        logger.info(f"process_messages:: {line_count} line(s) added to {self.db_path}. ")


    def get_int(self, segment):
        try:
            value = int(segment, base=0)
        except ValueError:
            value = -9999
        return value
    """ Entry in message log will look something like this:
    [2026-05-13 21:10:50]/(30281)/(1)/(547767120256)/(14084)/0/1/136/8/SENDER/RECEIVER/RS/GET_SNSR/100/0/91/91/4/10/0/0/23.5, 52.7, 162, 28, 0, 0, 0, 0, 0, 0, 78, 24, 0, 0, 0, 0, 38656, 424,
    """
    def parse_message_log_line(self, line):
        TIME_STAMP_IX           = 0
        PROCESS_ID_IX           = 1
        PARENT_PROCESS_ID_IX    = 2
        THREAD_ID_IX            = 3
        MESSAGE_COUNTER_IX      = 4
        PORT_IX                 = 5
        MESSAGE_INDEX_IX        = 6
        MESSAGE_LENGTH_IX       = 7
        SEGMENT_COUNT_IX        = 8
        SENDER_IX               = 9
        RECEIVER_IX             = 10
        RFLAG_IX                = 11
        TOPIC_IX                = 12
        CHAPTER_IX              = 13
        HOPS_IX                 = 14
        CRC_RECEIVED_IX         = 15
        CRC_CALCULATED_IX       = 16
        TOPIC_NR_IX             = 17
        ROUTING_CONTROL_IX      = 18
        HOPS_INT_IX             = 19
        ERROR_CODE_IX           = 20
        PAYLOAD_IX              = 21
        segment_list = line.rstrip().split('/')
        len_seg = len(segment_list)
        if len_seg < 22:
            m_e = f"parse_message_log_line:: received fewer segments than expected: "
            m_e += f"{len_seg}, segments: {segment_list}"
            logger.warning(m_e)
            segments = None
        else:
            time_stamp = clean_segment(segment_list[TIME_STAMP_IX])
            process_id = clean_segment(segment_list[PROCESS_ID_IX])
            parent_process_id = clean_segment(segment_list[PARENT_PROCESS_ID_IX])
            thread_id = clean_segment(segment_list[THREAD_ID_IX])
            message_counter = clean_segment(segment_list[MESSAGE_COUNTER_IX])
            port = segment_list[PORT_IX]
            message_index = segment_list[MESSAGE_INDEX_IX]
            message_length = segment_list[MESSAGE_LENGTH_IX]
            segment_count = segment_list[SEGMENT_COUNT_IX]
            sender = segment_list[SENDER_IX]
            receiver = segment_list[RECEIVER_IX]
            rflag = segment_list[RFLAG_IX]
            topic = segment_list[TOPIC_IX]
            chapter = segment_list[CHAPTER_IX]
            hops = segment_list[HOPS_IX]
            crc_received   = segment_list[CRC_RECEIVED_IX]
            crc_calculated = segment_list[CRC_CALCULATED_IX]
            topic_nr = segment_list[TOPIC_NR_IX]
            routing_control = segment_list[ROUTING_CONTROL_IX]
            hops_int = segment_list[HOPS_INT_IX]
            error_code = segment_list[ERROR_CODE_IX]
            payload = "".join(segment_list[PAYLOAD_IX:])

            segments = {}
            segments["TIME_STAMP"] = time_stamp
            segments["PROCESS_ID"] = process_id
            segments["PARENT_PROCESS_ID"] = parent_process_id
            segments["THREAD_ID"] = thread_id
            segments["MESSAGE_COUNTER"] = message_counter
            segments["PORT"] = port
            segments["MESSAGE_INDEX"] = message_index
            segments["MESSAGE_LENGTH"] = message_length
            segments["SEGMENT_COUNT"] = segment_count
            segments["SENDER"] = sender
            segments["RECEIVER"] = receiver
            segments["RFLAG"] = rflag
            segments["TOPIC"] = topic
            segments["CHAPTER"] = chapter
            segments["HOPS"] = hops
            segments["CRC_RECEIVED"] = crc_received
            segments["CRC_CALCULATED"] = crc_calculated
            segments["TOPIC_NR"] = topic_nr
            segments["ROUTING_CONTROL"] = routing_control
            segments["HOPS_INT"] = hops_int
            segments["ERROR_CODE"] = error_code
            segments["PAYLOAD"] = payload
        return segments

        #if new_files != None:
        #return len(new_files)

       
    def write_db(self, segments):
        self.db_cursor.execute(f"INSERT INTO received_messages VALUES ( :host_name, :time_stamp, :process_id, :parent_process_id, :thread_id, :message_counter, :port, :message_index, :message_length, :segment_count, :sender, :receiver, :rflag, :topic, :chapter, :hops, :crc_received, :crc_calculated, :topic_nr, :routing_control, :hops_int, :error_code, :payload)", {
            "host_name"         :   self.hostname,
            "time_stamp"        :   segments["TIME_STAMP"] ,
            "process_id"        :   segments["PROCESS_ID"] ,
            "parent_process_id" :   segments["PARENT_PROCESS_ID"] ,
            "thread_id"         :   segments["THREAD_ID"] ,
            "message_counter"   :   segments["MESSAGE_COUNTER"] ,
            "port"              :   segments["PORT"] ,
            "message_index"     :   segments["MESSAGE_INDEX"] ,
            "message_length"    :   segments["MESSAGE_LENGTH"] ,
            "segment_count"     :   segments["SEGMENT_COUNT"] ,
            "sender"            :   segments["SENDER"] ,
            "receiver"          :   segments["RECEIVER"] ,
            "rflag"             :   segments["RFLAG"] ,
            "topic"             :   segments["TOPIC"] ,
            "chapter"           :   segments["CHAPTER"] ,
            "hops"              :   segments["HOPS"] ,
            "crc_received"      :   segments["CRC_RECEIVED"] ,
            "crc_calculated"    :   segments["CRC_CALCULATED"] ,
            "topic_nr"          :   segments["TOPIC_NR"] ,
            "routing_control"   :   segments["ROUTING_CONTROL"] ,
            "hops_int"          :   segments["HOPS_INT"] ,
            "error_code"        :   segments["ERROR_CODE"] ,
            "payload"           :   segments["PAYLOAD"]})
    
        self.conn.commit()


    def get_senders(self):
        self.db_cursor.execute("SELECT DISTINCT sender FROM received_messages")
        self.sender_list = self.db_cursor.fetchall()
        senders = []
        for s in self.sender_list:
            senders.append(str(s)[2:-3])
        return senders

    def get_receivers(self):
        self.db_cursor.execute("SELECT DISTINCT receiver FROM received_messages")
        self.receiver_list = self.db_cursor.fetchall()
        receivers = []
        for s in self.receiver_list:
            receivers.append(str(s)[2:-3])
        return receivers

    def query(self, pattern):
        pattern_list = pattern.split(' ')
        if pattern_list[0].upper() != "SELECT":
            logger.warning(f"Pattern with no 'SELECT' keyword are not supported: '{pattern}'.")
            ret_val =  None
        elif "DROP" in pattern_list or "drop" in pattern_list:
            logger.warning(f"Pattern does not allow for 'DROP' keyword: '{pattern}'.")
            ret_val =  None
        else:
            self.db_cursor.execute(pattern)
            ret_val = self.db_cursor.fetchall()
        return ret_val

    def create_messages_table(self):
        logger.debug("create_messages_table called.")
        try:
            self.db_cursor.execute("""CREATE TABLE received_messages (
                "host_name"         text,  
                "time_stamp"        text,  
                "process_id"        text, 
                "parent_process_id" text, 
                "thread_id"         text, 
                "message_counter"   text, 
                "port"              text, 
                "message_index"     text, 
                "message_length"    text, 
                "segment_count"     text, 
                "sender"            text, 
                "receiver"          text, 
                "rflag"             text, 
                "topic"             text, 
                "chapter"           text, 
                "hops"              text, 
                "crc_received"      text, 
                "crc_calculated"    text, 
                "topic_nr"          text, 
                "routing_control"   text, 
                "hops_int"          text, 
                "error_code"        text,
                "payload"           text   
                ) """)
            self.conn.commit()
            logger.debug("create_messages_table:: Table 'received_messages' created.")
        except sqlite3.OperationalError:
            logger.warning("create_messages_table:: Table 'received_messages' already exists.")


    def save_topology(self):
        with open(f"/home/pi/c_taal/u2u/tui/u2uctl/topology_{self.hostname}.txt", "a") as f:
            f.write("Senders:\n")
            self.get_senders()
            self.get_receivers()
            for i, s in enumerate(self.sender_list):
                f.write(f"({i}: {s}\n")
            f.write("Receivers:\n")
            for i, r in enumerate(self.receiver_list):
                f.write(f"({i}: {r}\n")

            for s in self.sender_list:
                self.db_cursor.execute(f"SELECT MIN(hops), port FROM received_messages WHERE sender='{s[0]}'")
                res = self.db_cursor.fetchall()
                hop_s = res[0][0]
                port_s = res[0][1]
                out_string = f"Node '{s[0]}'\t is {int(hop_s)} hops away, on port {port_s}.\n"
                f.write(out_string)

    def close_db(self):
        logger.debug("close_db called.")
        self.conn.close()
        return


if __name__ == "__main__":
    exit(0)

