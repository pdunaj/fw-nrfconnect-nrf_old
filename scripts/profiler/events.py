import csv
import json
import hashlib


class Event():
    def __init__(self, type_id, timestamp, data):
        self.type_id = type_id
        self.timestamp = timestamp
        self.data = data

    def __str__(self):
        return "ID: {} Timestamp: {} Data: {}".format(self.type_id, self.timestamp, self.data)


class EventType():
    def __init__(self, name, data_types, data_descriptions):
        self.name = name
        self.data_types = data_types
        self.data_descriptions = data_descriptions

    def __str__(self):
        return "Name: {} Data: {}".format(self.name, ",".join([" ".join([i, j]) for i,j in zip(self.data_types, self.data_descriptions)]))

    def to_json(self):
        return {"name": self.name, "data_types": self.data_types,
                "data_descriptions": self.data_descriptions}

    def from_json(json):
        return EventType(
            json["name"],
            json["data_types"],
            json["data_descriptions"])


class EventsData():
    def __init__(self, events, registered_events_types):
        self.events = events
        self.registered_events_types = registered_events_types

    def verify(self):
        for ev in self.events:
            if ev.type_id not in self.registered_events_types:
                return False
        return True

    def write_data_to_files(self, filename_events, filename_event_types):
        self._write_events_csv(filename_events)
        csv_hash = EventsData._calculate_md5_hash_of_file(filename_events)
        self._write_events_types_json(filename_event_types, csv_hash)

    def read_data_from_files(self, filename_events, filename_event_types):
        self._read_events_csv(filename_events)
        csv_hash1 = self._read_events_types_json(filename_event_types)
        csv_hash2 = EventsData._calculate_md5_hash_of_file(filename_events)
        if csv_hash1 != csv_hash2:
            print("Hash values of csv files do not match")
            print("Events and descriptions may be inconsistent")

    def _calculate_md5_hash_of_file(filename):
        return hashlib.md5(open(filename, 'rb').read()).hexdigest()

    def _write_events_csv(self, filename):
        with open(filename, 'w', newline='') as csvfile:
            fieldnames = ['type_id', 'timestamp', 'data']
            wr = csv.DictWriter(csvfile, delimiter=',', fieldnames=fieldnames)
            wr.writeheader()
            for ev in self.events:
                wr.writerow({'type_id': ev.type_id, 'timestamp': ev.timestamp,
                             'data': ev.data})

    def _read_events_csv(self, filename):
        with open(filename, 'r', newline='') as csvfile:
            rd = csv.DictReader(csvfile, delimiter=',')
            for row in rd:
                type_id = int(row['type_id'])
                timestamp = float(row['timestamp'])
                # reading event data from single row in csv file
                if (row['data'][1:-1] != ''):
                    data = list(map(int, (row['data'][1:-1].split(','))))
                else:
                    data = []
                ev = Event(type_id, timestamp, data)
                self.events.append(ev)

    def _write_events_types_json(self, filename, csv_hash):
        d = dict((k, v.to_json())
                 for k, v in self.registered_events_types.items())
        d['csv_hash'] = csv_hash
        with open(filename, "w") as wr:
            json.dump(d, wr, indent=4)

    def _read_events_types_json(self, filename):
        with open(filename, "r") as rd:
            data = json.load(rd)
        csv_hash = data['csv_hash']
        del data['csv_hash']
        self.registered_events_types = dict((int(k), EventType.from_json(v))
                                            for k, v in data.items())
        return csv_hash
