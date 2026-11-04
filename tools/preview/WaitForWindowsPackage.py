"""Wait for an already-running native Windows job; never waive a failed build."""
import json
import os
import re
import time
import urllib.request

run = os.environ['REUSE_RUN_ID']
repo = os.environ['GH_REPO']
assert re.fullmatch(r'[0-9]+', run)
assert re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', repo)
headers = {'Authorization': 'Bearer ' + os.environ['GH_TOKEN'],
           'Accept': 'application/vnd.github+json', 'User-Agent': 'GMAT-Qt-package-reuse'}


def get(suffix):
    request = urllib.request.Request('https://api.github.com/repos/' + repo + suffix,
                                     headers=headers)
    with urllib.request.urlopen(request, timeout=30) as response:
        return json.load(response)


deadline = time.monotonic() + 45 * 60
while time.monotonic() < deadline:
    artifacts = get('/actions/runs/' + run + '/artifacts?per_page=100')['artifacts']
    if any(a['name'] == 'package-windows-x64' and not a['expired'] for a in artifacts):
        print('Verified Windows package artifact is ready from run ' + run, flush=True)
        break
    jobs = get('/actions/runs/' + run + '/jobs?per_page=100')['jobs']
    windows = [j for j in jobs if 'windows-x64' in j['name']]
    assert len(windows) == 1, 'Expected exactly one native Windows producer'
    job = windows[0]
    if job['status'] == 'completed':
        raise RuntimeError('Windows producer completed without a verified package: ' + str(job['conclusion']))
    print('Waiting for existing Windows job ' + str(job['id']) + ': ' + job['status'], flush=True)
    time.sleep(30)
else:
    raise TimeoutError('Windows package producer did not finish within the bounded wait')
