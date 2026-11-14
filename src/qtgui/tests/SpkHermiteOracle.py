"""Independent type-13 DAF reader and Hermite basis evaluator.

Implements the mathematical definition in NAIF SPK Required Reading, types
12/13. No SPICE, GMAT, NumPy, native interpolation or orbit library is used.
Agreement checks the curve encoded in a file; it cannot certify that curve's
physical trajectory or repair its time tags.
"""
from bisect import bisect_left, bisect_right
import math
import struct


def require(value, message):
    if not value:
        raise ValueError(message)


def read_segments(raw):
    require(len(raw) >= 1024 and len(raw) % 1024 == 0 and raw[:8] == b'DAF/SPK ',
            'Invalid DAF/SPK header or record length')
    order = {b'LTL-IEEE': '<', b'BIG-IEEE': '>'}.get(raw[88:96])
    require(order is not None, 'Unsupported DAF encoding')
    nd, ni = struct.unpack_from(order+'2i', raw, 8)
    require((nd, ni) == (2, 6), 'Unsupported SPK summary layout')
    record, backward, free = struct.unpack_from(order+'3i', raw, 76)
    require(0 < record <= len(raw)//1024 and 1 < free <= len(raw)//8+1,
            'Invalid DAF summary/free pointer')
    result, seen, previous = [], set(), 0
    while record:
        require(0 < record <= len(raw)//1024 and record not in seen, 'Invalid/cyclic DAF summary chain')
        seen.add(record)
        offset = (record-1)*1024
        nxt, prev, count = struct.unpack_from(order+'3d', raw, offset)
        require(all(math.isfinite(v) and v == int(v) for v in [nxt, prev, count]) and
                prev == previous and 0 <= count <= 25, 'Invalid DAF summary links/count')
        for i in range(int(count)):
            start = offset+24+40*i
            first, last = struct.unpack_from(order+'2d', raw, start)
            target, center, frame, kind, address, end = struct.unpack_from(order+'6i', raw, start+16)
            require(kind == 13, 'Only SPK type13 is implemented')
            require(math.isfinite(first) and math.isfinite(last) and first <= last and
                    1 <= address <= end < free, 'Invalid SPK descriptor/data addresses')
            words = struct.unpack_from(order+str(end-address+1)+'d', raw, (address-1)*8)
            require(len(words) >= 9 and all(math.isfinite(v) for v in words), 'Invalid SPK data words')
            window_minus_one, count_states = words[-2:]
            require(count_states == int(count_states) and window_minus_one == int(window_minus_one),
                    'Noninteger SPK count/window')
            n, window = int(count_states), int(window_minus_one)+1
            require(1 <= window <= n and len(words) == 7*n+(n-1)//100+2,
                    'SPK state/window count does not match its record')
            states = [list(words[6*i:6*i+6]) for i in range(n)]
            epochs = list(words[6*n:7*n])
            require(all(b > a for a, b in zip(epochs, epochs[1:])) and
                    first == epochs[0] and last == epochs[-1], 'SPK knot/descriptor coverage differs')
            require(list(words[7*n:-2]) == [epochs[100*(i+1)-1] for i in range((n-1)//100)],
                    'SPK epoch directory differs from knots')
            result.append(dict(target=target, center=center, frame=frame, degree=2*window-1,
                               window=window, first=first, last=last, epochs=epochs, states=states))
        previous, record = record, int(nxt)
    require(previous == backward and result, 'DAF backward pointer or empty SPK differs')
    return result


def window_start(epochs, size, request):
    """NAIF centered-window rule, clipped at the segment boundaries."""
    require(1 <= size <= len(epochs) and math.isfinite(request) and
            epochs[0] <= request <= epochs[-1], 'Request outside segment or invalid window')
    lower = max(0, bisect_right(epochs, request)-1)
    if size % 2 == 0:
        first = lower-size//2+1
    else:
        nearest = lower
        if lower+1 < len(epochs) and epochs[lower+1]-request < request-epochs[lower]:
            nearest += 1
        first = nearest-size//2
    return max(0, min(len(epochs)-size, first))


def hermite(epochs, states, request):
    """Value and first derivative from squared Lagrange cardinal bases.

    Normalize around the query to avoid multiplying large absolute ET values.
    The basis satisfies every stored position and derivative exactly.
    """
    require(len(epochs) == len(states) and epochs and math.isfinite(request) and
            all(math.isfinite(t) for t in epochs) and
            all(b > a for a, b in zip(epochs, epochs[1:])) and
            all(len(s) == 6 and all(map(math.isfinite, s)) for s in states),
            'Invalid Hermite input')
    exact = bisect_left(epochs, request)
    if exact < len(epochs) and epochs[exact] == request:
        return list(states[exact])
    scale = max(epochs[-1]-epochs[0], 1.)
    nodes = [(t-request)/scale for t in epochs]
    value_terms, derivative_terms = [[] for _ in range(3)], [[] for _ in range(3)]
    for i, node in enumerate(nodes):
        others = [j for j in range(len(nodes)) if j != i]
        lagrange = math.prod(-nodes[j]/(node-nodes[j]) for j in others)
        lagrange_derivative = math.fsum(
            math.prod(-nodes[k]/(node-nodes[k]) for k in others if k != j)/(node-nodes[j])
            for j in others)
        at_node_derivative = math.fsum(1/(node-nodes[j]) for j in others)
        delta = -node
        base = lagrange*lagrange
        base_derivative = 2*lagrange*lagrange_derivative
        h = (1-2*delta*at_node_derivative)*base
        hprime = -2*at_node_derivative*base+(1-2*delta*at_node_derivative)*base_derivative
        k = delta*base
        kprime = base+delta*base_derivative
        for axis in range(3):
            position, velocity = states[i][axis], states[i][axis+3]
            value_terms[axis].append(h*position+k*velocity*scale)
            derivative_terms[axis].append(hprime*position/scale+kprime*velocity)
    return [math.fsum(v) for v in value_terms]+[math.fsum(v) for v in derivative_terms]


def evaluate(segments, request, target=-100056, center=399, frame=1):
    require(math.isfinite(request), 'Nonfinite query epoch')
    # Later applicable segments in the file have priority, including shared
    # endpoints. This helper handles one file and no chained-center transforms.
    segment = next((s for s in reversed(segments) if s['target'] == target and
                    s['center'] == center and s['frame'] == frame and
                    s['first'] <= request <= s['last']), None)
    require(segment is not None, 'No matching SPK segment covers the request')
    first = window_start(segment['epochs'], segment['window'], request)
    last = first+segment['window']
    return hermite(segment['epochs'][first:last], segment['states'][first:last], request)


def read_queries(path):
    """Accept the GUI driver's decoded JSON, an audited query JSON, or CSV."""
    import csv
    import json
    from pathlib import Path
    path = Path(path)
    if path.suffix == '.json':
        data = json.loads(path.read_text())
        values = data.get('reader_samples', data.get('queries'))
        require(isinstance(values, list), 'Missing reader samples')
        return [[q['et']]+q['state'] for q in values]
    with path.open(newline='') as stream:
        rows = csv.reader(stream)
        require(next(rows, None) == ['et', 'x', 'y', 'z', 'vx', 'vy', 'vz'], 'Invalid query CSV header')
        return [list(map(float, row)) for row in rows]


def compare(segments, queries, target=-100056, center=399, frame=1,
            position_tolerance=1e-8, velocity_tolerance=1e-11):
    require(queries and position_tolerance > 0 and velocity_tolerance > 0,
            'Missing queries or invalid comparison bounds')
    maximum = [0.]*6
    for query in queries:
        require(len(query) == 7 and all(map(math.isfinite, query)), 'Invalid reader sample')
        expected = evaluate(segments, query[0], target, center, frame)
        for axis, (actual, wanted) in enumerate(zip(query[1:], expected)):
            error = abs(actual-wanted)
            require(error < (position_tolerance if axis < 3 else velocity_tolerance),
                    'Reader differs from independent encoded Hermite curve')
            maximum[axis] = max(maximum[axis], error)
    return dict(queries=len(queries), components=6*len(queries), segments=len(segments),
                component_max_errors=maximum, max_position_error_km=max(maximum[:3]),
                max_velocity_error_km_s=max(maximum[3:]),
                position_tolerance_km=position_tolerance, velocity_tolerance_km_s=velocity_tolerance,
                validates_encoded_curve_only=True, physical_trajectory_qualified=False)


if __name__ == '__main__':
    import argparse
    import json
    from pathlib import Path
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--kernel', type=Path, required=True)
    parser.add_argument('--samples', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--target', type=int, default=-100056)
    parser.add_argument('--center', type=int, default=399)
    parser.add_argument('--frame', type=int, default=1)
    args = parser.parse_args()
    result = compare(read_segments(args.kernel.read_bytes()), read_queries(args.samples),
                     args.target, args.center, args.frame)
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result))
