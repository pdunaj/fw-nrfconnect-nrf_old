from plot_nordic import PlotNordic
import sys
import argparse
import logging

parser = argparse.ArgumentParser(
    description='Plotting events from given files.')
parser.add_argument('event_csv', help='.csv file to save collected events')
parser.add_argument(
    'event_descr',
    help='.json file to save events descriptions')
parser.add_argument('--log', help='Log level')
args = parser.parse_args()

if args.log is not None:
	log_lvl_number = getattr(logging, args.log.upper(), None)
	if not isinstance(log_lvl_number, int):
		raise ValueError("Invalid log level: %s" % args.log)

else:
	log_lvl_number = logging.WARNING

pn = PlotNordic(log_lvl=log_lvl_number)
pn.read_data_from_files(args.event_csv, args.event_descr)
pn.plot_events_from_file()
pn.log_stats('log')
