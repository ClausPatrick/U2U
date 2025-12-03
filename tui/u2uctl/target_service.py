#!/usr/bin/python3

""" ********************** """
""" ***     target     *** """
""" ********************** """

import os
import os.path
import sys
import time
import json
import subprocess
import logging
import logging_config

logger = logging.getLogger("target")


class Target(): # Target refers to u2u_HAL_lx
    def __init__(self, self_name="DEFAULT_NAME"):
        self.Self_Name = self_name
        self.get_paths()
        self.remote_hosts = []
        self.remote_hostsnames = []
        self.remote_states = [[0], [0]]
        self.systemd_cmd_check = "sudo systemctl is-active u2u_node.service"
        self.systemd_cmd_start = "sudo systemctl start u2u_node.service"
        self.systemd_cmd_stop = "sudo systemctl stop u2u_node.service"
        self.active = 0
        self.active_remote = 0
        self.process_local = None
        self.process_remote = []
        logger.info(f'CC target construct.')

    def get_paths(self):
        with open("paths.json", "r") as f:
            paths_file = json.load(f)
        self.hal_pipe_out  = paths_file["paths"]["hal_pipe_out"]
        self.hal_pipe_in  = paths_file["paths"]["hal_pipe_in"]

    def set_hosts(self, json_file):
        with open(json_file, "r") as f:
            data = json.load(f)

        for host in data["hosts"]:
            if self.Self_Name != host["hostname"]:
                self.remote_hostsnames.append(host["hostname"])
                user = host["user"]
                ip = host["address"]
                self.remote_hosts.append(f"{user}@{ip}")
        logger.debug(f"target.set_hosts:: hostnames: {self.remote_hostsnames}, hosts: {self.remote_hosts}")
        return

    def pipe_command(self, command):
        logger.info(f'Piping send command from {self.Self_Name}-target: "{command}"')
        with open(self.hal_pipe_out, 'w') as pipe:
            cmd = "-c" + command + "\n"
            pipe.write(cmd)
            logger.info(f"Piped command: '{cmd}'")
        with open(self.hal_pipe_in, 'r') as pipe:
            while True:
                pipe_data = pipe.read()
                if len(pipe_data) == 0:
                    break
                if pipe_data[0]=='0' or pipe_data[0]==0:
                    logger.info(f'U2U core responded with success "{pipe_data}".')
                    break
                else:
                    logger.error(f'U2U core response error: "{pipe_data}".')
                    break
            logger.info(f"pipe_command loop break.")
        return pipe_data


    def fifo_format(self, message):
        message_f = f" #{message.sender} {message.receiver} {message.rqs} {message.topic_ix} {message.chapter} {message.payload}#"
        return message_f

    def set_payload(self, new_payload):
        p_len = len(new_payload)
        if (p_len):
            self.Payload = new_payload
        else:
            logger.error("Payload ERROR: empty")

    def send_message(self, message, uart):
        formatted_message = self.fifo_format(message)
        formatted_message = f"S{uart} " + formatted_message
        logger.info(f'Sending u2u message via uart{uart}: "{formatted_message}".')
        r = self.pipe_command(formatted_message)
        #print(f"Pipe return value: '{r}'")
        return r
 

    def get_target_status(self):
        p = subprocess.run(self.systemd_cmd_check.split(), capture_output=True)
        logger.info(f"get_target_status:: {p.stdout.decode().strip()}, {p.stderr.decode().strip()}")
        r = p.stdout
        if (r==b'active\n'):
            self.active = 1
        elif (r==b'inactive\n'):
            self.active = 0
        else: 
            self.active = -1
        return self.active


    def get_target_status_remote(self, host):
        cmd = ['ssh', host]
        cmd = cmd + self.systemd_cmd_check.split()
        p = subprocess.run(cmd, capture_output=True)
        logger.info(f"get_target_status_remote:: {p.stdout.decode().strip()}, {p.stderr.decode().strip()}")
        r = p.stdout
        if (r==b'active\n'):
            return 1
        elif (r==b'inactive\n'):
            return 0
        else: 
            return -1

    def enable_local(self):
        r = f'Enabling local target process.'
        logger.info(r)
        if self.get_target_status()==0:
            p = subprocess.run(self.systemd_cmd_start.split(), capture_output=True, check=True)
            if self.get_target_status()==1:
                r = f'Local target process enabled. '
                logger.info(r)
                logger.info(f"enable_local:: output: {p.stdout}, error: {p.stderr}")
            else:
                r = f'Local target process not enabled due to error.'
                logger.error(r)
            return r
        if self.get_target_status()==1:
            r = f'Cannot enable local target process; already started.'
            logger.warning(r)
            return r


    def enable_remote(self):
        r = ""
        if len(self.remote_hosts):
            for host_i, host in enumerate(self.remote_hosts):
                r = r + self._enable_remote_host(host)
        else:
            r = "No information on remote hosts. Ensure host_data.json is present and call set_hosts()."
        return r


    def _enable_remote_host(self, host):
        r = f'Enabling remote target process {host}.'
        logger.info(r)
        target_status_return = self.get_target_status_remote(host)
        if target_status_return==0:
            cmd = ['ssh', host]
            cmd = cmd + self.systemd_cmd_start.split()
            p = subprocess.run(cmd, capture_output=True)
            if self.get_target_status_remote(host)==1:
                r = f'Remote target {host} process enabled. '
                logger.info(r)
                logger.info(f"_enable_remote_host:: output: {p.stdout}, error: {p.stderr}")
            else:
                r = f'Remote target {host} process not enabled due to error.'
                logger.error(r)
            return r
        if target_status_return==1:
            r = f'Cannot enable remote target {host} process; already started.'
            logger.warning(r)
            return r
        if target_status_return==-1:
            r = f'Cannot enable remote target {host} process; request failed.'
            logger.error(r)
            return r


    def disable_local(self):
        r = f'Disenabling local target process.'
        logger.info(r)
        if self.get_target_status()==1:
            self.write_to_ta('Q00')
            while (self.get_target_status()==1):
                time.sleep(1)

            self.active = 0
            r = f'Local target process disabled.'
            return r
        if self.get_target_status()==0:
            r = f'Cannot disable local target process; already disabled.'
            logger.warning(r)
            return r


    def force_stop_local(self):
        r = f'Disenabling target process.'
        p = subprocess.run(self.systemd_cmd_stop.split(), capture_output=True)
        logger.info(f"Force stop_local:: output: {p.stdout}, error: {p.stderr}")
        return r

    def force_stop_remote(self):
        r = f'Disenabling remote process.'
        if len(self.remote_hosts):
            for host_i, host in enumerate(self.remote_hosts):
                r = r + self._force_stop_remote_host(host)
        else:
            r = "No information on remote hosts. Ensure host_data.json is present and call set_hosts()."

        return r


    def _force_stop_remote_host(self, host):
        r = f'Stopping remote process {host}.'
        logger.info(r)
        target_status_return = self.get_target_status_remote(host)
        if target_status_return==1:
            cmd = ['ssh', host]
            cmd = cmd + self.systemd_cmd_stop.split()
            p = subprocess.run(cmd, capture_output=True)
            if self.get_target_status_remote(host)==0:
                r = f'Remote target {host} process disabled. '
                logger.info(r)
                logger.info(f"_force_stop_remote_host:: output: {p.stdout}, error: {p.stderr}")
            else:
                r = f'Remote target {host} process not disabled due to error.'
                logger.error(r)
            return r
        if target_status_return==0:
            r = f'Cannot disable remote target {host} process; already stopped.'
            logger.warning(r)
            return r
        if target_status_return==-1:
            r = f'Cannot disable remote target {host} process; request failed.'
            logger.error(r)
            return r


    def disable_remote(self):
        r = ""
        for host_i, host in enumerate(self.remote_hosts):
            r = r + self._disable_remote_host(host)
        return r

    def _disable_remote_host(self, host):
        r = ""
        target_status_return = self.get_target_status_remote(host)
        if target_status_return==1:
            cmd = ['ssh', host]
            cmd = cmd + f"echo 'Q00' > {self.hal_pipe_out}".split()
            p = subprocess.run(cmd, capture_output=True)
            while (self.get_target_status_remote(host))==1:
                time.sleep(1)
            r = f'Remote target process on {host} disabled. '
            logger.info(f"_disable_remote_host:: output: {p.stdout}, error: {p.stderr}")
            return r
        if target_status_return==0:
            r = f'Cannot disable remote target process; already disabled.'
            logger.warning(r)
            return r
        if target_status_return==-1:
            r = f'Cannot disable remote target process; request failed.'
            logger.error(r)
            return r
                  
    def get_stack_status(self):
        if self.active==1:
            self.write_to_ta("US")
            logger.info(f'Target request for stack-status.')
            r = self.read_from_ta()
            logger.info(f'Target responded: "{r}".')
        else:
            r = "Target not started; enable target first: 'target -enable'"
        return r

    def u2u_enable_local(self):
        if self.active==1:
            self.write_to_ta("U1")
            logger.info(f'Target enabling u2u-stack command sent.')
            r = self.read_from_ta()
            logger.info(f'Target responded: "{r}".')
        else:
            r = "Target not started; enable target first: 'target -enable'"
        return r

    def u2u_disable_local(self):
        if self.active==1:
            self.write_to_ta("U0")
            logger.info(f'Target disabling u2u-stack command sent.')
            r = self.read_from_ta()
            logger.info(f'Target responded: "{r}".')
        else:
            r = "Target not started; enable target first: 'target -enable'"
        return r

    def get_stack_status_remote(self):
        if self.active==1:
            self.write_to_ta_remote("US")
            logger.info(f'Remote target request for remote stack-status.')
            r = self.read_from_ta_remote()
            logger.info(f'Remote target responded: "{r}".')
        else:
            r = "Target not started; enable target first: 'target -remote-enable'"
        return r

    def u2u_enable_remote(self):
        if self.active==1:
            self.write_to_ta_remote("U1")
            logger.info(f'Remote target enabling remote u2u-stack command sent.')
            r = self.read_from_ta_remote()
            logger.info(f'Remote target responded: "{r}".')
        else:
            r = "Remote target not started; enable target first: 'target -remote-enable'"
        return r

    def u2u_disable_remote(self):
        if self.active==1:
            self.write_to_ta_remote("U0")
            logger.info(f'Remote target disabling remote u2u-stack command sent.')
            r = self.read_from_ta_remote()
            logger.info(f'Remote target responded: "{r}".')
        else:
            r = "Remote target not started; enable target first: 'target -remote-enable'"
        return r

    def read_from_ta(self):
        data = None
        if (os.path.exists(self.hal_pipe_in)):
            try:
                with open(self.hal_pipe_in, "r") as pipe:
                    data = pipe.read()
                logger.info(f'Pipe data read: "{data}".')
            except FileNotFoundError:
                logger.warning(f'Named pipe at {self.hal_pipe_in} not found. Creating pipe now.')
            except OSError as e:
                logger.warning(f'Failure to read from pipe at {self.hal_pipe_in }.')
        else:
            logger.warning(f'Pipe has not yet been created: {self.hal_pipe_in }.')
        return data
        
    def write_to_ta(self, data):
        if (os.path.exists(self.hal_pipe_in)):
            try:
                with open(self.hal_pipe_out, "w") as pipe:
                    pipe.write(data + "\n")
                logger.info(f'Pipe data write: "{data}"')
            except FileNotFoundError:
                logger.warning(f'Named pipe at {self.hal_pipe_out} not found. Creating pipe now.')
            except OSError as e:
                logger.warning(f'Failure to read from pipe at {self.hal_pipe_out}.')
        else:
            logger.warning(f'Pipe has not yet been created: {self.hal_pipe_in }.')
        return


    def read_from_ta_remote(self):
        data = "" 
        logger.info(f"Reading from remote host.")
        for host_i, host in enumerate(self.remote_hosts):
            logger.info(f"Host {host_i}: {host}.")
            r = subprocess.run(['ssh', host, f"cat {self.hal_pipe_in}"], capture_output=True).stdout.decode()
            logger.info(f'Pipe data read {host_i}, {host}: "{r}".')
            data += r
        return data
            
    
    def write_to_ta_remote(self, data):
        logger.info(f"Writing to remote host.")
        for host_i, host in enumerate(self.remote_hosts):
            subprocess.run(['ssh', host, f"echo  {data} > {self.hal_pipe_out}"], check=True)
            time.sleep(0.2)
            logger.info(f'Pipe data write {host_i}, {host}: "{data}".')


    def _close_remote(self):
        cmd_quit = f"echo  'Q00' >  {ca_ta_pipe}"
        for host_i, host in enumerate(self.remote_hosts):
            process = subprocess.run(['ssh', host, cmd_quit], check=True)
            logger.info(f"Remote target process {host_i}, {host} input: {cmd_quit}: output: {process.stdout}, error: {process.stderr}.")
        return
    
    
    def pipe_cleaner(self):
        pipes = [self.hal_pipe_out, self.hal_pipe_in]
        for pipe in pipes:
            logger.info(f'Cleaning pipe {pipe}.')
            try:
                os.remove(pipe)
                logger.info(f'Pipe {pipe} removed.')
            except:
                logger.warning(f'Pipe {pipe} not found.')
                pass
    
    
    """ Sending template, former 'cmd_test()' with options:
    0: HAIL
    1: SENSOR
    3: TIMING
    """
    def send_from_template(self, option=0):
        cmd = f"T{option}"
        r = self.pipe_command(cmd)
        return r

if __name__ == "__main__":
    exit(0)
    
