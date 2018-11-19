import logging


def print_ms(seconds):
    logging.info("%.3f ms" % (seconds*1000))

def print_ms_message(seconds, message):
    logging.info(message)
    print_ms(seconds)
