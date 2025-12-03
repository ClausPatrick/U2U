#!/usr/bin/python3
#control_centre.py

""" ********************** """
""" ***     u2uctl     *** """
""" ********************** """

from datetime import datetime
import os
import os.path
import sys
import re
import subprocess
import numpy as np
import time
import sqlite3
import RPi.GPIO as gpio
import logging
import logging_config
import threading
from copy import deepcopy
from prompt_toolkit import prompt
from prompt_toolkit.completion import WordCompleter
from prompt_toolkit.keys import Keys
from prompt_toolkit.key_binding import KeyBindings
from prompt_toolkit import Application
from prompt_toolkit import PromptSession
from prompt_toolkit.layout.containers import VSplit, HSplit
from prompt_toolkit.layout.layout import Layout
from prompt_toolkit.widgets import Box, Label, TextArea
from prompt_toolkit import print_formatted_text, HTML
from prompt_toolkit.history import InMemoryHistory
#from prompt_toolkit.auto_suggest import AutoSuggestFromHistory
import pickle
import json

from u2u_messaging import Message
from data_processing import Data_Processor
from target_service import Target

MAX_MESSAGE_PAYLOAD_SIZE  = 255

logger = logging.getLogger("u2uctl")

LOG_DIR = "/home/pi/c_taal/u2u/hal/linux/logs"

LEDs = [5, 6, 13, 19, 26, 12, 16, 20, 21]

title_text = """U2U command centre """
help_text = """
'exit': quit this program.
'clear': Clear input history.
'exit': quit this program.
"""


def get_timestamp():
    now = datetime.now()
    return now.strftime('%Y/%m/%d %H:%M:%S')

def get_timestamp_int():
    now = datetime.now()
    return int(now.strftime('%Y%m%d%H%M%S'))
    

"""Date format from db looks like this: '('Tue Nov 18 23:14:25 2025',)' to '20251118231425'"""
def get_timestamp_db_to_int(db_date_str):
    month_to_day = {"Jan": "01", "Feb": "02", "Mar": "03", "Apr": "04", "May": "05", "Jun": "06", "Jul": "07", "Aug": "08", "Sep": "09", "Oct": "10", "Nov": "11", "Dec": "12"}
    timestamp = get_timestamp()
    db_date_str = db_date_str.replace("'", "").replace("(", "").replace(")", "").replace(",", "")
    date_parts = db_date_str.split()
    month = month_to_day[date_parts[1]]
    day = ("00" + date_parts[2])[-2:]
    time = date_parts[3].replace(":", "")
    year = date_parts[4]
    return int(year+month+day+time)

def readable_timestamp(int_time_stamp):
    ts = str(int_time_stamp)
    return f"{ts[0:4]}-{ts[4:6]}-{ts[6:8]} {ts[8:10]}:{ts[10:12]}:{ts[12:14]}"

def io_setup():
    gpio.setmode(gpio.BCM)
    for L in LEDs:
        gpio.setup(L, gpio.OUT)
        gpio.output(L, 0)

def get_selfname():
    p = subprocess.Popen(["hostname"], stdout=subprocess.PIPE, shell=True)
    (outp, err) = p.communicate()
    name = str(outp, encoding="utf-8")
    return name.strip()

def get_selfip():
    p = subprocess.Popen(["hostname -I"], stdout=subprocess.PIPE, shell=True)
    (outp, err) = p.communicate()
    name = str(outp, encoding="utf-8")
    return name.strip()

def print_list(items):
    for i in items:
        print(i[0])
    print()

def read_file(file_name):
    try:
        with open(file_name, 'r', errors='ignore') as f:
            lines = f.readlines()
        return lines
    except Exception as e:
        print(f"File read error {file_name}: {e}")
        return None

UPDATE_DAEMON = False

class u2u_repl:
    def __init__(self, target):
        self.target = target
        self.hostname = get_selfname()
        self.hostip = get_selfip().split(' ')[0]
        self.state_inactive = f"<ansired>inactive</ansired>"
        self.state_active = f"<ansigreen>  active</ansigreen>"
        self.states = [self.state_inactive, self.state_active]
        self.state_prompt = self.states[0]
        if self.target.get_target_status() == 1:
            self.state_prompt = self.states[1]
        self.quitting = False
        self.message = Message() 
        self.new_message_counter = 0
        self.node_list = []
        self.sent_messages = []
        self.last_response_time = 0
        self.session = PromptSession()
        if UPDATE_DAEMON == True:
            self.quitting_thread = threading.Event()
            self.thread_checker = threading.Thread(target=self.update_db_data_daemon, daemon=True)
            self.thread_checker.start()
        else:
            self.db = Data_Processor(self.hostname)
            self.update_db_data()
        try:
            with open("message_store.bin", "rb") as f:
                self.stored_messages = pickle.load(f)
        except FileNotFoundError as e:
            self.stored_messages = [] 

    def compose_host_lists(self):
        self.receiver_list = list(set(self.db.get_receivers()))
        self.sender_list = list(set(self.db.get_senders()))
        self.node_list = self.receiver_list + self.sender_list
    
    def update_db_data(self): # Called periodically outside of key press events.
        new_messages = self.db.process_messages()
        if (new_messages > 0 ):
            self.new_message_counter += new_messages
            logger.info(f"update_db_data:: new messages: {new_messages}.")
        self.compose_host_lists()


    def update_db_data_daemon(self): # Called periodically outside of key press events.
        self.db = Data_Processor(self.hostname)
        while not self.quitting_thread.is_set():
            time.sleep(2)
            if (self.target.get_target_status() == 0):
                break
            self.node_list = list(set(self.db.get_receivers() + self.db.get_senders()))
            new_messages = self.db.process_messages()
            if (new_messages > 0 ):
                self.new_message_counter += new_messages
                logger.info(f"update_db_data_daemon:: new messages: {new_messages}.")

        self.db.close_db()
        return 

    def bottom_toolbar(self):
        return HTML(f"New messages: {self.new_message_counter}")


    def main_menu(self):
        completer = WordCompleter(['list', 'enable', 'disable', 'messages', 'options', 'help', 'exit'])
        option = self.session.prompt(HTML(f"U2U {self.state_prompt} cc# "), completer=completer)
        option = option.strip().replace(" ", "")
        logger.debug(f'u2u_repl::main_menu: {option}.')
        return option

    def cmdloop(self):
        while True:
            option = self.main_menu()
            #option = option.strip().replace(" ", "")
            if option == "exit":
                logger.debug(f'u2u_repl::cmdloop: exit.')
                self.do_exit()
                break
            elif option == "help":
                self.do_help()
            elif option == "list":
                self.do_list()
            elif option == "listhosts":
                self.do_listhosts()
            elif option == "enable":
                self.do_enable()
            elif option == "enableall":
                self.enable_local()
                self.enable_remote()
            elif option == "disable":
                self.do_disable()
            elif option == "disableall":
                self.disable_local()
                self.disable_remote()
            elif option == "enablelocal":
                self.enable_local()
            elif option == "disablelocal":
                self.disable_local()
            elif option == "enableremote":
                self.enable_remote()
            elif option == "disableremote":
                self.disable_remote()
            elif option == "messages":
                self.do_messages()
            elif option == "composemessage":
                self.do_compose()
            elif option == "options":
                self.do_options()
        return

    def do_exit(self):
        self.quitting = True
        if UPDATE_DAEMON == True:
            self.quitting_thread.set()
            self.thread_checker.join()
        return

    def do_messages(self):
        self.update_db_data()
        wc = ["compose", "load", "responses", "templates", "query", "exit"]
        completer = WordCompleter(wc)
        while True:
            option = self.session.prompt(HTML(f"U2U {self.state_prompt} messages# "), completer=completer)
            option = option.strip().replace(" ", "")
            if option == "exit":
                break
            elif option == "compose":
                logger.debug(f'u2u_repl::do_messages: option(compose).')
                self.do_compose()
            elif option == "query":
                self.do_query()
            elif option == "load":
                self.do_load()
            elif option == "responses":
                self.do_check_responses()
            elif option == "send":
                self.do_send()
            elif option == "templates":
                self.do_templates()
        return

    def do_templates(self):
        wc = ["hail", "sensor", "timing", "exit"]
        completer = WordCompleter(wc)
        while True:
            option = self.session.prompt(HTML(f"U2U {self.state_prompt} select template:# "), completer=completer)
            option = option.strip().replace(" ", "")
            if option == "exit":
                break
            elif option == "hail":
                r = self.target.send_from_template(0)
                logger.debug("do_template: option 0 returned '{r}' from target.")
                break
            elif option == "sensor":
                r = self.target.send_from_template(1)
                logger.debug("do_template: option 1 returned '{r}' from target.")
                break
            elif option == "timing":
                r = self.target.send_from_template(2)
                logger.debug("do_template: option 2 returned '{r}' from target.")
                break
        return



    def do_query(self):
        while True:
            option = self.session.prompt(HTML(f"U2U {self.state_prompt} query database# "))
            option = option.strip()
            if len(option) and option != "exit":
                print(f"querying: {option}")
                print(self.db.query(option))
            if option == "exit":
                break
        return


    def do_compose(self):
        self.message.sender = (self.hostname).upper()
        logger.debug(f'u2u_repl::do_compose.')
        completer = WordCompleter(["receiver", "r_flag", "topic", "chapter", "payload", "send", "store", "load"])
        while True:
            option = self.session.prompt(HTML(f"U2U format segment:# "), completer=completer)
            option = option.strip().replace(" ", "")
            if option == "receiver":
                self.do_compose_receiver()
            elif option == "r_flag" or option == "rflag":
                self.do_compose_rflag()
            elif option == "topic":
                self.do_compose_topic()
            elif option == "chapter":
                self.do_compose_chapter()
            elif option == "payload":
                self.do_compose_payload()
            elif option == "port":
                self.do_compose_port()
            elif option == "send":
                r = self.do_send()
                if r==0:
                    break
            elif option == "store":
                self.do_store()
            elif option == "load":
                self.do_load()
            elif option == "exit":
                break
        return

    def _check_message_complete(self):
        ret_val = 0
        if self.message.sender == "":
            ret_val = 1
            print(f"ERROR: message incomplete: segment sender is not set.")
        if self.message.receiver == "":
            ret_val = 3
            print(f"ERROR: message incomplete: segment receiver is not set.")
        if self.message.rqs == "":
            ret_val = 7
            print(f"ERROR: message incomplete: segment r_flag is not set.")
        if self.message.topic == "":
            ret_val = 15
            print(f"ERROR: message incomplete: segment topic is not set.")
        if self.message.chapter == "":
            ret_val = 31
            print(f"ERROR: message incomplete: segment chapter is not set.")
        if self.message.payload == "":
            ret_val = 63
            print(f"ERROR: message incomplete: segment payload is not set.")
        if self.message.receiver == self.message.sender:
            print(f"WARNING: message sender and receiver are the same: {self.message.receiver}.")
            self.do_compose_receiver()
        return ret_val

    def do_send(self):
        ret_val = self._check_message_complete()
        if ret_val == 0:
            self.do_compose_port()
            if self.target.get_target_status() == 1:
                r = target.send_message(self.message, self.message.port)
                self.sent_messages.append((get_timestamp_int(), deepcopy(self.message)))
                print(f"Sending message from port {self.message.port}: {self.message}, result: {r}.")
            else:
                print(f"Unable to send: target is not active. Enable first:")
                self.do_enable()
        return ret_val

    def do_check_responses(self):
        self.update_db_data()
        if len(self.sent_messages) ==  0:
            print("WARNING: no messages have been sent.")
        else:
            if self.last_response_time == 0:
                self.last_response_time = self.sent_messages[0][0]
                print(f"was 0, now is: {readable_timestamp(self.sent_messages[0][0])} {self.sent_messages[0][1]}.")

            query = f"SELECT * FROM central_comms WHERE date > '{self.last_response_time}' AND receiver = '{self.hostname.upper()}' AND r_flag = 'RS'"
            response = self.db.query(query)
            #print(f"query: '{query}'")
            if len(response) != 0:
                self.last_response_time = get_timestamp_int()
            print(f"t: {readable_timestamp(self.last_response_time)}, response: \n{response}")
        return
            

    def do_store(self):
        message_count = len(self.stored_messages)
        self.stored_messages.append(deepcopy(self.message))
        for i, m in enumerate(self.stored_messages):
            print(f"({i}) {m}")
        with open("message_store.bin", "wb") as f:
            pickle.dump(self.stored_messages, f)
        return

    def do_load(self):
        if len(self.stored_messages) == 0:
            print("WARNING: no messages stored.")
        else:
            options = [str(i) for i in range(len(self.stored_messages))]
            completer = WordCompleter(options)
            for i, m in enumerate(self.stored_messages):
                print(f"({i}) {m}")
            while True:
                option = self.session.prompt(HTML(f"U2U load message ({options[0]}-{options[-1]})# "), completer=completer)
                if options == "exit":
                    break
                elif option in options:
                    self.message = deepcopy(self.stored_messages[int(option)])
                    break
        return


    def do_compose_receiver(self):
        logger.debug(f'u2u_repl::do_compose_receiver.')
        #options = list(set(self.db.get_receivers() + self.db.get_senders()))
        options = self.node_list
        completer = WordCompleter(options)
        item = ""
        if self.message.receiver != "":
            item = f"({self.message.receiver})"
        while True:
            option = self.session.prompt(HTML(f"U2U receiver {item}:# "), completer=completer)
            option = option.strip().replace(" ", "")
            if option == "exit":
                break
            elif option in options:
                self.message.receiver = option.upper()
                print(self.message)
                break
            else:
                self.message.receiver = option
                print(f"Warning: Receiver '{option}' does not appear in database and might not respond.")
                print(self.message)
                break
        return

    def do_compose_port(self):
        logger.debug(f'u2u_repl::do_compose_port.')
        options = ['0', '1', '0 & 1']
        completer = WordCompleter(options)
        item = ""
        if self.message.port != "":
            item = f"({self.message.port})"
        while True:
            option = self.session.prompt(HTML(f"U2U port {item}:# "), completer=completer)
            option = option.strip().replace(" ", "")
            if option == "exit":
                break
            elif option in options:
                self.message.port = option
                break
            elif option == "0&1" or option == "0,1" or option == "2":
                option = 2
                self.message.port = option
                break
            else:
                print(f"Not an port option: {option}.")
        return

    def do_compose_rflag(self):
        logger.debug(f'u2u_repl::do_compose_rflag.')
        options = self.message.r_flags
        completer = WordCompleter(options)
        item = ""
        if self.message.rqs != "":
            item = f"({self.message.rqs})"
        while True:
            option = self.session.prompt(HTML(f"U2U rqs {item}:# "), completer=completer)
            option = option.strip().replace(" ", "")
            if option == "exit":
                break
            elif option in options:
                self.message.rqs = option
                print(self.message)
                break
            else:
                print(f"Not an rqs option: {option}.")
        return

    def do_compose_topic(self):
        logger.debug(f'u2u_repl::do_compose_topic.')
        options = self.message.topic_list
        completer = WordCompleter(options)
        item = ""
        if self.message.topic != "":
            item = f"({self.message.topic})"
        while True:
            option = self.session.prompt(HTML(f"U2U topic {item}:# "), completer=completer)
            option = option.strip().replace(" ", "")
            if option == "exit":
                break
            elif option in options:
                self.message.topic_ix = str(self.message.topic_list.index(option))
                self.message.topic = option
                print(self.message)
                break
            else:
                print(f"Not an topic option: {option}. Current version does not support custom topics.")
        return


    def _validate_input(self, value, max, min=0):
        try:
            int_value = int(value)
            if not (min <= int_value < max):
                print(f"ERROR: value must be between {min} and {max}: {value}")
                return 1
        except ValueError as e:
            print(f"ERROR: value must be numeric and between {min} and {max}: {value}")
            return 2
        return 0

            
    def do_compose_chapter(self):
        logger.debug(f'u2u_repl::do_compose_chapter.')
        item = ""
        if self.message.chapter != "":
            item = f"({self.message.chapter})"
        while True:
            option = self.session.prompt(HTML(f"U2U chapter {item}:# "))
            option = option.strip().replace(" ", "")
            if option == "exit":
                break
            else:
                if self._validate_input(option, 999) == 0:
                    self.message.chapter = option
                    print(self.message)
                    break
        return

    def do_compose_payload(self):
        logger.debug(f'u2u_repl::do_compose_payload.')
        item = ""
        if self.message.payload != "":
            item = f"({self.message.payload})"
        while True:
            option = self.session.prompt(HTML(f"U2U payload {item}:# "))
            option = option.strip()
            if option == "exit":
                break
            else:
                if len(option) <MAX_MESSAGE_PAYLOAD_SIZE:
                    self.message.payload = option
                    print(self.message)
                    break
                else:
                    self.message.payload = option[:MAX_MESSAGE_PAYLOAD_SIZE]
                    print(self.message)
                    break
        return


    def do_list(self):
        completer = WordCompleter(['hosts', 'receivers', 'senders', 'exit'])
        self.update_db_data()
        while True:
            option = self.session.prompt(HTML(f"U2U {self.state_prompt} list# "), completer=completer)
            option = option.strip().replace(" ", "")
            if option == "exit":
                break
            elif option == "receivers":
                print(f"{self.receiver_list}")
            elif option == "senders":
                print(f"{self.sender_list}")
            elif option == "hosts":
                self.do_listhosts()
        return

    def enable_local(self):
        logger.debug(f'u2u_repl::enable_local.')
        self.target.enable_local()
        self.state_prompt = self.states[1]
        if UPDATE_DAEMON == True:
            self.thread_checker = threading.Thread(target=self.update_db_data_daemon, daemon=True)
            self.thread_checker.start()
        return


    def disable_local(self):
        logger.debug(f'u2u_repl::disable_local.')
        self.target.disable_local()
        self.state_prompt = self.states[0]
        self.quitting = True
        if UPDATE_DAEMON == True:
            self.quitting_thread.set()
        return

    def do_listhosts(self):
        act_colours = [f"[<ansired>x</ansired>]", f"[<ansigreen>o</ansigreen>]"]
        print_formatted_text(HTML(f"{act_colours[self.target.get_target_status()]}\t{self.hostname}\t{self.hostip}\t*self"))
        for hostname, ip in zip(self.target.remote_hostsnames, self.target.remote_hosts):
            print_formatted_text(HTML(f"{act_colours[self.target.get_target_status_remote(ip)]}\t{hostname}\t{ip.split('@')[1]}"))
        return


    def enable_remote(self):
        logger.debug(f'u2u_repl::enable_remote.')
        r = self.target.enable_remote()
        print(r)
        return

    def disable_remote(self):
        logger.debug(f'u2u_repl::disable_remote.')
        r = self.target.disable_remote()
        print(r)
        return

    def do_enable(self):
        completer = WordCompleter(['local', 'remote', 'exit'])
        while True:
            option = self.session.prompt(HTML("U2U {self.state_prompt} enable# "), completer=completer)
            option = option.strip().replace(" ", "")
            if option == "exit":
                break
            elif option == "local":
                self.enable_local()
                print(f"local")
            elif option == "remote":
                self.disable_local()
                print(f"remote")
        return

    def do_disable(self):
        completer = WordCompleter(['local', 'remote', 'exit'])
        while True:
            option = self.session.prompt(HTML("U2U {self.state_prompt} disable# "), completer=completer)
            option = option.strip()
            if option == "exit":
                break
            elif option == "local":
                self.enable_remote()
                print(f"local")
            elif option == "remote":
                self.disable_remote()
                print(f"remote")
        return

    def do_options(self):
        completer = WordCompleter(['cleanup', 'option1', 'option2'])
        while True:
            option = self.session.prompt(HTML("U2U options# "), completer=completer)
            option = option.strip()
            if option == "exit":
                break
            if option == "cleanup":
                target.pipe_cleaner()
                break
        return


    def do_help(self):
        print(f"help: ")
        return


if __name__ == "__main__":
    selfname = get_selfname()
    selfip = get_selfip()
    logger.info(f'Starting control centre at {selfname}.')
    cli_args = sys.argv #.replace(" ", "")
    target = Target(selfname)
    target.set_hosts("host_data.json")
    u2u_repl(target).cmdloop()
    exit(0)
