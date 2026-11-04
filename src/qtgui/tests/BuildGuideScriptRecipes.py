#!/usr/bin/env python3
"""Build keyboard-input recipes from explicitly script-based Help chapters.

Reads DocBook program listings only. It never reads a completed mission script.
The resulting recipe still types every line through the GUI script editor.
"""
import hashlib
import json
import argparse
import re
from pathlib import Path
import xml.etree.ElementTree as ET

REPO = Path(__file__).resolve().parents[3]
DESTINATION = Path(__file__).with_name('tutorials')
NS = {'d': 'http://docbook.org/ns/docbook'}


def script_stage(name, code, filename, blocks, run=True):
    actions = [{'action': 'key', 'keys': 'ESC'}, {'action': 'key', 'keys': 'CTRL+N'},
               {'action': 'key', 'keys': 'CTRL+A', 'target_name': 'scriptEditor'}]
    for line in code.splitlines():
        if line:
            if not line.isascii():
                raise ValueError('Review non-ASCII guide code before recording keyboard input')
            actions.append({'action': 'text', 'text': line, 'target_name': 'scriptEditor'})
        actions.append({'action': 'key', 'keys': 'ENTER'})
    actions += [{'action': 'screenshot', 'label': 'guide-code-entered'},
                {'action': 'key', 'keys': 'CTRL+S'},
                {'action': 'key', 'keys': 'CTRL+A', 'target_name': 'fileNameEdit'},
                {'action': 'text', 'text': '${WORK}/'+filename, 'target_name': 'fileNameEdit'},
                {'action': 'key', 'keys': 'ENTER'}, {'action': 'key', 'keys': 'CTRL+SHIFT+F7'}]
    if run:
        actions.append({'action': 'key', 'keys': 'F5'})
    actions += [
                {'action': 'screenshot', 'label': 'guide-mission-run'},
                {'action': 'key', 'keys': 'CTRL+O'},
                {'action': 'key', 'keys': 'CTRL+A', 'target_name': 'fileNameEdit'},
                {'action': 'text', 'text': '${WORK}/'+filename, 'target_name': 'fileNameEdit'},
                {'action': 'key', 'keys': 'ENTER'},
                {'action': 'screenshot', 'label': 'own-saved-mission-reopened'}]
    return {'name': name, 'input': None, 'guide_programlistings': blocks, 'actions': actions, 'requires_run': run}


def chapter(recipe):
    root=ET.parse(REPO/recipe['guide']).getroot()
    return root, [''.join(node.itertext()) for node in root.findall('.//d:programlisting', NS)]


def join_blocks(listings, numbers):
    return '\n\n'.join(listings[number-1] for number in numbers)


def strip_mission(code):
    return code.split('BeginMissionSequence',1)[0]


def dsn_code(realistic=True, gn=False):
    recipe=json.loads(next(DESTINATION.glob('09-*.json')).read_text())
    root,listings=chapter(recipe)
    resources=join_blocks(listings,range(2,11))
    if realistic:
        resources+='\n\n'+join_blocks(listings,[13,14,15]+([] if gn else [16])+[17])
        resources=resources.replace("{'Sat_dsn_range_and_doppler_measurements.gmd'}", "{'Simulate DSN Range and Doppler Data 3 weeks.gmd'}")
        resources=resources.replace('../output/Simulate DSN Range and Doppler Data 3 weeks.rmp','${WORK}/Simulate DSN Range and Doppler Data 3 weeks.rmp')
    if gn:
        other=json.loads(next(DESTINATION.glob('10-*.json')).read_text());_,gnlist=chapter(other)
        resources=resources.replace("'DSN_SeqRange'","'Range'").replace("'DSN_TCP'","'RangeRate'")
        resources=re.sub(r'(?m)^\s*DSNsimData\.SimRangeModuloConstant[^\n]*','',resources)
        resources+='\n\n'+join_blocks(gnlist,[32,33])
    return resources+'\n\n'+listings[10]


def simulation(recipe):
    root,listings=chapter(recipe)
    short=script_stage('Guide short DSN simulation',dsn_code(False),'tutorial09-short.script',list(range(2,12)))
    ramp=next(''.join(node.itertext()) for node in root.findall('.//d:literallayout',NS) if '27252' in ''.join(node.itertext()))
    ramp_actions=[{'action':'key','keys':'ESC'}, {'action':'key','keys':'CTRL+SHIFT+N'},
                  {'action':'key','keys':'CTRL+A','target_name':'documentEditor'}]
    for line in ramp.splitlines():
        ramp_actions += [{'action':'text','target_name':'documentEditor','text':line.strip()},{'action':'key','keys':'ENTER'}]
    ramp_actions += [{'action':'key','keys':'CTRL+S'},
                     {'action':'key','keys':'CTRL+A','target_name':'fileNameEdit'},
                     {'action':'text','target_name':'fileNameEdit','text':'${WORK}/Simulate DSN Range and Doppler Data 3 weeks.rmp'},
                     {'action':'key','keys':'ENTER'}, {'action':'screenshot','label':'guide-ramp-data-created'},
                     {'action':'key','keys':'CTRL+W'}]
    realistic=script_stage('Guide three-station ramp and noisy simulation',dsn_code(),'tutorial09-realistic.script',list(range(2,12))+list(range(13,18)))
    realistic['actions']=ramp_actions+realistic['actions']
    return [short,realistic]


def estimation(recipe):
    _,listings=chapter(recipe)
    base=join_blocks(listings,range(2,13))
    base=base.replace('../output/Simulate DSN Range and Doppler Data 3 weeks.gmd','${WORK}/Simulate DSN Range and Doppler Data 3 weeks.gmd')
    base=base.replace('../output/Simulate DSN Range and Doppler Data 3 weeks.rmp','${WORK}/Simulate DSN Range and Doppler Data 3 weeks.rmp')
    nominal=script_stage('Guide DSN batch estimate with own observations',base+'\n\n'+listings[12],'tutorial10-dsn.script',list(range(2,14)))
    nominal['data_aliases']={'own-dsn-observations.gmd':'Simulate DSN Range and Doppler Data 3 weeks.gmd'}
    recipe['reference_input_aliases']={'Simulate DSN Range and Doppler Data 3 weeks.gmd':'own-dsn-observations.gmd'}
    gn_sim=script_stage('Guide GN radio and measurement re-simulation',dsn_code(gn=True),'tutorial10-gn-simulation.script',[32,33,34,35])
    gn_sim['data_aliases']={'own-gn-observations.gmd':'Simulate DSN Range and Doppler Data 3 weeks.gmd'}
    # Change the six configurations as instructed, rather than appending a
    # second set. Remove both lines of the commented-out ramp declaration.
    gn=base.replace("'DSN_SeqRange'","'Range'").replace("'DSN_TCP'","'RangeRate'")
    gn=re.sub(r'(?ms)^\s*DSNsimData\.RampTable\s*=.*?;\s*','',gn)
    gn+='\n\n'+join_blocks(listings,[36,37,39])
    gn=gn.replace('../output/Simulate DSN Range and Doppler Data 3 weeks.gmd','${WORK}/own-gn-observations.gmd')
    final=script_stage('Guide GN batch estimate with own GN observations',gn+'\n\n'+listings[12],'tutorial10-gn.script',[36,37,38,39,40])
    nominal['reference_milestone']=True
    recipe['guide_discrepancies']=[]
    recipe['not_exercised']=['Optional external MATLAB analysis; the generated MAT files are retained.']
    return [nominal,gn_sim,final]


def filter_smoother(recipe):
    _,listings=chapter(recipe)
    sim=strip_mission(listings[0])+'\n\n'+join_blocks(listings,range(2,9))
    simulation_stage=script_stage('Guide GPS position-vector simulation',sim,'tutorial11-simulation.script',list(range(1,9)))
    resources=strip_mission(listings[9])+'\n\n'+join_blocks(listings,range(11,18))
    resources=resources.replace("{'gps_posvec.gmd'}","{'${WORK}/gps_posvec.gmd'}")
    filtering=script_stage('Guide EKF with fresh own GPS observations',resources+'\n\n'+listings[17],'tutorial11-filter.script',list(range(10,19)))
    smoothing=script_stage('Guide forward-backward smoother',resources+'\n\n'+listings[21]+'\n\n'+listings[22],'tutorial11-smoother.script',[22,23])
    smoothing['reference_milestone']=True
    simulation_stage['reference_spacecraft']=['SimSat']
    smoothing['reference_spacecraft']=['EstSat']
    warm_resources=resources.replace(listings[13],listings[26])
    warm_resources=warm_resources.replace("InputWarmStartFile   = 'filter.csv'","InputWarmStartFile   = '${WORK}/filter.csv'")
    warm=script_stage('Guide filter warm start from own CSV',warm_resources+'\n\n'+listings[17],'tutorial11-warm.script',[27])
    ephem_resources=resources+'\n\n'+listings[21]+'\n\n'+listings[27]
    ephem=script_stage('Guide filter and smoother ephemerides',ephem_resources+'\n\n'+listings[28],'tutorial11-ephemeris.script',[28,29])
    predict=script_stage('Guide one-day smoother prediction',ephem_resources+'\n\n'+listings[29]+'\n\n'+listings[28],'tutorial11-prediction.script',[30])
    recipe['not_exercised']=['Optional Python plotting utilities and Windows gmat.exe appendix. Numerical report, MAT, CSV and ephemeris artifacts are retained.']
    return [simulation_stage,filtering,smoothing,warm,ephem,predict]


def lunar(recipe):
    _,listings=chapter(recipe)
    resources=join_blocks(listings,range(2,12))
    initial=listings[12]
    skeleton=listings[13]
    body=join_blocks(listings,[15,16,17,20,21,22,23,24])
    first=script_stage('Guide resources and initial optimization skeleton',resources+'\n\n'+initial+'\n\n'+skeleton,'tutorial05-resources.script',list(range(2,15)),run=False)
    nominal=skeleton.replace('EndOptimize',body+'\nEndOptimize')
    stopped=script_stage('Guide initial trajectory with explicit Stop',resources+'\n\n'+initial+'\n\n'+nominal,'tutorial05-initial.script',[15,16,17,20,21,22,23,24])
    stopped['accepted_run_statuses']=['Mission stopped']
    patch=re.sub(r'(?m)^\s*Stop\s*$', '',nominal)
    patch_stage=script_stage('Guide patch-point feasibility solve',resources+'\n\n'+initial+'\n\n'+patch,'tutorial05-patch.script',[24])
    full=re.sub(r'(?m)^(\s*)%\s*(NonlinearConstraint|Minimize|conMOIInclination)',r'\1\2',patch)
    complete=script_stage('Guide full constrained optimization',resources+'\n\n'+initial+'\n\n'+full,'tutorial05-full.script',[25])
    alternate=initial.replace('EndScript',listings[26]+'\nEndScript')
    alternative=script_stage('Guide provided alternative initial guess',resources+'\n\n'+alternate+'\n\n'+full,'tutorial05-alternative.script',[27])
    warm=initial.replace('EndScript',listings[27]+'\nEndScript')
    marker='NonlinearConstraint NLPOpt(satMOI_Backward.RadPer = conMOIPeriapsis)'
    if marker not in full: raise RuntimeError('Review the changed lunar initial constraints')
    constrained=full.replace(marker,marker+'\n'+listings[28],1)
    exercise=script_stage('Guide minimum lunar periapsis radius exercise',resources+'\n\n'+warm+'\n\n'+constrained,'tutorial05-radius.script',[28,29])
    recipe['guide_discrepancies']=[]
    return [first,stopped,patch_stage,complete,alternative,exercise]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tutorial',action='append',choices=['5','9','10','11','12'])
    args=parser.parse_args()
    for number in args.tutorial or ['12']:
        path=next(DESTINATION.glob(f'{int(number):02}-*.json'))
        recipe=json.loads(path.read_text())
        guide=REPO/recipe['guide']
        if hashlib.sha256(guide.read_bytes()).hexdigest()!=recipe['guide_sha256']:
            raise RuntimeError('Review the changed guide before regenerating its instructions')
        if number!='12':
            recipe['stages']={'5':lunar,'9':simulation,'10':estimation,'11':filter_smoother}[number](recipe)
            recipe['provenance']='Current script-based Help program listings entered through the GUI with QtTest keyboard events. No completed example mission is read to generate this recipe. Output/data paths are isolated to this case; dependencies are its own newly generated observations.'
            path.write_text(json.dumps(recipe,indent=2)+'\n')
            print(recipe['id'],len(recipe['stages']),'Guide code stages')
            continue
        build_interspacecraft()


def build_interspacecraft():
    guide = REPO / 'doc/help/src/Tut_Simulate_and_Estimate_Inter_Spacecraft_Tracking.xml'
    root = ET.parse(guide).getroot()
    listings = [''.join(node.itertext()) for node in root.findall('.//d:programlisting', NS)]
    blocks = list(range(2, 12))
    code = '\n\n'.join(listings[index-1] for index in blocks)
    recipe = {'id': '12-inter-spacecraft-tracking', 'title': ''.join(root.find('d:title', NS).itertext()),
              'guide': str(guide.relative_to(REPO)), 'guide_sha256': hashlib.sha256(guide.read_bytes()).hexdigest(),
              'reference': 'application/samples/Tut_Inter_Spacecraft_Tracking.script',
              'provenance': 'Literal program listings 2 through 11 of the current script-based Help chapter, entered by GUI keyboard events. No example mission is read to generate this recipe.',
              'stages': [script_stage('Written simulation and estimation code', code, 'tutorial12-authored.script', blocks)]}
    (DESTINATION / (recipe['id']+'.json')).write_text(json.dumps(recipe, indent=2)+'\n')


if __name__ == '__main__':
    main()
