#!/usr/bin/python3

import logging.config

LOGGING_CONFIG = {
    'version': 1,
    'formatters': {
        'default': {
            'format': '%(asctime)s: %(levelname)s %(message)s'
        }
    },
    'handlers': {
        'u2uctl': {
            'class': 'logging.FileHandler',
            'filename': 'logs/u2uctl.log',
            'encoding': 'utf-8',
            'level': 'DEBUG',
            'formatter': 'default'
        },
        'data_processing': {
            'class': 'logging.FileHandler',
            'filename': 'logs/data_processing.log',
            'encoding': 'utf-8',
            'level': 'DEBUG',
            'formatter': 'default'
        },
        'target': {
            'class': 'logging.FileHandler',
            'filename': 'logs/target.log',
            'encoding': 'utf-8',
            'level': 'DEBUG',
            'formatter': 'default'
        }
    },
    'loggers': {
        'u2uctl': {
            'handlers': ['u2uctl'],
            'level': 'DEBUG'
        },
        'data_processing': {
            'handlers': ['data_processing'],
            'level': 'DEBUG'
        },
        'target':{
            'handlers': ['target'],
            'level': 'DEBUG'
            }
        }

}

logging.config.dictConfig(LOGGING_CONFIG)

