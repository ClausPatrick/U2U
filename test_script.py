#!/usr/bin/python3

import re
import subprocess
import numpy as np
import path_file

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



class Trial():
    trial_counter = 0;
    def __init__(self):
        self.message_counter = 0
        self.message_list = []
        #self.index = trial_counter
        self.index = type(self).trial_counter
        type(self).trial_counter += 1
        #trial_counter += 1
        self.port = 0
        self.crc_val = -1


    def run_u2u_executable(self, executable_path, args):
        try:
            output = subprocess.check_output([executable_path] + args)
            return output.decode().strip()
        except subprocess.CalledProcessError as e:
            print(f"Error executing the executable: {e}")
            return None

    def get_crc(self, crc_str):
        args = []
        args.append(crc_str)

        try:
            process = subprocess.Popen([path_file.crc_executable_path] + args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            stdout, stderr = process.communicate()
            return_code = process.returncode

            if return_code == 0:
                return stdout.decode().strip()
            else:
                print(f"Error executing the executable. Return code: {return_code}")
                return None
        except Exception as e:
            print(f"An error occurred: {e}")
            return None


    def read_file(self, log):
        try:
            with open(log, 'r', errors='ignore') as file:
                lines = file.readlines()
            return lines
        except:
            print(f"File read error {log}")
            return None


    #SEGMENT_MAX_LENGTH[]      = {1, 32, 32, 8, 16, 8, 4, MAX_MESSAGE_PAYLOAD_SIZE, 4, 4};




    def compose_random_segments(self, port):
        ITEMS = 5
        payload_lines = self.read_file(path_file.random_payload_path)
        if payload_lines==None:
            return None
#        with open(random_payload_path, 'r', errors='ignore') as file:
#            lines = file.readlines()
        rand_index = np.random.randint(100%len(payload_lines))
        payload = payload_lines[rand_index][:-2]
        #payload = "Aar:sneu:keni:sluk"

        rand_index = np.random.randint(100%(len(path_file.receiver_list)-1)+1)
        receiver = path_file.receiver_list[rand_index]
        rand_index = np.random.randint(100%(len(path_file.topic_list)-1)+1)
        topic = path_file.topic_list[rand_index]
        rand_index = np.random.randint(100%(len(path_file.rqs_list)-1)+1)
        rqs = path_file.rqs_list[rand_index]
        #rand_index = np.random.randint(100%(len(port_list)-1)+1)
        #port = path_file.port_list[rand_index]
        chapter = str(np.random.randint(255))

        return_list = ['0', str(self.port), receiver, rqs, topic, chapter, payload]
        print(return_list)
        return return_list

    def compose_random_message(self, port):
        rand_index = np.random.randint(100%(len(path_file.sender_list)-1)+1)
        sender = path_file.sender_list[rand_index]
        items = self.compose_random_segments(port)
        payload_len = str(len(items[-1]))
        #crc_buffer = sender + ':'.join(items[2:])
        crc_buffer = '::' + sender + ':' + items[2] + ':' + items[3] + ':' + items[4] + ':' + items[5] + ':' + payload_len + ':' + items[6] + ':0:'
        crc_val = self.get_crc(crc_buffer)
        new_message = crc_buffer + str(crc_val) + ':'
        print(f"items: {items}")
        print(f"crc bu: {crc_buffer}")
        return new_message

    def check_segments(self):
        max_lens = [0, 0, 32, 32, 8, 16, 8, 4, MAX_MESSAGE_PAYLOAD_SIZE, 4, 4, 99]
        min_lens = [0, 0,  3,  3, 2,  3, 1, 1, 0,                        1, 1, 0]
        lens = []
        r = 0
        if len(self.segments)<11:
            r = 1  # Error code MESSAGE_INCOMPLETE
        else:
            for i, s in enumerate(self.segments):
                lens.append(len(s))
                r = r + ((len(s) > max_lens[i]) * 1)  # Error code MESSAGE_SEGMENT_OVERSIZED
                r = r + ((len(s) < min_lens[i]) * 2)  # Error code MESSAGE_SEGMENT_UNDERSIZED
            self.segments = self.segments[1:-1] # Cropping message so it aligns with index constants.
            if int(self.segments[sg_LENGTH_INDEX])!=len(self.segments[sg_PAYLOAD_INDEX]):
                print(int(self.segments[sg_LENGTH_INDEX]))
                r = r + 128  # Error code MESSAGE_PAYLOAD_LENGTH_MISMATCH

            if int(self.crc_val)!=int(self.segments[sg_CRC_INDEX]):
                print(f"CRC Failure: crc val: {self.crc_val}, crc seg: {self.segments[sg_CRC_INDEX]}")
                r = r + 256 # Error code MESSAGE_CRC_FAILURE

        print("result: ", r)
        self.result = r
        return 0

    def parse_message(self, message):
        self.message_counter += 1
        self.message_list.append(message)
        self.segments = message.split(':')
        crc_buffer = ':'.join(self.segments[:-2]) + ':'
        self.crc_val = self.get_crc(crc_buffer)

        if len(self.segments)>12: # If payload contains ':' they will be split apart.
            new_segments = []
            new_segments = new_segments + self.segments[:8]
            new_payload = ':'.join(self.segments[8:-3])
            new_segments.append(new_payload)
            self.segments = new_segments + self.segments[-3:]
            #print(f"new segment: {new_segments}")
        print(len(self.segments))
        print(self.segments)
        result = self.check_segments()
        print(f"Message: {message}")
        return result

    def receive_test(self, port):
        self.port = port
        new_message = self.compose_random_message(port)
        args_list = []
        args_list.append('1')
        args_list.append(str(port))
        args_list.append(new_message)
        result = self.run_u2u_executable(path_file.executable_path, args_list)
        print(f"result receive test: {result}")


    def send_test(self, port):
        self.port = port
        args_list = self.compose_random_segments(port)
        result = self.run_u2u_executable(path_file.executable_path, args_list)
        if result is not None:
            output = self.read_file(path_file.log_path[0])
            if output==None:
                return 1
            for l in output:
                print(l)
                self.parse_message(l)
            output = self.read_file(path_file.log_path[1])
            if output==None:
                return 1
            for l in output:
                print(l)
                self.parse_message(l)
            return 0

if __name__ == "__main__":


    #args = ['0', '0', 'AZATHOTH', 'RI', 'HAIL', '02', 'kattekop']
    #args = compose_random_segments(0)

    port = 0
    t = Trial()
    t.send_test(port)
    t.send_test(1-port)
    t.send_test(port)
    t.send_test(1-port)
    t.send_test(port)
    t.send_test(1-port)
    t.send_test(port)
    t.send_test(1-port)
    t.receive_test(port)
    t.receive_test(1-port)
    t.receive_test(port)
    t.receive_test(1-port)
    t.receive_test(port)
    t.receive_test(1-port)
    t.receive_test(port)
    t.receive_test(1-port)
