#!/usr/bin/python3

import re
import subprocess
import path_file
import os
from os.path import exists
import sys


''' Assuming at this point that logs/uartN_tester_log.txt is empty and can be overwritten '''
MAX_MESSAGE_PAYLOAD_SIZE = 255
sg_WAIT_INDEX         = 0;
sg_PREMESSAGE_INDEX   = 0;
sg_SENDER_INDEX       = 1;
sg_RECEIVER_INDEX     = 2;
sg_RQS_INDEX          = 3;
sg_TOPIC_INDEX        = 4;
sg_CHAPTER_INDEX      = 5;
sg_LENGTH_INDEX       = 6;
sg_PAYLOAD_INDEX      = 7;
sg_HOPCOUNT_INDEX     = 8;
sg_CRC_INDEX          = 9;

'''test_patterns legend     
    0: PORT, 
    1: SENDER 
    2: RECEIVER
    3: R-FLAG
    4: TOPIC
    5: CHAPTER
    6: PAYLOAD
'''

option_dict = {"verbose_option" : False}

DUT_NAME = "TEST_RCVR"

pattern_indices = {"PORT" : 0, "SENDER" : 1, "RECEIVER" : 2, "R-FLAG" : 3, "TOPIC" : 4, "CHAPTER" : 5, "PAYLOAD" : 6, "HOPS" : 7, "CRC" : 8}

_test_patterns = [
        [0, 0, 0, 0, 0, 0, 0], 
        [1, 0, 1, 0, 1, 1, 1], 
        [0, 3, 3, 1, 6, 16, 16],
        [1, 3, 2, 1, 7, 17, 17],
        [0, 4, 4, 1, 8, 18, 18],
        [0, 1, 0, 0, 2, 2, 2],
        [1, 1, 1, 0, 3, 3, 3]]

 

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


class Message():
    def __init__(self):
        self.active = False
        self.port = None
        self.sender = None
        self.receiver = None
        self.rqs = None
        self.topic = None
        self.chapter = None
        self.lenght = None
        self.payload = None
        self.hops = None
        self.crc_rx = None
        self.crc_val = None
        self.raw = None


    def assign_segments(self, port, segments, raw_message):
        #print_v(f"from Messages: {segments}")
        self.raw = raw_message
        self.active = True
        self.port = port
        self.sender = segments[2]
        self.receiver = segments[3]
        self.rqs = segments[4]
        self.topic = segments[5]
        self.chapter = segments[6]
        self.lenght = segments[7]
        self.payload = segments[8]
        self.hops = segments[9]
        self.crc_rx = segments[10]


class Trial():
    trial_counter = 0;
    def __init__(self):
        self.time_out = 1;
        self.message_counter = 0
        self.index = type(self).trial_counter
        type(self).trial_counter += 1
        self.port = 0
        self.crc_val = -1
        self.chapter_counter = 0;

        self.message_dict = {}

    def run_u2u_executable(self, executable_path, args):
        try:
            process = subprocess.Popen([executable_path] + args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            stdout, stderr = process.communicate(timeout=self.time_out)
            return_code = process.returncode
            if return_code == 0:
                #print_v(f"Executable return code: {return_code}.")
                return stdout.decode().strip()
            else:
                print_v(f"Error running the executable. Return code: {return_code}.")
                return None
        except subprocess.TimeoutExpired:
            process.terminate()
            print_v("The process timed out and has been terminated.")
            return None
        except Exception as e:
            print(f"An error occurred: {e}")
            return None

    def get_crc(self, crc_str):
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
                print(f"Error executing CRC_tester.c. Return code: {return_code} at: {crc_path}")
                return None
        except Exception as e:
            print(f"An error occurred with CRC_tester: {e}, executable expected at: {crc_path}.")
            return None
    
    
    def read_file(self, log):
        try:
            with open(log, 'r', errors='ignore') as file:
                lines = file.readlines()
            return lines
        except Exception as e:
            print(f"File read error {log}: {e}")
            return None

    def write_file(self, log, lines):
        with open(log, 'a') as file:
            file.write(str(lines) + '\n')

    
    #SEGMENT_MAX_LENGTH[]      = {1, 32, 32, 8, 16, 8, 4, MAX_MESSAGE_PAYLOAD_SIZE, 4, 4};
    


    def add_message(self, items_index, number_of_messages=1): # int items_index is to offset in test_patterns
        for n_o_m in range(number_of_messages):
            sender = path_file.sender_list[0]
            indices = test_patterns[n_o_m + items_index]
            #print_v(f"nom: {n_o_m}, i_i: {items_index}, indices: {indices}")
            if len(indices)<6:
                print("List of indices too short")
                return None
            payload_lines = self.read_file(path_file.random_payload_path)
            if payload_lines==None:
                print("No payload file found.")
                return None
            payload = payload_lines[indices[6]][:-2]
            port = indices[0]
            receiver = path_file.receiver_list[indices[2]]
            rqs = path_file.rqs_list[indices[3]]
            topic = path_file.topic_list[indices[4]]
            if indices[5]==-1:
                chapter = self.chapter_counter
                self.chapter_counter = (self.chapter_counter + 1) % 255
            else:
                chapter = indices[5]

            crc_buffer = '::' + sender + ':' + receiver + ':' + rqs + ':' + topic + ':' + str(chapter) + ':' + str(len(payload)) + ':' + payload + ':0:'
        crc_val = self.get_crc(crc_buffer)
        new_message = crc_buffer + str(crc_val) + ':'
        try:
            os.remove(path_file.test_messages)
        except OSError:
            print(f"{path_file.test_messages} is not found. Continuing creating and adding new_message.")
        self.write_file(path_file.test_messages, new_message)   #"/home/pi/c_taal/u2u/test_messages/test_messages.txt
        return port, new_message

    def fetch_response(self):
        responses = []
        file_counter = 0
        for file in path_file.log_path:
            if os.path.exists(file):
                responses.append(self.read_file(file))
                file_counter += 1
                os.remove(file)
            else:
                responses.append(None)
        if (file_counter == 0):
            #print(f"No log files found at {path_file.log_path[1]}, has U2U been executed at all?")
            pass
        return responses



    def u2u_self_test(self, port):
        args_list = ['2', str(port)]
        result = self.run_u2u_executable(path_file.executable_path, args_list)
        return result

    def check_segments(self, segments):
        max_lens = [0, 0, 32, 32, 8, 16, 8, 4, MAX_MESSAGE_PAYLOAD_SIZE, 4, 4, 99]
        min_lens = [0, 0,  3,  3, 2,  3, 1, 1, 0,                        1, 1, 0]
        lens = []
        r = 0

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

            if int(self.crc_val)!=int(segments[sg_CRC_INDEX]):
                print_v(f"Error: CRC Failure: crc val: {self.crc_val} / crc seg: {segments[sg_CRC_INDEX]}")
                r = r + 256 # Error code MESSAGE_CRC_FAILURE
    
        print_v(f"Checked segments. Result: {r}.")
        return r
 

    def parse_message(self, message):
        if (message==None):
            return None
        else:
            segments = message.split(':')
            crc_buffer = ':'.join(segments[:-2]) + ':'
            self.crc_val = self.get_crc(crc_buffer)

            if len(segments)>12: # If payload contains ':' they will be split apart.
                new_segments = []
                new_segments = new_segments + segments[:8]
                new_payload = ':'.join(segments[8:-3])
                new_segments.append(new_payload)
                segments = new_segments + segments[-3:]
                #print_v(f"new segment: {new_segments}")
            #print_v(len(self.segments))
            #print_v(self.segments)
            #result = self.check_segments()
            #print_v(f"Message: {message}")
            #print_v(f"Segments: {segments}")
        return segments

    def check_for_forwarding(self, in_message, out_message):
        result = 0
        error_str = ""

        print_v(f"Checking forwarding message. \nInbound: <{in_message.raw}>, \nOutbound: <{out_message.raw}>")
        if (in_message.sender != out_message.sender):
            result += 1
            error_str += f"Senders are not consistent: in: {in_message.sender}, out: {out_message.sender}. "

        if (in_message.receiver != out_message.receiver):
            result += 1
            error_str += f"Receivers are not consistent: in: {in_message.receiver}, out: {out_message.receiver}. "

        if (in_message.rqs != out_message.rqs):
            result += 1
            error_str += f"R-flag are not consistent: in: {in_message.rqs}, out: {out_message.rqs}. "
        
        if (in_message.topic != out_message.topic):
            result += 1
            error_str += f"Topics are not consistent: in: {in_message.topic}, out: {out_message.topic}. "

        if (in_message.chapter != out_message.chapter):
            result += 1
            error_str += f"Chapters are not consistent: in: {in_message.chapter}, out: {out_message.chapter}. "

        if (in_message.lenght != out_message.lenght):
            result += 1
            error_str += f"Lenghts are not consistent: in: {in_message.lenght}, out: {out_message.lenght}. "
            
        if (in_message.payload != out_message.payload):
            result += 1
            error_str += f"Payloads are not consistent: in: {in_message.payload}, out: {out_message.payload}. "
            
        #print_v(f"HOPS   : in: {in_message.hops}, out: {out_message.hops}. ")
        #print_v(error_str)
        if (int(in_message.hops) != int(out_message.hops)-1):
            result += 1
            error_str += f"Hops did not increment: in: {in_message.hops}, out: {int(out_message.hops)}. "
            
        return result, error_str

    def check_for_response(self, in_message, out_message):
        result = 0
        error_str = ""

        if (in_message.rqs != "RI"):
            print_v(f"Checking response message. \nRequest: <{in_message.raw}>, \nresponse: <{out_message.raw}>")
            if (in_message.sender != out_message.receiver): # Verifying response uses correct sender.
                result += 1
                error_str += f"Incorrect response: in: {in_message.sender}, out: {out_message.receiver}. "
    
            if (in_message.receiver != DUT_NAME and in_message.receiver != "GEN"): # Is DUT truly addressed?
                result += 1
                error_str += f"Illegal response - not being addressed: in: {in_message.receiver}. "
    
            if (in_message.sender != out_message.receiver): # Is DUT actually responding to correct SENDER?
                result +1
                error_str += f"Response does not contain correct receiver: {out_message.receiver}."
    
            if (out_message.rqs != "RS"):
                result += 1
                error_str += f"Illegal R-flag for response: expected: 'RS', out: {out_message.rqs}. "
            
            if (in_message.topic != out_message.topic):
                result += 1
                error_str += f"Topics are not consistent: in: {in_message.topic}, out: {out_message.topic}. "
    
            if (in_message.chapter != out_message.chapter):
                result += 1
                error_str += f"Chapters are not consistent: in: {in_message.chapter}, out: {out_message.chapter}. "
    
            if (int(in_message.hops) != int(out_message.hops)-1):
                result += 1
                error_str += f"Hops did not increment: in: {in_message.hops}, out: {int(out_message.hops)}. "
    
            #if not (in_message.sender):

        return result, error_str


    def check_context(self, port, message_dict): 
        result = 0
        message_list = []
        segment_list = []
        error_str = "Error statements: "

        in_message = message_dict["INBOUND"]
        o0_message = message_dict["OUTBOUND"][0]
        o1_message = message_dict["OUTBOUND"][1]
        o0_segments = None
        o1_segments = None

        M_i = Message()
        M_0 = Message()
        M_1 = Message()


        in_segments = self.parse_message(in_message)
        M_i.assign_segments(port, in_segments, in_message)
        print_v(f"Context check for message: <{M_i.raw}>.")
        if (o0_message!=None):
            o0_segments = self.parse_message(o0_message[0])
            result = self.check_segments(o0_segments)
            M_0.assign_segments(0, o0_segments, o0_message)
        if (o1_message!=None):
            o1_segments = self.parse_message(o1_message[0])
            result = self.check_segments(o1_segments)
            M_1.assign_segments(1, o1_segments, o1_message)

        if (M_0.active==False and M_1.active==False):
            if (M_i.rqs=="RI"):
                error_str += "RI flag in INBOUND message. No response expected and no outbound messages found. This is expected behavour."
            else:
                error_str += "No outbound messages found."
            return result, error_str

        out_messages = [M_0, M_1]
        if (M_i.receiver == "GEN" and M_i.rqs != "RI"):
            if (M_0.active != True):
                result += 1
                if (port==0):
                    error_str += "Receiver: Gen, no response."
                if (port==1):
                    error_str += "Receiver: Gen, no forwarding."

            if (M_1.active != True):
               result += 1
               if (port==0):
                   error_str += "Receiver: Gen, no response."
               if (port==1):
                   error_str += "Receiver: Gen, no forwarding."


            print_v(f"Adressing GENERAL: <{M_i.receiver}>")
            new_result, new_error_str = self.check_for_response(M_i, out_messages[port])
            error_str += new_error_str
            result += new_result
            new_result, new_error_str = self.check_for_forwarding(M_i, out_messages[1-port])
            error_str += new_error_str
            result += new_result

        else:
            if (M_i.receiver == DUT_NAME):
                print_v(f"Addressing SELF ({DUT_NAME}): <{M_i.receiver}>")
                result, new_error_str = self.check_for_response(M_i, out_messages[port])
            else:
                print_v(f"Adressing OTHERS: <{M_i.receiver}>")
                result, new_error_str = self.check_for_forwarding(M_i, out_messages[1-port])
        return result, error_str

    def context_tester(self, trials=1):
        result = 0
        total_result = 0
        number_of_ports = 2
        for trial in range(trials):
            print_v(f"-------Trial {trial}-------")
            message_dict_test = {}
            message_dict_test["OUTBOUND"] = []
            port, new_message = self.add_message(trial, 1)
            message_dict_test["INBOUND"] = new_message
            result_exe = self.u2u_self_test(port)
            result_response = 0
            if (result_exe != None):
                responses = self.fetch_response()
                for p in range(number_of_ports):
                    response = responses[p]
                    message_dict_test["OUTBOUND"].append(response)
                result_response, error_str = self.check_context(port, message_dict_test)
                if result_response: 
                    print(f"*** Test concluded with error ({result_response}) statement: {error_str}. ***")
                    print_v("")
                else:
                    print_v(f"*** Test concluded with no errors. *** ")
                    print_v("")
                
                self.message_dict[trial] = message_dict_test
            else:
                result_response = -555
            total_result = total_result + result_response
                

        return total_result

def print_v(txt):
    if option_dict["verbose_option"]:
        print(txt)


if __name__ == "__main__":
    
    if (len(sys.argv) > 1):
        if "-v" in sys.argv or "--verbose" in sys.argv:
            option_dict["verbose_option"] = True
    r = 0

    port = 0
    t = Trial()
    #r = t.u2u_self_test(0)
    r = t.context_tester(len(test_patterns))
    print(f"All tests completed with cumulative result: {r}.")
    
    #args = ['0', '0', 'AZATHOTH', 'RI', 'HAIL', '02', 'kattekop']
    #args = compose_random_segments(0)

#    t.send_test_indexed(test_patterns[0])
#    t.send_test_indexed(test_patterns[1])
#    for tp in test_patterns:
#        t.receive_test_indexed(tp)
#    #t.receive_test_indexed(test_patterns[0])
#    #t.receive_test_indexed(test_patterns[1])
#    r = t.check_files()



    if r==0:
        print("-/===\-------------------------------")
        print("-|>0<|-----|Test succeded|-----------")
        print("-\===/-------------------------------")
    else:
        print("-\-|-/-----------------------------------")
        print(f"-=>X<=------|Test failed: {r}|-----------")
        print("-/-|-\-----------------------------------")

    sys.exit(0)


