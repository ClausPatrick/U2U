#!/usr/bin/python3
#data_processing.py

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
import pickle
import json

logger = logging.getLogger("data_processing")

def read_file(file_name):
    try:
        with open(file_name, 'r', errors='ignore') as f:
            lines = f.readlines()
        return lines
    except Exception as e:
        print(f"File read error {file_name}: {e}")
        return None


"""Date format from db looks like this: '('Tue Nov 18 23:14:25 2025',)'"""
def date_formatter(db_date_str):
    month_to_day = {"Jan": "01", "Feb": "02", "Mar": "03", "Apr": "04", "May": "05", "Jun": "06", "Jul": "07", "Aug": "08", "Sep": "09", "Oct": "10", "Nov": "11", "Dec": "12"}
    db_date_str = db_date_str.replace("'", "").replace("(", "").replace(")", "").replace(",", "")
    date_parts = db_date_str.split()
    month = month_to_day[date_parts[1]]
    day = ("00" + date_parts[2])[-2:]
    time = date_parts[3].replace(":", "")
    year = date_parts[4]
    #print(f"date_formatter:: in:'{db_date_str}', out:'{year + month + day + time}'.")
    return year + month + day + time


class Data_Processor:
    def __init__ (self, hostname):
        self.hostname = hostname
        self.get_paths()
        if os.path.exists(self.db_path):
            logger.debug("DB:: database exists.")
            self.conn = sqlite3.connect(self.db_path)
            self.db_cursor = self.conn.cursor()
        else:
            logger.debug("DB:: Creating it as it is not found.")
            self.conn = sqlite3.connect(self.db_path)
            self.db_cursor = self.conn.cursor()
            self.create_db()

        self.sender_list = []
        self.receiver_list = []
        self.nr_old_files = 0
#        try:
#            with open("comm_file_tracker.bin", "rb") as f:
#                self.file_tracker = pickle.load(f)
#        except FileNotFoundError as e:
#            self.file_tracker = [[],[]]
#
    
    def get_paths(self):
        with open("paths.json", "r") as f:
            paths_file = json.load(f)
        self.log_dir  = paths_file["paths"]["log_dir"]
        self.db_path  = paths_file["paths"]["db_path"]
        self.topology = paths_file["paths"]["topology"]

            
    def get_local_log_names(self):
        paths = sorted(Path(self.log_dir).iterdir(), key=os.path.getctime)
        log_files = []
        for f in paths:
            if f.name[-4:]==".log" and "comm" in f.name:
                logger.info(f"    {f}")
                log_files.append(f.name)
        return log_files

    def check_new_messages(self):
        log_files_names = self.get_local_log_names()
        nr_stale_messages = self.nr_old_files
        self.nr_old_files = len(log_files_names)
        #nr_new_messages = self.nr_old_files - nr_stale_messages 
        nr_new_messages = len(log_files_names) - nr_stale_messages 
        logger.debug(f"check_new_messages:: nr old files: {nr_stale_messages}, nr new files: {nr_new_messages}, nr logs: {len(log_files_names)}.")
        if (nr_new_messages > 0):
            new_files = log_files_names[-(nr_new_messages):]
            for f in new_files:
                logger.info(f"    '{f}'")
            return new_files
        else:
            return []

    def process_messages(self): # External entry point 
        new_files = self.check_new_messages()
        if new_files != None:
            self.write_db(new_files)
        return len(new_files)

    def parse_message(self, message):
        segments = message.split(':')
        if len(segments)>12: # If payload contains ':' they will be split apart.
            new_segments = []
            new_segments = new_segments + segments[:8]
            new_payload = ':'.join(segments[8:-2])
            new_segments.append(new_payload)
            segments = new_segments + segments[-2:]
        return segments[2:]
        

    def retro_date_formatter(self):
        self.db_cursor.execute(f"SELECT rowid, date FROM central_comms")
        rows = self.db_cursor.fetchall()
        for row in rows:
            row_nr, old_date = row
            try:
                new_date = date_formatter(old_date)
                self.db_cursor.execute(f"UPDATE central_comms SET date = ? WHERE rowid = ?", (new_date, row_nr))

            except Exception as e:
                e_m = f"Error updating 'date' in databse: {old_date}."
                print(e_m)

        
        self.conn.commit()
        self.db_cursor.execute(f"SELECT rowid, date FROM central_comms")
        rows = self.db_cursor.fetchall()


    def write_db(self, file_names):
        logger.debug("write_db called")
        for i, f in enumerate(file_names):
            fn = self.log_dir + '/' + f
            (pid, pdr) = (f[:-4].split('_'))[1:]
            date_created_raw = os.path.getctime(fn)
            date = date_formatter(time.ctime(date_created_raw))
            logger.debug(f"write_db:: date: {date}, type: {type(date)}.")
            new_line = read_file(fn)
            packet = new_line[0]   # File typically only contains 1 line.
            ix_mes_begin = packet.index("#_COMM_#")
            ix_mes_end = packet.index("#_COMM_END_#")
            meta = packet[:ix_mes_begin]
            port_ix = meta.index("PORT ") + len("PORT ")
            port = meta[port_ix]
            in_out = ("OUTBOUND" in meta) * 1
            message = packet[(ix_mes_begin+len("#_COMM_#")):ix_mes_end]
            segments = self.parse_message(message)
            self.db_cursor.execute(f"INSERT INTO central_comms VALUES (:hostname, :date, :port, :in_out, :process_id, :function_type, :raw, :sender, :receiver, :r_flag, :topic, :chapter, :length, :payload, :hops, :crc)", {
                        'hostname':      self.hostname, 
                        'date':          date, 
                        'port':          port,
                        'in_out':        in_out,
                        'process_id':    pid, 
                        'function_type': pdr, 
                        'raw':           message,
                        'sender':        segments[0],
                        'receiver':      segments[1],
                        'r_flag':        segments[2],
                        'topic':         segments[3],
                        'chapter':       segments[4],
                        'length':        segments[5],
                        'payload':       segments[6],
                        'hops':          segments[7],
                        'crc':           segments[8]})
            self.conn.commit()
            self.db_cursor.execute(f"INSERT INTO file_table VALUES (:filename, :date, :pid)", {
                        'filename':     f,
                        'date':         date,
                        'pid':          pid})
            self.conn.commit()
        
        self.db_cursor.execute("SELECT DISTINCT sender FROM central_comms")
        self.sender_list = self.db_cursor.fetchall()
        self.db_cursor.execute("SELECT DISTINCT receiver FROM central_comms")
        self.receiver_list = self.db_cursor.fetchall()
        logger.debug("write_db: done.")
        return

    def get_senders(self):
        self.db_cursor.execute("SELECT DISTINCT sender FROM central_comms")
        self.sender_list = self.db_cursor.fetchall()
        senders = []
        for s in self.sender_list:
            senders.append(str(s)[2:-3])
        logger.debug(f"get_senders called: {senders}.")
        return senders

    def get_receivers(self):
        self.db_cursor.execute("SELECT DISTINCT receiver FROM central_comms")
        self.receiver_list = self.db_cursor.fetchall()
        receivers = []
        for s in self.receiver_list:
            receivers.append(str(s)[2:-3])
        logger.debug(f"get_receivers called: {receivers}.")
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


    def create_db(self):
        logger.debug("create_db called.")
        try:
            self.db_cursor.execute("""CREATE TABLE central_comms (
                    hostname            text,
                    date                text,
                    port                text,
                    in_out              text,
                    process_id          text,
                    function_type       text,
                    raw                 text,
                    sender              text,
                    receiver            text,
                    r_flag              text,
                    topic               text,
                    chapter             text,
                    length              text,
                    payload             text,
                    hops                text,
                    crc                 text
                    ) """)
            self.conn.commit()
        except sqlite3.OperationalError:
            logger.warning("Table 'central_comms' already exists.")

        try:
            self.db_cursor.execute("""CREATE TABLE file_table (
                    filename            text, 
                    date                text, 
                    pid                 text
                    ) """)
            self.conn.commit()
        except sqlite3.OperationalError:
            logger.warning("Table 'file_table' already exists.")

        file_names = self.get_local_log_names()
        self.write_db(file_names)


    def safe_data(self):
        with open(f"/home/pi/c_taal/u2u/tui/topology_{self.hostname}.txt", "a") as f:
            f.write("Senders:\n")
            for i, s in enumerate(self.sender_list):
                f.write(f"({i}: {s}\n")
            f.write("Receivers:\n")
            for i, r in enumerate(self.receiver_list):
                f.write(f"({i}: {r}\n")

            for s in self.sender_list:
                self.db_cursor.execute(f"SELECT MIN(hops), port, in_out FROM central_comms WHERE sender='{s[0]}'")
                res = self.db_cursor.fetchall()
                hop_s = res[0][0]
                port_s = res[0][1]
                in_out_s = res[0][2]
                io_s = "INBOUND"
                if in_out_s=='1':
                    io_s = "OUTBOUND"
        
                out_string = f"Sender: '{s[0]}'\t is {int(hop_s)} hops away, on port {port_s}, {io_s}.\n"
                f.write(out_string)

    def close_db(self):
        logger.debug("close_db called.")
        self.conn.close()
        return


if __name__ == "__main__":
    exit(0)

