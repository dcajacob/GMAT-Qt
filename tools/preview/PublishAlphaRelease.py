"""Publish the qualified alpha.4 archives without rebuilding or replacing assets."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess


REPO = Path(__file__).resolve().parents[2]
METADATA = Path(__file__).with_name('releases') / '0.1.0-alpha.4'
ROOT = REPO / 'build/publish-alpha'
TAG = 'v0.1.0-alpha.4'


def gh(*arguments):
    return subprocess.check_output(['gh', *arguments], text=True)


def api(path):
    return json.loads(gh('api', path))


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def release_identity(release):
    fields = ['id', 'tag_name', 'target_commitish', 'name', 'body', 'draft',
              'prerelease', 'created_at', 'published_at']
    identity = {key: release[key] for key in fields}
    asset_fields = ['id', 'name', 'label', 'content_type', 'state', 'size',
                    'digest', 'created_at', 'updated_at']
    identity['assets'] = sorted(
        [{key: asset.get(key) for key in asset_fields} for asset in release['assets']],
        key=lambda asset: asset['id'])
    return identity


def check_assets(release, expected):
    actual = {asset['name']: asset for asset in release['assets']}
    assert set(actual) == set(expected), 'Release assets do not match the verified upload set'
    for name, details in expected.items():
        assert actual[name]['state'] == 'uploaded', name
        assert actual[name]['size'] == details['size'], name
        assert actual[name]['digest'] == 'sha256:' + details['sha256'], name


def main():
    repository = os.environ['GH_REPO']
    assert repository == 'dcajacob/GMAT-Qt'
    receipt = json.loads((METADATA / 'release-receipt.json').read_text())
    source = receipt['source_commit']
    assert receipt['status'] == 'passed' and receipt['version'] == '0.1.0-alpha.4'
    assert {package['platform'] for package in receipt['platforms']} == {
        'linux-x86_64', 'windows-x64', 'macos-arm64', 'macos-x64'}
    assert len(receipt['platforms']) == 4
    assert api(f'repos/{repository}/commits/{source}')['sha'] == source
    previous = api(f'repos/{repository}/releases?per_page=100')
    assert len(previous) < 100, 'Release inventory needs pagination'
    (ROOT / 'releases-before.json').write_text(json.dumps(previous, indent=2) + '\n')
    existing = next((release for release in previous if release['tag_name'] == TAG), None)
    assert existing is None or existing['draft'], 'Published alpha.4 already exists'
    if existing is not None:
        assert existing['target_commitish'] == source, 'Draft targets a different source'
    references = api(f'repos/{repository}/git/matching-refs/tags/{TAG}')
    for reference in references:
        if reference['ref'] == 'refs/tags/' + TAG:
            assert reference['object']['type'] == 'commit'
            assert reference['object']['sha'] == source, 'Existing tag has a different source'

    dist = ROOT / 'dist'
    dist.mkdir(exist_ok=True)
    assert not any(dist.iterdir()), 'Publication directory must start empty'
    checksums = []
    for package in receipt['platforms']:
        directory = ROOT / package['platform']
        name = Path(package['archive']).name
        archive = directory / name
        assert digest(archive) == package['sha256'], name
        assert archive.stat().st_size == package['bytes'], name
        native_path = directory / Path(package['receipt']).name
        assert digest(native_path) == package['receipt_sha256'], native_path
        native = json.loads(native_path.read_text())
        assert native['status'] == 'passed' and native['source_commit'] == source
        assert native['python']['source_commit'] == source
        assert native['python']['package_source_commit'] == source
        assert native['python']['python_version'] == '3.12'
        assert native['event_checks']['status'] == native['mcp']['status'] == 'passed'
        assert native['gui_without_python']['status'] == 'passed'
        checksum = directory / (name + '.sha256')
        assert checksum.read_text().split() == [package['sha256'], name]
        for path in [archive, checksum, native_path]:
            shutil.copy2(path, dist / path.name)
        checksums.append(package['sha256'] + '  ' + name)
    (dist / 'SHA256SUMS').write_text('\n'.join(checksums) + '\n')
    for name in ['release-receipt.json', 'reused-evidence.json', 'RELEASE-NOTES.md']:
        shutil.copy2(METADATA / name, dist / name)
    shutil.copy2(REPO / 'RELEASE-NOTES.rst', dist / 'RELEASE-NOTES.rst')
    expected = {path.name: {'size': path.stat().st_size, 'sha256': digest(path)}
                for path in sorted(dist.iterdir())}
    (ROOT / 'expected-assets.json').write_text(json.dumps(expected, indent=2) + '\n')

    if existing is None:
        gh('release', 'create', TAG, '--target', source, '--draft', '--prerelease',
           '--title', 'GMAT Qt 0.1.0-alpha.4', '--notes-file', str(METADATA / 'RELEASE-NOTES.md'),
           '--latest=false')
        existing = next(release for release in api(f'repos/{repository}/releases?per_page=100')
                        if release['tag_name'] == TAG)
    release_path = f'repos/{repository}/releases/{existing["id"]}'
    release = api(release_path)
    assert release['draft'] and release['prerelease']
    assert release['target_commitish'] == source
    present = {asset['name']: asset for asset in release['assets']}
    assert set(present).issubset(expected), 'Draft contains unexpected assets'
    for name, asset in present.items():
        assert asset['digest'] == 'sha256:' + expected[name]['sha256'], name
        assert asset['size'] == expected[name]['size'], name
    for name in expected:
        if name not in present:
            gh('api', '--method', 'POST',
               f'https://uploads.github.com/{release_path}/assets?name={name}',
               '--header', 'Content-Type: application/octet-stream', '--input', str(dist / name))
    release = api(release_path)
    check_assets(release, expected)
    publication = ROOT / 'publish-request.json'
    publication.write_text(json.dumps({
        'draft': False, 'prerelease': True, 'make_latest': 'false',
        'name': 'GMAT Qt 0.1.0-alpha.4',
        'body': (METADATA / 'RELEASE-NOTES.md').read_text()}))
    gh('api', '--method', 'PATCH', release_path, '--input', str(publication))
    release = api(release_path)
    assert release['prerelease'] and not release['draft']
    assert release['body'].strip() == (METADATA / 'RELEASE-NOTES.md').read_text().strip()
    check_assets(release, expected)
    tag = api(f'repos/{repository}/git/ref/tags/{TAG}')
    assert tag['object']['type'] == 'commit' and tag['object']['sha'] == source
    after = api(f'repos/{repository}/releases?per_page=100')
    by_id = {item['id']: item for item in after}
    for old in previous:
        if old['tag_name'] != TAG:
            assert release_identity(by_id[old['id']]) == release_identity(old), old['tag_name']
    result = {'status': 'published', 'url': release['html_url'], 'tag': TAG,
              'source_commit': source, 'assets': expected,
              'previous_releases_preserved': len([r for r in previous if r['tag_name'] != TAG]),
              'workflow_run': os.environ.get('GITHUB_RUN_ID'),
              'verification_workflow_run': receipt['workflow_run']}
    (ROOT / 'publication-receipt.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))


if __name__ == '__main__':
    main()
