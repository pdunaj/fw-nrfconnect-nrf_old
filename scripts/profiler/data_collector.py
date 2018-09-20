from rtt_nordic_profiler_host import RttNordicProfilerHost
import sys
import argparse
import logging

parser = argparse.ArgumentParser(
    description='Collecting data from Nordic profiler for given time and saving to files.')
parser.add_argument('time', type=int, help='Time of collecting data')
parser.add_argument('event_csv', help='.csv file to save collected events')
parser.add_argument('event_descr', help='.json file to save events descriptions')
parser.add_argument('--log', help='Log level')
args = parser.parse_args()

if args.log is not None:
	log_lvl_number = getattr(logging, args.log.upper(), None)
	if not isinstance(log_lvl_number, int):
		raise ValueError("Invalid log level: %s" % args.log)

else:
	log_lvl_number = logging.WARNING


profiler = RttNordicProfilerHost(event_filename=args.event_csv,
                                 event_types_filename=args.event_descr,
				 log_lvl=log_lvl_number)
profiler.get_events_descriptions()
profiler.read_events_rtt(args.time)
profiler.disconnect()
