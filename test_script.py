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
        pass


class Trial():
    trial_counter = 0;
    def __init__(self):
        self.message_counter = 0
        self.message_list = []
        self.index = type(self).trial_counter
        type(self).trial_counter += 1
        self.port = 0
        self.crc_val = -1
        self.chapter_counter = 0;


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
        except Exception as e:
            print(f"File read error {log}: {e}")
            return None


    #SEGMENT_MAX_LENGTH[]      = {1, 32, 32, 8, 16, 8, 4, MAX_MESSAGE_PAYLOAD_SIZE, 4, 4};

    def compose_indexed_segments(self, indices):
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
        return_list = ['0', str(port), receiver, rqs, topic, str(chapter), payload]
        return return_list

    def compose_random_segments(self, port):
        ITEMS = 5
        payload_lines = self.read_file(path_file.random_payload_path)
        if payload_lines==None:
            return None
#        with open(random_payload_path, 'r', errors='ignore') as file:
#            lines = file.readlines()
        rand_index = np.random.randint(len(payload_lines))
        payload = payload_lines[rand_index][:-2]

        rand_index = np.random.randint(len(path_file.receiver_list))
        receiver = path_file.receiver_list[rand_index]
        rand_index = np.random.randint(len(path_file.topic_list))
        topic = path_file.topic_list[rand_index]
        rand_index = np.random.randint(len(path_file.rqs_list))
        rqs = path_file.rqs_list[rand_index]
        #rand_index = np.random.randint(100%(len(port_list))
        #port = path_file.port_list[rand_index]
        chapter = str(self.chapter_counter)
        self.chapter_counter = (self.chapter_counter + 1) % 255
        return_list = ['0', str(self.port), receiver, rqs, topic, chapter, payload]
        #print(f"return_list: {return_list}")
        return return_list

    def compose_indexed_message(self, indices):
        sender = path_file.sender_list[indices[1]]
        items = self.compose_indexed_segments(indices)
        payload_len = str(len(items[-1]))
        #crc_buffer = sender + ':'.join(items[2:])
        crc_buffer = '::' + sender + ':' + items[2] + ':' + items[3] + ':' + items[4] + ':' + items[5] + ':' + payload_len + ':' + items[6] + ':0:'
        crc_val = self.get_crc(crc_buffer)
        new_message = crc_buffer + str(crc_val) + ':'
        #print(f"items: {items}")
        #print(f"crc bu: {crc_buffer}")
        return new_message

    def compose_random_message(self, port):
        rand_index = np.random.randint(100%(len(path_file.sender_list)-1)+1)
        sender = path_file.sender_list[rand_index]
        items = self.compose_random_segments(port)
        payload_len = str(len(items[-1]))
        #crc_buffer = sender + ':'.join(items[2:])
        crc_buffer = '::' + sender + ':' + items[2] + ':' + items[3] + ':' + items[4] + ':' + items[5] + ':' + payload_len + ':' + items[6] + ':0:'
        crc_val = self.get_crc(crc_buffer)
        new_message = crc_buffer + str(crc_val) + ':'
        #print(f"items: {items}")
        #print(f"crc bu: {crc_buffer}")
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
                #print(int(self.segments[sg_LENGTH_INDEX]))
                r = r + 128  # Error code MESSAGE_PAYLOAD_LENGTH_MISMATCH

            if int(self.crc_val)!=int(self.segments[sg_CRC_INDEX]):
                print(f"CRC Failure: crc val: {self.crc_val}, crc seg: {self.segments[sg_CRC_INDEX]}")
                r = r + 256 # Error code MESSAGE_CRC_FAILURE

        if r!=0:
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
        #print(len(self.segments))
        #print(self.segments)
        result = self.check_segments()
        #print(f"Message: {message}")
        return result

    def parse_segments(self, segments, test_index):
        pattern = test_patterns[test_index]
        segment_list = (segments.split('  '))[1:]
        if int(segment_list[5])!=test_index: # Chapter should be index.
            print(f"c: {segment_list[5]}")
            return 2**9 # Error code MESSAGE_CHAPTER_INCORRECT
        if int(segment_list[-4]) != int(segment_list[-5]):
            print(f"c: {segment_list[-4]}")
            print(f"c: {segment_list[-3]}")
            return 2**10 # Error code MESSAGE_CRC_INCORRECT
        if segment_list[1] != (path_file.sender_list[pattern[1]]):
            print(f"s: {segment_list[1]}")
            print(f"s: {path_file.sender_list[pattern[1]]}")
            return 2**11
        if segment_list[2] != (path_file.receiver_list[pattern[2]]):
            print(f"r: {segment_list[2]}")
            print(f"r: {path_file.receiver_list[pattern[2]]}")
            return 2**12 # Error code MESSAGE_RECEIVER_INCORRECT
        if segment_list[4] != (path_file.topic_list[pattern[4]]):
            print(f"t: {segment_list[4]}")
            print(f"t: {path_file.topic_list[pattern[4]]}")
            return 2**13 # Error code MESSAGE_TOPIC_INCORRECT
        return 0





    def check_files(self):
        output = self.read_file(path_file.log_path[0])
        return_val = 0
        if output==None:
            return 1
        for output_line in output:
            #print(output_line)
            return_val = self.parse_message(output_line)
        output = self.read_file(path_file.log_path[1])
        if output==None:
            return 1
        for output_line in output:
            #print(output_line)
            return_val = return_val + self.parse_message(output_line)
        messages_rx = self.read_file(path_file.message_path)
        for i, mr in enumerate(messages_rx):
            return_val = return_val + self.parse_segments(mr, i)
        return return_val

    def receive_test_indexed(self, indices):
        self.port = indices[0]
        new_message = self.compose_indexed_message(indices)
        args_list = []
        args_list.append('1')
        args_list.append(str(self.port))
        args_list.append(new_message)
        result = self.run_u2u_executable(path_file.executable_path, args_list)
        #print(f"result receive test: {result}")
        if result is not None:
            return 0
        return 1

    def receive_test(self, port):
        self.port = port
        new_message = self.compose_random_message(port)
        args_list = []
        args_list.append('1')
        args_list.append(str(port))
        args_list.append(new_message)
        result = self.run_u2u_executable(path_file.executable_path, args_list)
        print(f"result receive test: {result}")

    def send_test_indexed(self, indices):
        self.port = indices[0]
        args_list = self.compose_indexed_segments(indices)
        result = self.run_u2u_executable(path_file.executable_path, args_list)
        if result is not None:
            return 0
        return 1

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
    t.send_test_indexed(test_patterns[0])
    t.send_test_indexed(test_patterns[1])
    for tp in test_patterns:
        t.receive_test_indexed(tp)
    #t.receive_test(1-port)
    r = t.check_files()
    if r==0:
        print("Test succeded")
    else:
        print(f"Test failed: {r}")

