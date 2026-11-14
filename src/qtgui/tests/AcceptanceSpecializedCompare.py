"""Reusable CCSDS-TDM record and SPK sampled-state truth comparisons.

TDM1.0/2.0 KVN/XML are compared without time-scale or unit conversion. All
semantic metadata and every record must agree. SPK uses a fresh CSPICE process
per file, explicit observer/frame and complete object/coverage identities. The
finite sampling grid is reported; it is not a continuous-error bound.
"""
import argparse
import ctypes as C
import datetime as dt
import hashlib
import json
import math
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

import AcceptanceCompare as base
from AcceptanceCompare import need, decimal, numeric_cell, mismatch, compare_metadata

UNITS = {'RANGE': None, 'DOPPLER_INSTANTANEOUS': 'km/s', 'DOPPLER_INTEGRATED': 'km/s',
         'DOPPLER_COUNT': 'n/a', 'ANGLE_1': 'deg', 'ANGLE_2': 'deg',
         'RECEIVE_FREQ': 'Hz', 'CLOCK_BIAS': 's', 'CLOCK_DRIFT': 's/s',
         'DOR': 's', 'VLBI_DELAY': 's', 'PC_N0': 'dBHz', 'PR_N0': 'dBHz',
         'TROPO_DRY': 'm', 'TROPO_WET': 'm', 'STEC': 'TECU',
         'PRESSURE': 'hPa', 'RHUMIDITY': '%', 'TEMPERATURE': 'K', 'MAG': 'n/a', 'RCS': 'm^2', 'CARRIER_POWER': 'dBW'}
SPK_DEFAULT = dict(library=str(Path(__file__).resolve().parents[3]/'application/bin/libGmatBase.so.R2026a'),
                   observer=0, frame='J2000', step=60.0, max_samples=10000)


def clock(label, system):
    """Exact clock-label identity, including distinct UTC leap-second labels."""
    match = re.fullmatch(r'(\d{4})-(\d{3})(T.*)', label)
    if match:
        year, day = int(match[1]), int(match[2]); date = dt.date(year, 1, 1)+dt.timedelta(days=day-1)
        need(day >= 1 and date.year == year, 'Invalid TDM day of year')
        label = date.isoformat()+match[3]
    return base.epoch(label, system)


def unit(kind, metadata):
    if kind in UNITS:
        value = metadata.get('RANGE_UNITS', 'km') if kind == 'RANGE' else UNITS[kind]
        need(kind != 'RANGE' or value in ('km', 's', 'RU'), 'Unsupported TDM range unit')
        return value
    if re.fullmatch(r'(RECEIVE|TRANSMIT)_FREQ(?:_[1-5])?', kind): return 'Hz'
    if re.fullmatch(r'TRANSMIT_FREQ_RATE_[1-5]', kind): return 'Hz/s'
    if re.fullmatch(r'(RECEIVE|TRANSMIT)_PHASE_CT_[1-5]', kind): return 'n/a'
    raise ValueError('Unsupported TDM observation: '+kind)


def put(target, key, value):
    need(re.fullmatch(r'[A-Z][A-Z0-9_]*', key) and key not in target and value, 'Duplicate/empty/invalid TDM keyword: '+key)
    target[key] = value


def tdm_xml(payload):
    need(b'<!DOCTYPE' not in payload.upper() and b'<!ENTITY' not in payload.upper(), 'TDM DTD/entities are unsupported')
    root = ET.fromstring(payload)
    for node in root.iter():
        node.tag = node.tag.rsplit('}', 1)[-1]
        need(node is root or not node.attrib or set(node.attrib) <= {'units'}, 'Unsupported TDM XML attribute')
    need(set(root.attrib) <= {'id','version','{http://www.w3.org/2001/XMLSchema-instance}schemaLocation','{http://www.w3.org/2001/XMLSchema-instance}noNamespaceSchemaLocation'}, 'Unsupported TDM root attributes')
    need(all(not e.attrib for e in root.iter() if e is not root and e.tag in ('header','body','segment','metadata','data','observation','EPOCH')), 'Unsupported TDM container attributes')
    need(root.tag == 'tdm' and root.get('version') in ('1.0', '2.0') and root.get('id') == 'CCSDS_TDM_VERS', 'Unsupported TDM XML root/version')
    need([e.tag for e in root] == ['header', 'body'], 'TDM XML requires one header and body')
    header = {'CCSDS_TDM_VERS': root.get('version')}; segments = []
    for node in root[0]:
        if node.tag != 'COMMENT':
            need(not list(node) and not node.attrib, 'Nested/attributed TDM header')
            put(header, node.tag, (node.text or '').strip())
    for segment in root[1]:
        need(segment.tag == 'segment' and [e.tag for e in segment] == ['metadata', 'data'], 'Invalid TDM XML segment')
        metadata, observations = {}, []
        for node in segment[0]:
            if node.tag != 'COMMENT':
                need(not list(node), 'Nested TDM metadata')
                put(metadata, node.tag, (node.text or '').strip())
                if node.get('units') is not None: put(metadata, node.tag+'__UNITS', node.get('units'))
        for node in segment[1]:
            if node.tag == 'COMMENT': continue
            need(node.tag == 'observation' and len(node) == 2 and node[0].tag == 'EPOCH' and not list(node[0]) and not list(node[1]), 'TDM observation requires epoch and one scalar value')
            observations.append((node[1].tag, (node[0].text or '').strip(), (node[1].text or '').strip(), node[1].get('units')))
        segments.append((metadata, observations))
    return header, segments


def tdm_kvn(payload):
    header, segments = {}, []; state = 'header'; metadata = observations = None
    for raw in payload.decode('utf-8-sig').splitlines():
        line = raw.strip()
        if not line or line == 'COMMENT' or line.startswith('COMMENT '): continue
        if line == 'META_START':
            need(state in ('header', 'end'), 'Misplaced TDM metadata start'); metadata = {}; observations = []; state = 'metadata'
        elif line == 'META_STOP':
            need(state == 'metadata', 'Misplaced TDM metadata stop'); state = 'before-data'
        elif line == 'DATA_START':
            need(state == 'before-data', 'Misplaced TDM data start'); state = 'data'
        elif line == 'DATA_STOP':
            need(state == 'data', 'Misplaced TDM data stop'); segments.append((metadata, observations)); state = 'end'
        else:
            need('=' in line, 'Invalid TDM KVN line'); key, value = map(str.strip, line.split('=', 1))
            if state in ('header', 'metadata'): put(header if state == 'header' else metadata, key, value)
            elif state == 'data':
                tokens = value.split(); need(len(tokens) == 2, 'TDM data requires epoch and scalar')
                observations.append((key, tokens[0], tokens[1], None))
            else: raise ValueError('Unexpected TDM line outside metadata/data')
    need(state == 'end', 'Incomplete TDM KVN structure')
    return header, segments


def parse_tdm(payload):
    header, segments = tdm_xml(payload) if payload.lstrip().startswith(b'<') else tdm_kvn(payload)
    need(header.get('CCSDS_TDM_VERS') in ('1.0', '2.0') and header.get('CREATION_DATE') and header.get('ORIGINATOR') and segments, 'Incomplete TDM header/segments')
    clock(header['CREATION_DATE'], 'UTC')
    result = []
    for metadata, observations in segments:
        system = metadata.get('TIME_SYSTEM'); need(system in ('UTC', 'TAI', 'TT', 'TDB', 'TCB', 'GPS'), 'Unsupported/missing TDM time system')
        need(metadata.get('PARTICIPANT_1') and observations, 'Missing TDM participant or observations')
        if 'START_TIME' in metadata: metadata['START_TIME'] = clock(metadata['START_TIME'], system)
        if 'STOP_TIME' in metadata: metadata['STOP_TIME'] = clock(metadata['STOP_TIME'], system)
        for field in ('PATH', 'PATH_1', 'PATH_2'):
            if field in metadata:
                nodes = metadata[field].split(','); need(nodes and all(re.fullmatch(r'[1-5]', n.strip()) and metadata.get('PARTICIPANT_'+n.strip()) for n in nodes), 'Invalid TDM path participant')
                metadata[field] = ','.join(n.strip() for n in nodes)
        rows, last = {}, {}
        for kind, label, text, explicit_unit in observations:
            units = unit(kind, metadata); stamp = clock(label, system); value = decimal(text)
            need(explicit_unit is None or explicit_unit == units, 'TDM observation units disagree with standard/metadata')
            need((kind, stamp) not in rows and (kind not in last or stamp > last[kind]), 'Duplicate/out-of-order TDM observation')
            need(('START_TIME' not in metadata or stamp >= metadata['START_TIME']) and ('STOP_TIME' not in metadata or stamp <= metadata['STOP_TIME']), 'TDM observation outside declared interval')
            if kind == 'DOPPLER_COUNT': need(value == value.to_integral_value() and value >= 0, 'Invalid TDM integer Doppler count')
            if kind == 'RANGE': metadata.setdefault('RANGE_UNITS', 'km')
            if kind.startswith('ANGLE_'): need(metadata.get('ANGLE_TYPE'), 'TDM angle type missing')
            rows[kind, stamp] = dict(value=value, unit=units, label=label); last[kind] = stamp
        result.append((metadata, rows))
    return header, result


def compare_tdm(actual, truth, report, policy):
    ah, aa = parse_tdm(actual); th, tt = parse_tdm(truth)
    report['format_scope'] = 'TDM1.0/2.0 KVN/XML scalar-record comparison, all supplied semantic metadata. No unit/time conversion, full XSD validation or leap-table validation.'
    report['tdm_metadata'] = base.json_data(dict(actual=[s[0] for s in aa], truth=[s[0] for s in tt]))
    compare_metadata(ah, th, report, 'TDM header', ('CREATION_DATE', 'ORIGINATOR', 'MESSAGE_ID'))
    report['ignored_header_fields'] = ['CREATION_DATE', 'ORIGINATOR', 'MESSAGE_ID']
    if len(aa) != len(tt): mismatch(report, 'TDM', 'segment-count', len(aa), len(tt))
    for index, ((am, av), (tm, tv)) in enumerate(zip(aa, tt), 1):
        location = f'TDM segment {index}'
        if not compare_metadata(am, tm, report, location+' metadata'): continue
        for key in sorted(set(av)|set(tv)):
            kind, stamp = key; at = location+' '+kind+' '+str(stamp)
            if key not in av or key not in tv: mismatch(report, at, 'observation', av.get(key), tv.get(key)); continue
            report['comparisons'].append(numeric_cell(at, kind, av[key]['value'], tv[key]['value'], policy, tv[key]['unit']))


class Cell(C.Structure):
    _fields_ = [(x, C.c_int) for x in ('dtype', 'length', 'size', 'card', 'isSet', 'adjust', 'init')]+[('base', C.c_void_p), ('data', C.c_void_p)]

    def __init__(self, double, size):
        data_type = C.c_double if double else C.c_int
        self.storage = (data_type*(size+6))()
        super().__init__(1 if double else 2, 0, size, 0, 1, 0, 0, C.addressof(self.storage), C.addressof(self.storage)+6*C.sizeof(data_type))


class Spice:
    """Worker-local CSPICE binding. No GMAT startup, sandbox or GUI is created."""
    def __init__(self, library):
        self.library = Path(library).resolve(); self.lib = C.CDLL(str(self.library))
        prototypes = {'erract_c': [C.c_char_p, C.c_int, C.c_char_p], 'errprt_c': [C.c_char_p, C.c_int, C.c_char_p],
                      'getmsg_c': [C.c_char_p, C.c_int, C.c_char_p], 'reset_c': [], 'failed_c': [],
                      'furnsh_c': [C.c_char_p], 'unload_c': [C.c_char_p], 'spkobj_c': [C.c_char_p, C.POINTER(Cell)],
                      'spkcov_c': [C.c_char_p, C.c_int, C.POINTER(Cell)],
                      'spkgeo_c': [C.c_int, C.c_double, C.c_char_p, C.c_int, C.POINTER(C.c_double), C.POINTER(C.c_double)]}
        for name, args in prototypes.items():
            fn = getattr(self.lib, name); fn.argtypes = args; fn.restype = C.c_int if name == 'failed_c' else None
        self.call('erract_c', b'SET', 0, b'RETURN'); self.call('errprt_c', b'SET', 0, b'NONE')

    def call(self, name, *args):
        getattr(self.lib, name)(*args)
        if self.lib.failed_c():
            msg = C.create_string_buffer(4096); self.lib.getmsg_c(b'LONG', len(msg), msg); self.lib.reset_c()
            raise ValueError('CSPICE '+name+': '+msg.value.decode(errors='replace'))

    def inventory(self, path):
        names = Cell(False, 4096); self.call('spkobj_c', str(path).encode(), C.byref(names))
        ids = list(C.cast(names.data, C.POINTER(C.c_int))[:names.card]); need(ids, 'SPK contains no ephemeris objects')
        coverage = {}
        for body in ids:
            window = Cell(True, 32768); self.call('spkcov_c', str(path).encode(), body, C.byref(window))
            values = list(C.cast(window.data, C.POINTER(C.c_double))[:window.card])
            need(values and len(values)%2 == 0 and all(math.isfinite(x) for x in values), 'Invalid SPK coverage window')
            coverage[str(body)] = [values[i:i+2] for i in range(0, len(values), 2)]
        return coverage


def worker(request):
    spice = Spice(request['library']); path = Path(request['path']); coverage = spice.inventory(path)
    states = []
    if request.get('grid'):
        spice.call('furnsh_c', str(path).encode())
        try:
            for body, seconds in request['grid']:
                state = (C.c_double*6)(); light = C.c_double()
                spice.call('spkgeo_c', body, seconds, request['frame'].encode(), request['observer'], state, C.byref(light))
                need(all(math.isfinite(x) for x in state), 'Nonfinite SPK state')
                states.append(list(state))
        finally: spice.call('unload_c', str(path).encode())
    images = {}
    for line in Path('/proc/self/maps').read_text().splitlines():
        parts = line.split()
        if len(parts) >= 6 and parts[-1].startswith('/'):
            p = Path(parts[-1])
            if p.exists() and (p == spice.library or 'libGmat' in p.name): images[str(p)] = hashlib.sha256(p.read_bytes()).hexdigest()
    return dict(coverage=coverage, states=states, loaded_dependencies=images)


def spk_process(path, options, grid=None):
    p = subprocess.run([sys.executable, str(Path(__file__).resolve()), '--worker'],
                       input=json.dumps(dict(options, path=str(path), grid=grid)), text=True, capture_output=True, timeout=60)
    need(p.returncode == 0, 'SPK reader failed: '+p.stderr[-2000:])
    result = json.loads(p.stdout); need('error' not in result, result.get('error', 'SPK read error'))
    return result


def compare_spk(actual, truth, report, policy, options=None):
    options = dict(SPK_DEFAULT if options is None else options)
    need(math.isfinite(options['step']) and options['step'] > 0 and 1 <= options['max_samples'] <= 100000 and options['frame'], 'Invalid SPK sampling policy')
    report['spk_policy'] = dict(options, time_system='TDB', epoch_origin='J2000', position_unit='km', velocity_unit='km/s',
                               comparison='Geometric sampled states; not a continuous-error bound or byte/segment-layout comparison')
    with tempfile.TemporaryDirectory(prefix='gmat-spk-compare-') as directory:
        paths = [Path(directory)/name for name in ('actual.bsp', 'truth.bsp')]
        for path, data in zip(paths, (actual, truth)): path.write_bytes(data)
        inventories = [spk_process(p, options) for p in paths]
        left, right = [r['coverage'] for r in inventories]
        report['spk_coverage'] = dict(actual=left, truth=right)
        if left != right:
            mismatch(report, 'SPK', 'object IDs/coverage windows', left, right); return
        grid = []
        for body, windows in sorted(left.items(), key=lambda item: int(item[0])):
            for start, stop in windows:
                need(start <= stop, 'Invalid SPK interval')
                count = math.floor((stop-start)/options['step'])+1
                need(count <= options['max_samples']-len(grid), 'SPK sample limit exceeded; increase step or explicit limit')
                seconds = [start+i*options['step'] for i in range(count)]
                if seconds[-1] != stop: seconds.append(stop)
                need(len(grid)+len(seconds) <= options['max_samples'], 'SPK endpoint sample exceeds explicit limit')
                grid.extend((int(body), t) for t in seconds)
        results = [spk_process(p, options, grid) for p in paths]
        require_images = [v['loaded_dependencies'] for v in inventories+results]
        need(all(v == require_images[0] and v for v in require_images), 'SPK dependency changed during comparison')
        report['loaded_dependencies'] = require_images[0]
        report['spk_samples'] = [dict(object=body, tdb_seconds=seconds) for body, seconds in grid]
        for index, (body, seconds) in enumerate(grid):
            for j, field in enumerate(base.PV_FIELDS):
                report['comparisons'].append(numeric_cell(f'SPK body {body} TDB {seconds}', field,
                    results[0]['states'][index][j], results[1]['states'][index][j], policy, 'km' if j < 3 else 'km/s'))


def render(report):
    page = base.render_comparison(report)
    details = [(k, json.dumps(report[k], ensure_ascii=False)) for k in ('format_scope','tdm_metadata','spk_policy','spk_coverage','loaded_dependencies') if k in report]
    return page.replace('</main>', base.table('Comparison interpretation and limits', ['Field','Value'], details)+'</main>')


def register_comparators(registry):
    registry.register('tdm', compare_tdm); registry.register('spk', compare_spk)


def main():
    if sys.argv[1:] == ['--worker']:
        try: print(json.dumps(worker(json.load(sys.stdin)), allow_nan=False)); return 0
        except Exception as error: print(json.dumps(dict(error=str(error)))); return 0
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('actual', 'truth', 'json', 'html'): p.add_argument('--'+name, type=Path, required=name in ('actual', 'truth'))
    p.add_argument('--kind', choices=('tdm', 'spk'), required=True); p.add_argument('--absolute', default='0'); p.add_argument('--relative', default='0'); p.add_argument('--policy', type=Path)
    p.add_argument('--library', type=Path, default=Path(SPK_DEFAULT['library'])); p.add_argument('--observer', type=int, default=0); p.add_argument('--frame', default='J2000'); p.add_argument('--step', type=float, default=60); p.add_argument('--max-samples', type=int, default=10000)
    args = p.parse_args(); need(args.json or args.html, 'Select JSON and/or HTML output')
    options = dict(library=str(args.library.resolve()), observer=args.observer, frame=args.frame, step=args.step, max_samples=args.max_samples)
    registry = base.Registry(); registry.register('tdm', compare_tdm); registry.register('spk', lambda a,t,r,p: compare_spk(a,t,r,p,options))
    report = registry.compare(args.actual, args.truth, args.kind, json.loads(args.policy.read_text()) if args.policy else dict(absolute=args.absolute, relative=args.relative))
    protected = [args.actual, args.truth, args.library, Path(__file__), *([args.policy] if args.policy else [])]
    need(not (args.json and args.html and args.json.resolve() == args.html.resolve()), 'JSON and HTML destinations must differ')
    for path in (args.json, args.html):
        if path: base.validate_output(path, protected)
    if args.json: base.write_output(args.json, json.dumps(report, indent=2, allow_nan=False)+'\n', protected)
    if args.html: base.write_output(args.html, render(report), protected)
    return dict(passed=0, failed=1, error=2)[report['status']]


if __name__ == '__main__':
    try: raise SystemExit(main())
    except (ValueError, OSError) as error: print(str(error), file=sys.stderr); raise SystemExit(2)
