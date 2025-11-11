#!/usr/bin/python3


sender_list         = ["AZATHOTH", "BLUESCREEN", "BLOODHOUND", "COCADEMON", "TEST_SNDR"]
receiver_list       = ["BRIANLEWIS", "PINHEAD", "QUEENBEE", "TEST_RCVR", "GEN"]

topic_list          = ["HAIL____", "HELP____", "SET_LCD_", "SET_OLED", "GET_SNSR", "SET_ENCR", "GET_ENCR", "SET_LED_", "SET_TIME", "GET_TIME", "SET_DATE", "GET_DATE", "RSERVD_0", "RSERVD_1", "RSERVD_2", "RSERVD_3", "RSERVD_4"]
rqs_list            = ["RQ" ,"RI" ,"RS" ,"RN", "RA"]
rqs_list_norm       = ["RQ" ,"RI" ,"RS"]
rqs_list_tx         = ["RQ" ,"RI"]
port_list           = ["0" ,"1", "2"]

''' Path to binary '''
executable_path                 = "../u2u_tester"
''' Not used '''
message_path                    = "../logs/message_log.txt"
''' Responses from u2u executable are put based on what uart it is sent from '''
u2u_core_response_log_path      = {"uart0" : "../logs/uart0_tester_log.txt", "uart1" : "../logs/uart1_tester_log.txt", "uart2" : "../logs/uart2_tester_log.txt"}
random_payload_path             = "../testing/random_payload.txt"
crc_executable_path             = "../../utils/crc_tester/crc_tester"
''' Not used '''
test_log_path                   = "../logs/test_logs/test_log_file.txt"
''' Messages are placed here for u2u_core to process '''
test_messages_for_u2u_core      = "../test_messages_for_u2u_core.txt"
