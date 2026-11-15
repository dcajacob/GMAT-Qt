#!/usr/bin/env python3
"""Prepare the bundled map catalog from locally downloaded source archives.

Requires pyshp 2.3.1. Pass the archive directory and output directory.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path
import zipfile

import shapefile


def simplify(points, tolerance):
    if len(points) < 3:
        return points
    keep = {0, len(points) - 1}
    pending = [(0, len(points) - 1)]
    while pending:
        first, last = pending.pop()
        ax, ay = points[first]
        bx, by = points[last]
        dx, dy = bx - ax, by - ay
        length = dx * dx + dy * dy
        distance, index = tolerance * tolerance, None
        for candidate in range(first + 1, last):
            x, y = points[candidate]
            position = max(0, min(1, ((x - ax) * dx + (y - ay) * dy) / length)) if length else 0
            squared = (x - ax - position * dx) ** 2 + (y - ay - position * dy) ** 2
            if squared > distance:
                distance, index = squared, candidate
        if index is not None:
            keep.add(index)
            pending.extend(((first, index), (index, last)))
    return [points[index] for index in sorted(keep)]


def paths(shape):
    parts = list(shape.parts) + [len(shape.points)]
    result = []
    for first, last in zip(parts, parts[1:]):
        current = []
        for longitude, latitude in shape.points[first:last]:
            point = [round(longitude, 5), round(latitude, 5)]
            if current and abs(longitude - current[-1][0]) > 180:
                if len(current) > 1:
                    result.append(current)
                current = []
            current.append(point)
        if len(current) > 1:
            result.append(current)
    return result


def center(contours):
    best_area, best_center = 0, [0, 0]
    for contour in contours:
        area, longitude, latitude = 0, 0, 0
        for a, b in zip(contour, contour[1:]):
            cross = a[0] * b[1] - b[0] * a[1]
            area += cross
            longitude += (a[0] + b[0]) * cross
            latitude += (a[1] + b[1]) * cross
        if abs(area) > best_area:
            best_area = abs(area)
            best_center = [round(longitude / (3 * area), 5), round(latitude / (3 * area), 5)]
    return best_center


def records(path):
    with zipfile.ZipFile(path) as archive:
        with shapefile.Reader(shp=io.BytesIO(archive.read(path.stem + '.shp')),
                              dbf=io.BytesIO(archive.read(path.stem + '.dbf')),
                              encoding='utf-8') as reader:
            yield from reader.iterShapeRecords()


def write(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, separators=(',', ':'), allow_nan=False) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sources', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    sources = {
        'countries': ('ne_50m_admin_0_countries.zip', 'Natural Earth 5.1.1',
                      'https://naturalearth.s3.amazonaws.com/50m_cultural/ne_50m_admin_0_countries.zip'),
        'counties': ('cb_2025_us_county_5m.zip', 'US Census Bureau 2025, 1:5,000,000',
                    'https://www2.census.gov/geo/tiger/GENZ2025/shp/cb_2025_us_county_5m.zip'),
        'places': ('ne_10m_populated_places_simple.zip', 'Natural Earth 5.1.2',
                   'https://naturalearth.s3.amazonaws.com/10m_cultural/ne_10m_populated_places_simple.zip'),
    }
    manifest = {}
    for kind, (filename, version, url) in sources.items():
        archive = args.sources / filename
        features = []
        for record in records(archive):
            fields = record.record.as_dict()
            if kind == 'places':
                longitude, latitude = record.shape.points[0]
                features.append({'name': fields['name'], 'region': fields['adm1name'],
                                 'country': fields['adm0name'], 'ascii': fields['nameascii'],
                                 'point': [round(longitude, 5), round(latitude, 5)],
                                 'rank': fields['scalerank'], 'population': fields['pop_max']})
                continue
            contours = paths(record.shape)
            features.append({'name': fields['NAME'], 'point': center(contours),
                             'coarse': [simplify(contour, .05) for contour in contours],
                             'fine': [simplify(contour, .004) for contour in contours]})
        if kind == 'places':
            features.sort(key=lambda place: (place['rank'], -place['population'], place['name']))
        output = args.output / (kind + '.json')
        write(output, features)
        manifest[kind] = {'source': url, 'version': version, 'features': len(features),
                          'archive_sha256': hashlib.sha256(archive.read_bytes()).hexdigest(),
                          'prepared_sha256': hashlib.sha256(output.read_bytes()).hexdigest()}
    write(args.output / 'sources.json', manifest)
    print(json.dumps(manifest, indent=2))


if __name__ == '__main__':
    main()
