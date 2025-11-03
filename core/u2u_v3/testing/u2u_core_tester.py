#!/usr/bin/python3

import re
import subprocess
import path_file
import os
from os.path import exists
import sys
import logging



logger = logging.getLogger(__name__)
main_name, _ = os.path.splitext(sys.argv[0])
logging.basicConfig(filename=f'{main_name}.log', encoding='utf-8', level=logging.DEBUG, format='%(asctime)s: %(levelname)s %(message)s')

''' Assuming at this point that logs/uartN_tester_log.txt is empty and can be overwritten '''
MAX_MESSAGE_PAYLOAD_SIZE = 255
sg_WAIT_INDEX            = 0;
sg_PREMESSAGE_INDEX      = 0;
sg_SENDER_INDEX          = 1;
sg_RECEIVER_INDEX        = 2;
sg_RQS_INDEX             = 3;
sg_TOPIC_INDEX           = 4;
sg_CHAPTER_INDEX         = 5;
sg_LENGTH_INDEX          = 6;
sg_PAYLOAD_INDEX         = 7;
sg_HOPCOUNT_INDEX        = 8;
sg_CRC_INDEX             = 9;

option_dict = {"verbose_option" : False}

u2u_platform_channel = 1
DUT_NAME = "TEST_RCVR"

pattern_indices = {"PORT" : 0, "SENDER" : 1, "RECEIVER" : 2, "R-FLAG" : 3, "TOPIC" : 4, "CHAPTER" : 5, "PAYLOAD" : 6, "HOPS" : 7, "CRC" : 8}

__test_patterns =  [[0, 0, 0, 0, 0, 0, 0]]

_test_patterns = [
        [0, 0, 0, 0, 0, 0, 0], 

        [1, 0, 1, 0, 1, 1, 1], 
        [0, 3, 3, 1, 6, 16, 16],
        [1, 3, 2, 1, 7, 17, 17],
        [0, 4, 4, 1, 8, 18, 18],
        [0, 1, 0, 0, 2, 2, 2],
        [1, 1, 1, 0, 3, 3, 3]]

'''test_patterns legend     
    0: PORT, 
    1: SENDER 
    2: RECEIVER
    3: R-FLAG
    4: TOPIC
    5: CHAPTER
    6: PAYLOAD
'''



test_patterns = [
        [0, 0, 0, 0, 0, 0, 0], 
        [1, 0, 1, 0, 1, 1, 1], 
        [0, 1, 0, 0, 2, 2, 2],
        [1, 1, 1, 0, 3, 3, 3],
        [0, 2, 2, 0, 4, 4, 4],
        [1, 2, 1, 0, 5, 5, 5],
        [0, 3, 3, 0, 6, 6, 6],
        [1, 3, 2, 0, 7, 7, 7],
        [0, 4, 4, 0, 8, 8, 8],
        [1, 4, 3, 1, 9, 9, 9],
        [0, 0, 0, 1, 0, 10, 10], 
        [1, 0, 1, 1, 1, 11, 11], 
        [0, 1, 0, 1, 2, 12, 12],
        [1, 1, 1, 1, 3, 13, 13],
        [0, 2, 2, 1, 4, 14, 14],
        [1, 2, 1, 1, 5, 15, 15],
        [0, 3, 3, 1, 6, 16, 16],
        [1, 3, 2, 1, 7, 17, 17],
        [0, 4, 4, 1, 8, 18, 18],
        [1, 4, 3, 1, 9, 19, 19],

        [0, 0, 0, 2, 0, 20, 20], 
        [1, 0, 1, 2, 1, 21, 21], 
        [0, 1, 0, 2, 2, 22, 22],
        [1, 1, 1, 2, 3, 23, 23],
        [0, 2, 2, 2, 4, 24, 24],
        [1, 2, 1, 2, 5, 25, 25],
        [0, 0, 0, 3, 6, 26, 26], 
        [1, 0, 1, 3, 7, 27, 27], 
        [0, 1, 0, 3, 8, 28, 28],
        [1, 1, 1, 3, 9, 29, 29],
        [0, 2, 2, 3, 10, 30, 30],
        [1, 2, 1, 3, 11, 31, 31]]


def print_v(txt):
    logger.debug(txt)
    if option_dict["verbose_option"]:
        print(f"# {txt}")

class Message():
    def __init__(self):
        self.active     = False
        self.port       = None
        self.sender     = None
        self.receiver   = None
        self.rqs        = None
        self.topic      = None
        self.chapter    = None
        self.lenght     = None
        self.payload    = None
        self.hops       = None
        self.crc_rx     = None
        self.crc_val    = None
        self.raw        = None

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
                #print_v(f"new segment: {new_self.segments}")
            #print_v(len(self.self.segments))
            #print_v(self.self.segments)
            #result = self.check_self.segments()
            #print_v(f"Message: {message}")
            #print_v(f"Segments: {self.segments}")
        self._assign_segments(port,  message_raw)

time_out = 1

def run_u2u_executable(exe_path, args):
    if os.path.exists(exe_path):
        try:
            working_dir = os.path.dirname(exe_path)
            #cmd = ["strace", "-f", "-o", "strace_output.txt"] + [exe_path] + args
            cmd = [exe_path] + args # Correction of path when testing script is located elsewhere.
            process = subprocess.Popen(cmd, cwd=working_dir, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            stdout, stderr = process.communicate(timeout=time_out)
            return_code = process.returncode
            if return_code == 0:
                logger.info(f"Executable return code: {return_code}.")
                return stdout.strip()

            else:
                e_m = (f"Error running the executable. Return code: {return_code}, error: {stderr}.")
                logger.error(e_m)
                print(e_m)
                return None
        except subprocess.TimeoutExpired:
            process.terminate()
            print_v("The process timed out and has been terminated.")
            return None
        except Exception as e:
            print(f"An error occurred: {e}")
            return None
    else:
        e_m = (f"Error: Executable is not found. Check path in path_file or make sure it is compiled.")
        logger.error(e_m)
        print(e_m)
        sys.exit(1)

def get_crc(crc_str):
    args = []
    args.append(crc_str)
    crc_path = path_file.crc_executable_path

    try:
        process = subprocess.Popen([crc_path] + args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()
        return_code = process.returncode

        if return_code == 0:
            return stdout.decode().strip()
        else:
            e_m = f"Error executing CRC_tester.c. Return code: {return_code} at: {crc_path}"
            logger.error(e_m)
            print(e_m)
            return None
    except Exception as e:
        e_m = f"An error occurred with CRC_tester: {e}, executable expected at: {crc_path}."
        logger.error(e_m)
        print(e_m)
        return None
 

def read_file(log):
    try:
        with open(log, 'r', errors='ignore') as file:
            lines = file.readlines()
        return lines
    except Exception as e:
        print(f"File read error {log}: {e}")
        return None


def write_file(log, lines):
    with open(log, 'a') as file:
        file.write(str(lines) + '\n')


class Trial():
    def __init__(self, u2u_platform_channel):
        self.time_out               = 1;
        #self.trial_counter          = 0;
        #self.message_counter        = 0
        #self.index                  = type(self).trial_counter
        #type(self).trial_counter    += 1
        #self.port                   = 0
        #self.crc_val                = -1
        self.chapter_counter        = 0;
        #self.message_dict           = {}
        self.archive             = {}
        self.error_dict             = {}
        self.total_errors           = 0
        self.number_of_ports = 2
        if (u2u_platform_channel==3):
            self.number_of_ports = 3
        print_v(f"Trial setup for {u2u_platform_channel} channels and {self.number_of_ports} ports. ")

    def add_message(self, trial_counter):
        #sender = path_file.sender_list[0] # why the fuck is this set to 0?
        indices = test_patterns[trial_counter % len(test_patterns)]
        if len(indices)<6:
            print("List of indices too short")
            return None
        port = indices[0]
        sender = path_file.sender_list[indices[1]%len(path_file.sender_list)]
        receiver = path_file.receiver_list[indices[2]%len(path_file.receiver_list)]
        _rqs_list = path_file.rqs_list
        rqs = _rqs_list[(indices[3]%len(_rqs_list))]
        topic = path_file.topic_list[indices[4]%len(path_file.topic_list)]
        #_rqs_list = path_file.rqs_list
        if indices[5]==-1:
            chapter = self.chapter_counter
            self.chapter_counter = (self.chapter_counter + 1) % 255
        else:
            chapter = indices[5]
        payload_lines = read_file(path_file.random_payload_path)
        if payload_lines==None:
            print("No payload file found.")
            return None
        payload = payload_lines[indices[6]][:-2]

        crc_buffer = '::' + sender + ':' + receiver + ':' + rqs + ':' + topic + ':' + str(chapter) + ':' + str(len(payload)) + ':' + payload + ':0:'
        crc_val = get_crc(crc_buffer)
        new_message = crc_buffer + str(crc_val) + ':'
        try:
            logger.info(f"add_message:: removing previous test message in {path_file.test_messages_for_u2u_core}.")
            os.remove(path_file.test_messages_for_u2u_core)
        except OSError:
            err_m = f"{path_file.test_messages_for_u2u_core} is not found. Continuing creating and adding new_message."
            print(err_m)
            logger.error(err_m)
        write_file(path_file.test_messages_for_u2u_core, new_message)   
        self.archive[trial_counter] = {}
        self.archive[trial_counter]["in_message"] = {}
        self.archive[trial_counter]["in_message"]["message_raw"] = new_message
        self.archive[trial_counter]["in_message"]["message"] = Message()
        self.archive[trial_counter]["in_message"]["message"].parse(str(port), new_message)
        self.archive[trial_counter]["in_message"]["indices"] = indices
        self.archive[trial_counter]["in_message"]["port"] = str(port)
        self.archive[trial_counter]["error_list"] = ""
        self.archive[trial_counter]["error_count"] = 0 

        print_v(f"Messages indices: {indices}.")
        logger.info(f"add_message:: {new_message}")
        return port, new_message

    def u2u_self_test(self, trial_counter):
        port = self.archive[trial_counter]["in_message"]["port"]
        args_list = ['2', str(port)]
        result = run_u2u_executable(path_file.executable_path, args_list)
        logger.info(f"u2u_self_test:: {path_file.executable_path}, {args_list}. Result: {result}.")
        return result

    def fetch_response(self, trial_counter):
        responses = {} 
        file_counter = 0
        for uart in path_file.u2u_core_response_log_path:
            file = path_file.u2u_core_response_log_path[uart]
            if os.path.exists(file):
                responses[uart] = read_file(file)
                file_counter += 1
                os.remove(file)
            else:
                responses[uart] = None
        if (file_counter == 0):
            logger.warning(f"fetch_response:: path: {path_file.u2u_core_response_log_path}, each file was empty.")
        logger.info(f"fetch_response:: responses: {responses}.")
        self.archive[trial_counter]["out_message_counter"] = file_counter
        self.archive[trial_counter]["out_messages"] = responses
        ud = {}
        for u, m in self.archive[trial_counter]["out_messages"].items():
            #self.archive[trial_counter]["out_messages"]["messages"][u] = Message()
            ud[u] = Message()
            if m != None:
                ud[u].parse(u[-1], m[0])
        self.archive[trial_counter]["out_messages"]["messages"] = ud
        #print(self.archive[trial_counter]["out_messages"]["messages"]["uart1"].receiver)

        return (file_counter, responses)

    def _check_segments(self, trial_counter, message):
        max_lens = [0, 0, 32, 32, 8, 16, 8, 4, MAX_MESSAGE_PAYLOAD_SIZE, 4, 4, 99]
        min_lens = [0, 0,  3,  3, 2,  3, 1, 1, 0,                        1, 1, 0]
        lens = []
        r = 0
        segments = message.segments

        if len(segments)<11:
            r = 1  # Error code MESSAGE_INCOMPLETE
            print_v(f"Error: Message incomplete: {segments}")
        else:
            for i, s in enumerate(segments):
                lens.append(len(s))
                r = r + ((len(s) > max_lens[i]) * 1)  # Error code MESSAGE_SEGMENT_OVERSIZED
                r = r + ((len(s) < min_lens[i]) * 2)  # Error code MESSAGE_SEGMENT_UNDERSIZED
                if r!=0:
                    print_v(f"Error: Failure on segment length: {segments}")

            segments = segments[1:-1] # Cropping message so it aligns with index constants. 
            if int(segments[sg_LENGTH_INDEX])!=len(segments[sg_PAYLOAD_INDEX]):
                #print_v(int(segments[sg_LENGTH_INDEX]))
                r = r + 128  # Error code MESSAGE_PAYLOAD_LENGTH_MISMATCH
                print_v(f"Error: Failure on PAYLOAD lenght: {segments[sg_LENGTH_INDEX]} / {len(segments[sg_PAYLOAD_INDEX])}")

            if int(message.crc_val)!=int(segments[sg_CRC_INDEX]):
                print_v(f"Error: CRC Failure: crc val: {message.crc_val} / crc seg: {segments[sg_CRC_INDEX]}")
                r = r + 256 # Error code MESSAGE_CRC_FAILURE
    
        print_v(f"Checked segments. Result: {r}.")

    def _dict_error_logger(self, trial_counter, error_message):
        self.total_errors += 1 
        self.error_dict[trial_counter] =  error_message
        self.archive[trial_counter]["error_count"]+= 1
        self.archive[trial_counter]["error_list"]+= error_message

    def _consistancy_checker(self, trial_counter, message):
        self._check_segments(trial_counter, message)

    def _check_forwarding(self, trial_counter, out_message, in_message):
        if (out_message.sender != in_message.sender):
            e_m =  f"Senders are not consistent: in: '{in_message.sender}', out: '{out_message.sender}'. "
            self._dict_error_logger(trial_counter, e_m)
        if (out_message.receiver != in_message.receiver):
            e_m = f"Receivers are not consistent: in: '{in_message.receiver}', out: '{out_message.receiver}'. "
            self._dict_error_logger(trial_counter, e_m)
        if (out_message.rqs != in_message.rqs):
            e_m = f"R-flag are not consistent: in: '{in_message.rqs}', out: '{out_message.rqs}'. "
            self._dict_error_logger(trial_counter, e_m)
        if (out_message.topic != in_message.topic):
            e_m = f"Topics are not consistent: in: '{in_message.topic}', out: '{out_message.topic}'. "
            self._dict_error_logger(trial_counter, e_m)
        if (out_message.chapter != in_message.chapter):
            e_m = f"Chapters are not consistent: in: '{in_message.chapter}', out: '{out_message.chapter}'. "
            self._dict_error_logger(trial_counter, e_m)
        if (int(out_message.hops)-1 != int(in_message.hops)):
            e_m = f"Hops did not increment: in: '{in_message.hops}', out: '{int(out_message.hops)}'. "
            self._dict_error_logger(trial_counter, e_m)
        if (int(in_message.lenght) != int(out_message.lenght)):
            e_m = f"Lenghts are not consistent: in: '{in_message.lenght}', out: '{out_message.lenght}'. "
            self._dict_error_logger(trial_counter, e_m)
        if (in_message.payload != out_message.payload):
            e_m = f"Payloads are not consistent: in: '{in_message.payload}', out: '{out_message.payload}'. "
            self._dict_error_logger(trial_counter, e_m)

        if (self.archive[trial_counter]["error_count"] == 0):
            print_v(f"Checked forwarded message: No errors on message from port '{out_message.port}'")
        else:
            print_v(f"Checked forwarded message: Errors on message from port '{out_message.port}': '{self.archive[trial_counter]['error_list']}'")
        return

    def _check_responding(self, trial_counter, out_message, in_message):
        if (in_message.sender != out_message.receiver): 
            e_m = f"Sender/reveiver are not consistent: in s: '{in_message.sender}', out r: '{out_message.receiver}'. "
            self._dict_error_logger(trial_counter, e_m)  #
        if (out_message.sender != DUT_NAME):
            e_m = f"Sender not correct: out: '{out_message.sender}', '{DUT_NAME}'. "
            self._dict_error_logger(trial_counter, e_m)
        if (out_message.receiver != in_message.sender and in_message.receiver != "GEN"):
            e_m = f"Receivers are not consistent: in: '{in_message.receiver}', out: '{out_message.receiver}'. "
            self._dict_error_logger(trial_counter, e_m)
        if (out_message.rqs != "RS"):
            e_m = f"R-flag are not consistent: in: '{in_message.rqs}', out: '{out_message.rqs}'. "
            self._dict_error_logger(trial_counter, e_m)
        if (out_message.topic != in_message.topic):
            e_m = f"Topics are not consistent: in: '{in_message.topic}', out: '{out_message.topic}'. "
            self._dict_error_logger(trial_counter, e_m)
        if (out_message.chapter != in_message.chapter):
            e_m = f"Chapters are not consistent: in: '{in_message.chapter}', out: '{out_message.chapter}'. "
            self._dict_error_logger(trial_counter, e_m)
        if (int(out_message.hops)-1 != int(in_message.hops)):
            e_m = f"Hops did not increment: in: '{in_message.hops}', out: '{int(out_message.hops)}'. "
            self._dict_error_logger(trial_counter, e_m)

        if (self.archive[trial_counter]["error_count"] == 0):
            print_v(f"Checked response message: No errors on message from port '{out_message.port}'")
        else:
            print_v(f"Checked response message: Errors on message from port '{out_message.port}': '{self.archive[trial_counter]['error_list']}'")
        return

    def _check_no_response(self, trial_counter, in_message):
        # No response if and only if: RI & receiver == self, RS & receiver == GEN, receiver == self & RS.
        if (in_message.rqs == "RS" and (in_message.receiver == DUT_NAME or in_message.receiver == "GEN") or (in_message.rqs == "RI" and in_message.receiver == DUT_NAME)):
            print_v(f"Checked no response: expected behaviour.")
        else:
            e_m = f"No response received while no GEN/self addressing: rqs: {in_message.rqs}, receiver: {in_message.receiver}. Message: '{in_message.raw}'"
            self._dict_error_logger(trial_counter, e_m)
        return

    def _context_checker(self, trial_counter, out_message, in_message):
        if (in_message.port == out_message.port):
            self._check_responding(trial_counter, out_message, in_message)
        else:
            self._check_forwarding(trial_counter, out_message, in_message)
        return

    def _get_expected_messages(self, trial_counter, in_message):
        in_port = int(in_message.port)
        #ptest = self.number_of_ports
        #self.number_of_ports = 3
        #expected_messages = [-1 for i in range(self.number_of_ports)] 
        expected_messages = [0 for i in range(3)] 

        if in_message.receiver == "GEN":            # GEN addressing: RESP and FORW
            for p in range(self.number_of_ports):
                expected_messages[p] = 1
            if in_message.rqs == "RI":              # No reply, only forwarding
                expected_messages[in_port] = 0
        elif in_message.receiver == DUT_NAME:       # SELF addressing: RESP
            if in_message.rqs == "RI":              # No reply
                expected_messages[in_port] = 0
            else:
                expected_messages[in_port] = 1
            for p_ in range(self.number_of_ports-1):
                p = (p_ + 1 + in_port) % self.number_of_ports
                expected_messages[p] = 0
        elif ((in_message.rqs == "RI" and in_message.receiver == DUT_NAME) or (in_message.rqs == "RS" and in_message.receiver == "GEN") or (in_message.rqs == "RS" and in_message.receiver == DUT_NAME)):
            for p in range(self.number_of_ports):    # NORPL: 3 cases where no reply is valid
                expected_messages[p] = 0
        else:
            expected_messages[in_port] = 0           # OTHER addressing: FORW
            for p_ in range(self.number_of_ports-1):
                p = (p_ + 1 + in_port) % self.number_of_ports
                expected_messages[p] = 1
        return expected_messages


    def message_checker(self, trial_counter):
        in_message = self.archive[trial_counter]["in_message"]["message"]
        expected_messages = self._get_expected_messages(trial_counter, in_message)

        sum_exp_msg = 0
        for p in expected_messages:
            sum_exp_msg += p


        omc = self.archive[trial_counter]["out_message_counter"]
        if sum_exp_msg == 0:
            if omc != 0:
                e_m = f"No messages are expected but some are found: {omc}."
                self._dict_error_logger(trial_counter, e_m)
            self._check_no_response(trial_counter, in_message)
        else:
            for uart, out_message in  self.archive[trial_counter]["out_messages"]["messages"].items():
                in_port = int(uart[-1])
                if out_message.active:
                    self._consistancy_checker(trial_counter, out_message)
                    self._context_checker(trial_counter, out_message, in_message)
                    if (expected_messages[in_port] == 0):
                        e_m = f"On port {uart} message was found but it was not expected. "
                        self._dict_error_logger(trial_counter, e_m)
                else:
                    if (expected_messages[in_port] == 1):
                        e_m = f"On port {uart} message was expected but not found. "
                        self._dict_error_logger(trial_counter, e_m)
        return


if __name__ == "__main__":
    if (len(sys.argv) > 1):
        if "-v" in sys.argv or "--verbose" in sys.argv:
            option_dict["verbose_option"] = True
        if "--upc" in sys.argv:
            upc_ix = int((sys.argv).index("--upc")) + 1
            if upc_ix <= len(sys.argv):
                u2u_platform_channel = int(sys.argv[upc_ix])
                logger.info(f"Parameter u2u_platform_channel is set to {u2u_platform_channel}.")
        else:
            e_m_no_upc = f"Parameter u2u_platform_channel is not set."
            logger.error(e_m_no_upc)
            print("ERROR: " + e_m_no_upc)
            exit(1)
    

    total_trials = 100
    offset = 38 #38 self  # 8 GEN
    trial = Trial(u2u_platform_channel)

    for t in range(total_trials):
        trial_nr = t + offset
        print_v(f"________TRIAL {t} offset: {offset}________")
        port, n_m = trial.add_message(trial_nr )
        result_exe = trial.u2u_self_test(trial_nr )
        trial.fetch_response(trial_nr )
        trial.message_checker(trial_nr)
    if trial.total_errors == 0:
        fb_m = "-/===\-------------------------------"
        print(fb_m)
        logger.info(fb_m)
        fb_m = "-|>0<|-----|Test succeded|-----------"
        print(fb_m)
        logger.info(fb_m)
        fb_m = "-\===/-------------------------------"
        print(fb_m)
        logger.info(fb_m)
    else:
        fb_m = "-\-|-/-----------------------------------"
        print(fb_m)
        logger.info(fb_m)
        fb_m = f"-=>X<=------|Test failed: {trial.total_errors}|-----------"
        print(fb_m)
        logger.info(fb_m)
        fb_m = "-/-|-\-----------------------------------"
        print(fb_m)
        logger.info(fb_m)
        for tr, em in trial.error_dict.items():
            in_msg = trial.archive[tr]["in_message"]["message_raw"]
            p = trial.archive[tr]["in_message"]["port"]
            print_v(f"Trial: {tr}, error: {em}")
            print_v(f"in  uart{p}: '{in_msg}'")
            for u, m in trial.archive[tr]["out_messages"].items():
                if (u[:4] == "uart"):
                    print_v(f"out {u}: '{m}'")
    exit(0)


