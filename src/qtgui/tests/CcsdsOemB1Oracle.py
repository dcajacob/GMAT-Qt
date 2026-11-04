"""Independent CCSDS 502.0-B-1 (September 2004), section 4, KVN checks.

This checks serialized records, never GMAT's reader/writer or integrator cache.
It is a format/interpolation-support oracle, not a trajectory or frame oracle.
Unknown ICD conventions and interpolation algorithms remain explicit gaps.
Source: https://ccsds.org/Pubs/502x0b1s.pdf, Tables 4-1..3, sections 4.3.1..7.
"""
import argparse
import datetime as dt
from decimal import Decimal, InvalidOperation, localcontext
import hashlib
import json
import math
from pathlib import Path
import re


HEADER = ('CCSDS_OEM_VERS', 'CREATION_DATE', 'ORIGINATOR')
META = ('OBJECT_NAME', 'OBJECT_ID', 'CENTER_NAME', 'REF_FRAME', 'TIME_SYSTEM',
        'START_TIME', 'USEABLE_START_TIME', 'USEABLE_STOP_TIME', 'STOP_TIME',
        'INTERPOLATION', 'INTERPOLATION_DEGREE')
REQUIRED = set(META[:6]) | {'STOP_TIME'}
CALENDAR = re.compile(r'(\d{4})-(?:(\d{2})-(\d{2})|(\d{3}))T'
                      r'(\d{2}):(\d{2}):(\d{2})(\.\d+)?\Z')
DESIGNATOR = re.compile(r'\d{4}-\d{3}[A-Z]{1,3}\Z')
FIXED = re.compile(r'[+-]?\d+(?:\.\d+)?\Z')
FLOAT = re.compile(r'[+-]?\d\.\d+[Ee][+-]?\d+\Z')
INTEGER = re.compile(r'[+-]?\d+\Z')
STANDARD_TIMES = {'UTC', 'TAI', 'TT', 'GPS', 'TDB', 'TCB'}
# Published positive UTC leap dates through 2016. Future/unlisted dates require
# a reviewed time profile; their absence must not silently yield full coverage.
UTC_LEAP_DAYS = set('1972-06-30 1972-12-31 1973-12-31 1974-12-31 1975-12-31 '
                    '1976-12-31 1977-12-31 1978-12-31 1979-12-31 1981-06-30 '
                    '1982-06-30 1983-06-30 1985-06-30 1987-12-31 1989-12-31 '
                    '1990-12-31 1992-06-30 1993-06-30 1994-06-30 1995-12-31 '
                    '1997-06-30 1998-12-31 2005-12-31 2008-12-31 2012-06-30 '
                    '2015-06-30 2016-12-31'.split())


class ConformanceError(ValueError):
    def __init__(self, clause, line, message):
        self.clause, self.line, self.message = clause, line, message
        super().__init__(f'{clause}, line {line}: {message}')


def need(ok, clause, line, message):
    if not ok:
        raise ConformanceError(clause, line, message)


def text(value):
    return ' '.join(value.replace('_', ' ').upper().split())


def integer(value, line):
    need(bool(INTEGER.fullmatch(value)), '4.3.3(c)', line, 'Expected integer')
    number = int(value)
    need(-2147483648 <= number <= 2147483647, '4.3.3(c)', line,
         'Integer outside signed 32-bit range')
    return number


def number(value, line):
    """Check the original standard's lexical precision as well as finiteness."""
    if 'e' in value.lower():
        need(bool(FLOAT.fullmatch(value)), '4.3.3(f)', line,
             'Floating value needs one integer mantissa digit and E/e exponent')
        mantissa, exponent = re.split('[Ee]', value)
        need(sum(c.isdigit() for c in mantissa) <= 16, '4.3.3(f)', line,
             'Mantissa exceeds 16 digits')
        integer(exponent, line)
    else:
        need(bool(FIXED.fullmatch(value)), '4.3.3(e)', line,
             'Invalid fixed point value or missing leading zero')
        need(sum(c.isdigit() for c in value) <= 18, '4.3.3(e)', line,
             'Fixed point value exceeds 18 digits')
    try:
        decimal, floating = Decimal(value), float(value)
    except (ValueError, InvalidOperation, OverflowError):
        raise ConformanceError('4.3.3(f)', line, 'Non-finite/unrepresentable value')
    need(decimal.is_finite() and math.isfinite(floating)
         and (decimal == 0 or floating != 0), '4.3.3(f)', line,
         'Non-finite, overflowed or underflowed double')
    return floating


def epoch(value, line, time_system, gaps, creation=False):
    """Return an exact (civil day, seconds) ordering key; never round to ms.

    Civil-day tuples preserve the ordering of UTC 23:59:60 and next-day zero.
    No absolute UTC/TAI conversion, leap-day duration inference or dynamics
    validation is performed. Julian strings use the standard noon origin.
    """
    match = CALENDAR.fullmatch(value)
    if not match:
        need(not creation and bool(FIXED.fullmatch(value)), 'Table 4-2/4-3',
             line, 'Expected ISO calendar/ordinal epoch or Julian date')
        with localcontext() as context:
            context.prec = max(40, len(value) + 20)
            days = Decimal(value) - Decimal('1721424.5')
            day = int(days // 1)
            need(1 <= day <= dt.date.max.toordinal(), 'Table 4-3', line,
                 'Julian date outside supported civil calendar')
            return day, (days - day) * 86400
    year, month, day, ordinal, hour, minute, second, fraction = match.groups()
    try:
        if ordinal:
            date = dt.date(int(year), 1, 1) + dt.timedelta(days=int(ordinal) - 1)
            need(int(ordinal) > 0 and date.year == int(year), 'Table 4-2/4-3',
                 line, 'Ordinal date outside year')
        else:
            date = dt.date(int(year), int(month), int(day))
    except (ValueError, OverflowError):
        raise ConformanceError('Table 4-2/4-3', line, 'Invalid calendar date')
    h, m, s = int(hour), int(minute), int(second)
    need(h < 24 and m < 60 and s <= 60, 'Table 4-2/4-3', line,
         'Invalid time of day')
    if s == 60:
        need(time_system == 'UTC' and (h, m) == (23, 59), 'Table 4-2/4-3',
             line, 'Leap second outside UTC day end')
        if date.isoformat() not in UTC_LEAP_DAYS:
            gaps.add('Unreviewed UTC leap date: ' + date.isoformat())
    with localcontext() as context:
        context.prec = max(40, len(value) + 20)
        return date.toordinal(), Decimal(h * 3600 + m * 60 + s) + Decimal(fraction or '0')


def validate(raw, *, icd_object_ids=()):
    """Raise for demonstrated violations; keep unverified conventions as gaps.

    ICD object IDs must be explicitly supplied by the caller, never guessed
    from a nonconforming file. Passing syntax alone awards no NASA row credit.
    """
    need(isinstance(raw, bytes) and bool(raw), '4.1', 1, 'Empty/non-byte OEM')
    need(raw.endswith((b'\r', b'\n')), '4.3.1(e)', 1, 'Unterminated final line')
    lines = re.split(rb'\r\n|\n\r|\r|\n', raw)[:-1]
    for at, line in enumerate(lines, 1):
        need(len(line) <= 254, '4.3.1(a)', at, 'Line exceeds 254 characters')
        need(all(32 <= char <= 126 for char in line), '4.3.1(b)', at,
             'Non-printable ASCII or control character')
    header, segments, gaps = {}, [], set()
    metadata, rows, meta_lines = None, [], {}
    in_meta, comment_after_data, comment_count = False, False, 0
    time_system, object_id = None, None
    allowed_ids = {text(value) for value in icd_object_ids}

    def finish(line):
        nonlocal rows
        if metadata is None:
            return
        need(not in_meta, '4.3.7(g)', line, 'Missing META_STOP')
        need(bool(rows), '4.2/Table 4-1', line, 'Empty ephemeris data block')
        system = text(metadata['TIME_SYSTEM'])
        start = epoch(metadata['START_TIME'], meta_lines['START_TIME'], system, gaps)
        stop = epoch(metadata['STOP_TIME'], meta_lines['STOP_TIME'], system, gaps)
        need(start == rows[0][0] and stop == rows[-1][0], 'Table 4-3', line,
             'START/STOP_TIME do not bound the actual data')
        lower = epoch(metadata.get('USEABLE_START_TIME', metadata['START_TIME']),
                      meta_lines.get('USEABLE_START_TIME', line), system, gaps)
        upper = epoch(metadata.get('USEABLE_STOP_TIME', metadata['STOP_TIME']),
                      meta_lines.get('USEABLE_STOP_TIME', line), system, gaps)
        need(start <= lower <= upper <= stop, 'Table 4-3', line,
             'Usable interval outside total span or reversed')
        if segments:
            need(start >= segments[-1]['stop_key'], '4.3.5(c)/Table 4-3', line,
                 'Overlapping/reversed metadata segments')
        degree = None
        if 'INTERPOLATION_DEGREE' in metadata:
            degree = integer(metadata['INTERPOLATION_DEGREE'],
                             meta_lines['INTERPOLATION_DEGREE'])
        if 'INTERPOLATION' in metadata:
            need(degree is not None, 'Table 4-3', line,
                 'Interpolation method requires degree')
            method = text(metadata['INTERPOLATION'])
            if method == 'LAGRANGE':
                need(degree >= 0 and len(rows) >= degree + 1, '4.3.5(f)', line,
                     f'Degree {degree} Lagrange needs {degree + 1} records; got {len(rows)}')
            elif method == 'LINEAR':
                need(degree == 1 and len(rows) >= 2, '4.3.5(f)', line,
                     'Linear interpolation needs degree 1 and two records')
            else:
                gaps.add('Interpolation support not proved: ' + method)
        else:
            gaps.add('No recommended interpolation method supplied')
        segments.append(dict(metadata=dict(metadata), records=len(rows),
                             start_key=start, stop_key=stop, degree=degree))
        rows = []

    for at, raw_line in enumerate(lines, 1):
        line = raw_line.decode('ascii').strip(' ')
        if not line:
            continue
        if line == 'COMMENT' or line.startswith('COMMENT '):
            need(bool(header), '4.3.7(c)', at, 'Comment precedes version header')
            need(raw_line.lstrip(b' ').startswith(b'COMMENT '), '4.3.6', at,
                 'COMMENT keyword requires a separating space')
            # A terminal comment is allowed. Another data row after it is not.
            if rows:
                comment_after_data = True
            comment_count += 1
            continue
        if line == 'META_START':
            need(tuple(header) == HEADER, 'Table 4-2', at, 'Incomplete header')
            finish(at)
            metadata, meta_lines, rows = {}, {}, []
            in_meta, comment_after_data = True, False
            continue
        if line == 'META_STOP':
            need(in_meta and REQUIRED <= metadata.keys(), 'Table 4-3', at,
                 'Incomplete or misplaced metadata')
            system, identity = text(metadata['TIME_SYSTEM']), text(metadata['OBJECT_ID'])
            need(time_system is None or system == time_system, '4.3.5(d)', at,
                 'TIME_SYSTEM changed between blocks')
            need(object_id is None or identity == object_id, '4.1', at,
                 'More than one object in OEM')
            time_system, object_id = system, identity
            if system not in STANDARD_TIMES:
                gaps.add('Unreviewed ICD time system: ' + system)
            if not DESIGNATOR.fullmatch(identity) and identity not in allowed_ids:
                gaps.add('Non-COSPAR object ID needs an ICD: ' + identity)
            in_meta = False
            continue
        if '=' in line:
            need(line.count('=') == 1, '4.3.2(b)', at, 'Multiple assignments')
            key, value = (part.strip(' ') for part in line.split('=', 1))
            need(bool(value), '4.3.3(h)', at, 'Null keyword value')
            need(bool(text(value)), '4.3.3(a,h)', at, 'Blank text keyword value')
            if metadata is None:
                need(len(header) < len(HEADER) and key == HEADER[len(header)],
                     '4.3.2(g)/Table 4-2', at, 'Unknown, duplicate or out-of-order header')
                header[key] = value
                if key == 'CCSDS_OEM_VERS':
                    need(value == '1.0', '4.3.7(a)', at, 'Expected OEM B-1 version 1.0')
                elif key == 'CREATION_DATE':
                    epoch(value, at, 'UTC', gaps, creation=True)
            else:
                need(in_meta and key in META and key not in metadata,
                     '4.3.2/4.3.7(e)', at, 'Unknown, duplicate or misplaced metadata')
                need(not metadata or META.index(key) > META.index(next(reversed(metadata))),
                     '4.3.2(g)', at, 'Metadata assignments out of order')
                metadata[key], meta_lines[key] = value, at
            continue
        need(metadata is not None and not in_meta, '4.3.7(f-g)', at,
             'Ephemeris data outside a metadata/data block')
        need(not comment_after_data, '4.3.1(d)', at, 'Comment between data records')
        parts = line.split(' ')
        parts = [part for part in parts if part]
        need(len(parts) == 7, '4.3.4/4.3.5(a-b)', at,
             'Data must contain epoch and six unitless Cartesian values')
        stamp = epoch(parts[0], at, time_system, gaps)
        need(not rows or stamp > rows[-1][0], '4.3.5(c)', at,
             'Duplicate or decreasing data epoch')
        rows.append((stamp, [number(value, at) for value in parts[1:]]))
    need(tuple(header) == HEADER and metadata is not None, '4.2/Table 4-1',
         len(lines), 'Missing header or metadata/data block')
    finish(len(lines))
    # Expose exact keys as strings so report writing never rounds timestamps.
    for segment in segments:
        for key in ('start_key', 'stop_key'):
            day, second = segment.pop(key)
            segment[key.replace('_key', '')] = [day, str(second)]
    return dict(status='passed' if not gaps else 'incomplete', standard='CCSDS 502.0-B-1',
                sha256=hashlib.sha256(raw).hexdigest(), header=header,
                segments=segments, records=sum(s['records'] for s in segments),
                comments=comment_count, gaps=sorted(gaps),
                scope='KVN structure, syntax, chronology and supported interpolation record counts; '
                      'not trajectory accuracy, frame orientation, physical units provenance, '
                      'creation-clock accuracy or complete NASA qualification')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('file', type=Path)
    parser.add_argument('--icd-object-id', action='append', default=[])
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    raw = args.file.read_bytes()
    try:
        report = validate(raw, icd_object_ids=args.icd_object_id)
    except ConformanceError as error:
        report = dict(status='failed', standard='CCSDS 502.0-B-1',
                      sha256=hashlib.sha256(raw).hexdigest(), clause=error.clause,
                      line=error.line, message=error.message)
    rendered = json.dumps(report, indent=2) + '\n'
    if args.output:
        args.output.write_text(rendered)
    print(rendered, end='')
    return 0 if report['status'] == 'passed' else 1


if __name__ == '__main__':
    raise SystemExit(main())
