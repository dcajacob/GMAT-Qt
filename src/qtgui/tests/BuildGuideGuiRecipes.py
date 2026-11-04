#!/usr/bin/env python3
"""Reviewed Help instructions expressed as semantic QtTest GUI operations.

Values below are transcribed from the named Help chapters. This generator never
reads example missions. No construction step sets engine parameters or pastes
mission scaffolding into the script editor.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path
import xml.etree.ElementTree as ET

REPO = Path(__file__).resolve().parents[3]
DEST = Path(__file__).with_name('tutorials')
NS = {'d': 'http://docbook.org/ns/docbook'}


class Walk:
    def __init__(self):
        self.actions = []

    def key(self, keys, **target):
        self.actions.append(dict(action='key', keys=keys, **target))

    def click(self, text=None, role='button', name=None, **properties):
        action = dict(action='click', **properties)
        if name:
            action['target_name'] = name
        else:
            action.update(target_text=text, target_role=role)
        self.actions.append(action)

    def tree(self, text, **properties):
        self.click(text, 'tree', **properties)

    def tab(self, text):
        self.click(text, 'tab')

    def field(self, name, value):
        self.key('Ctrl+A', target_name=name)
        if value:
            self.actions.append(dict(action='text', text=value, target_name=name))
        else:
            self.key('Backspace', target_name=name)

    def labelled(self, label, value, table=False):
        self.actions.append(dict(action='click', field_hint=label, count=2 if table else 1))
        self.key('Ctrl+A')
        self.actions.append(dict(action='text', text=value))
        if table:
            self.key('Return')

    def select(self, name, value):
        self.actions.append(dict(action='select', target_name=name, value=value))

    def check(self, name, value):
        self.actions.append(dict(action='check', target_name=name, checked=value))

    def shot(self, label):
        self.actions.append(dict(action='screenshot', label=re.sub('[^A-Za-z0-9_.-]+', '-', label)[:64]))

    def new(self):
        self.key('Esc')
        self.key('Ctrl+N')
        self.actions.append(dict(action='window', state='maximized'))

    def rename(self, old, new):
        self.tree(old)
        self.key('F2')
        self.field('resourceName', new)
        self.key('Return')

    def open(self, name):
        self.tree(name, count=2)

    def commit(self):
        self.click('Apply')
        self.click('Close')

    def create(self, folder, kind, name=None):
        self.tree(folder, button=3)
        self.click('Add '+kind, 'menu')
        if name:
            self.field('resourceName', name)

    def created(self):
        self.click('Create')
        self.click('Close')

    def append(self, command, branch=None):
        self.tree(branch or 'Mission Sequence', button=3)
        self.click('Append inside branch' if branch else 'Append command', 'menu')
        self.select('commandTemplate', command)

    def command_name(self, name):
        self.field('commandName', name)

    def save(self, filename):
        self.key('Ctrl+S')
        self.field('fileNameEdit', '${WORK}/'+filename)
        self.key('Return')

    def reopen(self, filename):
        self.key('Ctrl+O')
        self.field('fileNameEdit', '${WORK}/'+filename)
        self.key('Return')
        self.shot('own-saved-mission-reopened')


def orbit():
    w = Walk(); w.new(); w.rename('DefaultSC', 'Sat'); w.open('Sat')
    w.select('resource_DateFormat', 'UTCGregorian'); w.field('resource_Epoch', '22 Jul 2014 11:29:10.811')
    w.select('resource_DisplayStateType', 'Keplerian')
    for key, value in [('SMA', '83474.318'), ('ECC', '0.89652'), ('INC', '12.4606'),
                       ('RAAN', '292.8362'), ('AOP', '218.9805'), ('TA', '180')]:
        w.labelled(key, value)
    w.commit(); w.rename('DefaultProp', 'LowEarthProp'); w.open('LowEarthProp_ForceModel')
    w.field('resource_PointMasses', 'Sun, Luna'); w.select('resource_SRP', 'On')
    w.tab('Advanced'); w.tab('Gravity field')
    for name in ('Degree', 'Order'):
        w.field('propertyFilter', 'GravityField.Earth.'+name); w.labelled('GravityField.Earth.'+name, '10', table=True)
    w.click(name='forceAtmosphere'); w.select('atmosphere_AtmosphereModel', 'JacchiaRoberts'); w.click('OK'); w.commit()
    w.open('DefaultOrbitView'); w.select('orbit_ViewPointVector', 'Vector')
    for number, value in enumerate(['-60000', '30000', '20000'], 1):
        w.field('orbit_ViewPointVector'+str(number), value)
    w.check('orbit_XYPlane', False); w.commit(); w.shot('guide-orbit-resources-configured')
    w.tab('Mission'); w.open('Propagate'); w.field('propagationStopParameter', 'Sat.Earth.Periapsis'); w.commit()
    w.save('tutorial01-authored.script'); w.key('F5'); w.shot('guide-periapsis-run')
    w.tab('Mission'); w.tree('Propagate', button=3); w.click('Command summary', 'menu')
    w.select('summaryFrame', 'EarthFixed'); w.shot('guide-command-summary-earth-fixed'); w.click('Close')
    w.click(name='allPlotReplayStart'); w.check('allPlotReplayPlay', True)
    w.actions.append(dict(action='wait', ms=500)); w.check('allPlotReplayPlay', False); w.shot('guide-animation')
    w.reopen('tutorial01-authored.script')
    return w


def transfer():
    w = Walk(); w.new()
    w.create('Solvers', 'DifferentialCorrector', 'DC1'); w.created()
    w.open('DefaultOrbitView'); w.select('orbit_SolverIterations', 'Current'); w.select('orbit_ViewUpAxis', 'X')
    w.select('orbit_ViewPointVector', 'Vector')
    for number, value in enumerate(['0', '0', '120000'], 1): w.field('orbit_ViewPointVector'+str(number), value)
    w.commit()
    for name in ('TOI', 'GOI'): w.create('Burns', 'ImpulsiveBurn', name); w.created()
    w.tab('Mission'); w.open('Propagate'); w.command_name('Prop To Periapsis')
    w.field('propagationStopParameter', 'DefaultSC.Earth.Periapsis'); w.commit()
    w.append('Target'); w.command_name('Hohmann Transfer'); w.commit()
    branch = 'Target — Hohmann Transfer'
    w.append('Vary', branch); w.command_name('Vary TOI'); w.field('commandField_Variable', 'TOI.Element1')
    w.field('commandField_Initial value', '1.0'); w.field('commandField_MaxStep', '0.5'); w.commit()
    w.append('Maneuver', branch); w.command_name('Perform TOI'); w.field('commandField_Burn', 'TOI'); w.commit()
    w.append('Propagate', branch); w.command_name('Prop To Apoapsis'); w.field('propagationStopParameter', 'DefaultSC.Earth.Apoapsis'); w.commit()
    w.append('Achieve', branch); w.command_name('Achieve RMAG = 42165'); w.field('commandField_Goal', 'DefaultSC.Earth.RMAG')
    w.field('commandField_Value', '42164.169'); w.commit()
    w.append('Vary', branch); w.command_name('Vary GOI'); w.field('commandField_Variable', 'GOI.Element1'); w.field('commandField_Initial value', '1.0'); w.commit()
    w.append('Maneuver', branch); w.command_name('Perform GOI'); w.field('commandField_Burn', 'GOI'); w.commit()
    w.append('Achieve', branch); w.command_name('Achieve ECC = 0.005'); w.field('commandField_Goal', 'DefaultSC.Earth.ECC')
    w.field('commandField_Value', '0.005'); w.field('commandField_Tolerance', '0.0001'); w.commit()
    w.append('Propagate'); w.command_name('Prop One Day'); w.select('propagationUnits', 'Seconds'); w.field('propagationDuration', '86400'); w.commit()
    w.save('tutorial02-authored.script'); w.key('F5'); w.shot('guide-nominal-hohmann-solve')
    w.tab('Mission'); w.open(branch); w.click(name='applySolverCorrections'); w.click('Close')
    w.key('Ctrl+S'); w.key('F5'); w.shot('guide-apply-corrections-rerun'); w.reopen('tutorial02-authored.script')
    return w


def finite_burn():
    w = Walk(); w.new()
    for kind in ('ChemicalThruster', 'ChemicalTank'): w.create('Hardware', kind); w.created()
    w.open('ChemicalThruster1'); w.tab('Advanced'); w.click(name='thrusterTankMixtures'); w.click(name='tankMixtureAdd'); w.click('OK')
    w.tab('Fuel'); w.field('propertyFilter', 'DecrementMass')
    w.actions.append(dict(action='select', field_hint='DecrementMass', value='true'))
    w.click(name='thrusterCoefficients'); w.labelled('C1', '1000', table=True); w.click('OK'); w.commit()
    w.open('DefaultSC'); w.tab('Orbit and hardware')
    for property, item in [('Tanks', 'ChemicalTank1'), ('Thrusters', 'ChemicalThruster1')]:
        w.actions.append(dict(action='click', field_button='resource_'+property))
        w.actions.append(dict(action='check', target_text=item, target_role='list', checked=True)); w.click('OK')
    w.commit()
    w.create('Burns', 'FiniteBurn'); w.actions.append(dict(action='click', field_button='resource_Thrusters'))
    w.actions.append(dict(action='check', target_text='ChemicalThruster1', target_role='list', checked=True)); w.click('OK'); w.created()
    w.create('Solvers', 'DifferentialCorrector', 'DC1'); w.created()
    w.create('Variables, Arrays, Strings', 'Variable', 'BurnDuration'); w.created()
    w.tab('Mission'); w.open('Propagate'); w.command_name('Prop To Perigee'); w.field('propagationStopParameter', 'DefaultSC.Earth.Periapsis'); w.commit()
    w.append('Target'); w.command_name('Raise Apogee'); w.select('commandChoice_ExitMode', 'SaveAndContinue'); w.commit()
    branch = 'Target — Raise Apogee'
    w.append('Vary', branch); w.command_name('Vary Burn Duration'); w.field('commandField_Variable', 'BurnDuration')
    for name, value in [('Initial value', '200'), ('Upper', '10000'), ('MaxStep', '100')]: w.field('commandField_'+name, value)
    w.commit()
    w.append('BeginFiniteBurn', branch); w.command_name('Turn Thruster On'); w.commit()
    w.append('Propagate', branch); w.command_name('Prop BurnDuration'); w.field('propagationDuration', 'BurnDuration'); w.commit()
    w.append('EndFiniteBurn', branch); w.command_name('Turn Thruster Off'); w.commit()
    w.append('Propagate', branch); w.command_name('Prop To Apogee'); w.field('propagationStopParameter', 'DefaultSC.Earth.Apoapsis'); w.commit()
    w.append('Achieve', branch); w.command_name('Achieve Apogee Radius = 12000'); w.field('commandField_Value', '12000'); w.commit()
    w.save('tutorial03-authored.script'); w.key('F5'); w.shot('guide-finite-burn-target-solved')
    w.tab('Resources'); w.open('DefaultOrbitView'); w.select('orbit_SolverIterations', 'All'); w.commit()
    w.key('Ctrl+S'); w.key('F5'); w.shot('guide-all-target-trials')
    for command in ('Propagate — Prop To Perigee', 'BeginFiniteBurn — Turn Thruster On', 'EndFiniteBurn — Turn Thruster Off', 'Propagate — Prop To Apogee'):
        w.tab('Mission'); w.tree(command, button=3); w.click('Command summary', 'menu'); w.shot('summary-'+command.split(' — ')[-1]); w.click('Close')
    w.reopen('tutorial03-authored.script')
    return w


def report():
    w = Walk(); w.new()
    parameters = ['DefaultSC.UTCGregorian']+['DefaultSC.EarthMJ2000Eq.'+p for p in ('X', 'Y', 'Z', 'VX', 'VY', 'VZ')]
    for name in ('AutoReport', 'ManualReport', 'DecoratedReport'):
        w.create('Output', 'ReportFile', name); w.field('resource_Filename', name+'.txt')
        if name!='ManualReport': w.field('resource_Add', ', '.join(parameters))
        w.created()
    w.create('Variables, Arrays, Strings', 'String', 'stringVar'); w.created()
    w.tab('Mission'); w.tree('Propagate', button=3); w.click('Delete command', 'menu'); w.click('Yes')
    def manual():
        w.append('Report'); w.field('commandField_Report file', 'ManualReport'); w.field('commandField_Parameters', ' '.join(parameters)); w.commit()
    manual(); w.append('Script event'); w.click(name='editScriptEvent'); w.key('Ctrl+A', target_name='scriptEventBody')
    guide = ET.parse(REPO/'doc/help/src/Tut_CreatingReport.xml').getroot()
    code = ''.join(guide.find('.//d:programlisting', NS).itertext())
    for line in code.splitlines():
        if line: w.actions.append(dict(action='text', target_name='scriptEventBody', text=line))
        w.key('Return')
    w.click('OK'); w.commit()
    for index in range(2):
        if index: w.append('Script event'); w.commit()
        w.append('Propagate'); w.select('propagationUnits', 'Days'); w.field('propagationDuration', '0.1'); w.commit()
    manual(); w.save('AuthoredCreatingReport.script'); w.key('F5'); w.shot('guide-three-reports-produced'); w.reopen('AuthoredCreatingReport.script')
    return w


def mars_resources(w, function=False):
    """The resource tables shared by the current Mars and function chapters."""
    w.new(); w.create('Hardware', 'ChemicalTank', 'MainTank'); w.created(); w.open('MainTank')
    for key, value in [('FuelMass', '1718'), ('FuelDensity', '1000'), ('Pressure', '5000'), ('Volume', '2')]:
        w.field('resource_'+key, value)
    w.commit(); w.rename('DefaultSC', 'MAVEN'); w.open('MAVEN')
    w.select('resource_DateFormat', 'UTCGregorian'); w.field('resource_Epoch', '18 Nov 2013 20:26:24.315')
    w.select('resource_DisplayStateType', 'Keplerian')
    for key, value in zip(('SMA', 'ECC', 'INC', 'RAAN', 'AOP', 'TA'),
                          ('-32593.21599272774' if function else '-32593.21599272777', '1.202872548116186', '28.80241266404144',
                           '173.9693759331483', '240.9696529532762', '359.946553377841' if function else '359.9465533778342')):
        w.labelled(key, value)
    w.tab('Orbit and hardware'); w.actions.append(dict(action='click', field_button='resource_Tanks'))
    w.actions.append(dict(action='check', target_text='MainTank', target_role='list', checked=True)); w.click('OK'); w.commit()
    for name, origin in [('TCM', 'Earth'), ('MOI', 'Mars')]:
        w.create('Burns', 'ImpulsiveBurn', name); w.created(); w.open(name)
        w.select('burn_Origin', origin); w.check('burn_DecrementMass', True); w.select('burn_Tank', 'MainTank'); w.commit()
    w.rename('DefaultProp', 'NearEarth')
    for name, central, primary, points, method, accuracy, maximum in [
        ('NearEarth', 'Earth', 'Earth', 'Luna, Sun', 'RungeKutta89', '1e-13', '600'),
        ('DeepSpace', 'Sun', '', 'Earth, Jupiter, Luna, Mars, Neptune, Saturn, Sun, Uranus, Venus', 'PrinceDormand78', '1e-12', '864000'),
        ('NearMars', 'Mars', 'Mars', 'Sun', 'PrinceDormand78', '1e-12', '86400')]:
        if name!='NearEarth': w.create('Propagators', 'PropSetup', name); w.created()
        w.open(name); w.select('resource_Type', method)
        for key, value in [('InitialStepSize', '600'), ('Accuracy', accuracy), ('MinStep', '0'), ('MaxStep', maximum)]:
            w.field('resource_'+key, value)
        w.commit(); w.open(name+'_ForceModel')
        for key, value in [('CentralBody', central), ('PrimaryBodies', primary), ('PointMasses', points)]: w.field('resource_'+key, value)
        w.select('resource_SRP', 'On'); w.commit()
        if primary:
            w.open(name+'_ForceModel'); w.tab('Advanced'); w.tab('Gravity field')
            values = [('Degree', '8'), ('Order', '8')]
            if primary=='Mars': values.insert(0, ('PotentialFile', '${REPO}/application/data/gravity/mars/Mars50c.cof'))
            for prop, value in values:
                key='GravityField.'+primary+'.'+prop
                w.field('propertyFilter', key); w.labelled(key, value, table=True)
            w.commit()
    w.create('Solvers', 'DifferentialCorrector', 'DefaultDC'); w.created()
    for name, origin, axes in [('SunEcliptic', 'Sun', 'MJ2000Ec'), ('MarsInertial', 'Mars', 'BodyInertial')]:
        w.create('Coordinate Systems', 'CoordinateSystem', name); w.created(); w.open(name)
        w.field('resource_Origin', origin); w.select('resource_Axes', axes); w.commit()
    w.rename('DefaultOrbitView', 'EarthView')
    for name, frame, origin, vector, objects in [
        ('EarthView', 'EarthMJ2000Eq', 'Earth', ['0','0','30000'], 'MAVEN, Earth'),
        ('SolarSystemView', 'SunEcliptic', 'Sun', ['0','0','5e8'], 'MAVEN, Earth, Mars, Sun'),
        ('MarsView', 'MarsInertial', 'Mars', ['22000','22000','0'], 'MAVEN, Earth, Mars')]:
        if name!='EarthView': w.create('Output', 'OrbitView', name); w.created()
        w.open(name); w.select('orbit_CoordinateSystem', frame)
        w.select('orbit_ViewPointReference', origin); w.select('orbit_ViewPointVector', 'Vector')
        for i, value in enumerate(vector, 1): w.field('orbit_ViewPointVector'+str(i), value)
        w.select('orbit_ViewDirection', origin); w.select('orbit_ViewUpCoordinateSystem', frame)
        if name=='EarthView': w.field('orbit_ViewScaleFactor', '4')
        w.click(name='orbitClearObject')
        for obj in objects.split(', '):
            w.select('orbitAvailableObjects', obj); w.click(name='orbitAddObject')
        w.commit()
    w.shot('guide-mars-resources-created')


def mars_propagate(w, label, setup, stop, branch=None, days=False):
    w.append('Propagate', branch); w.command_name(label)
    w.select('propagationPropagator', setup); w.select('propagationSpacecraft', 'MAVEN')
    if days: w.select('propagationUnits', 'Days'); w.field('propagationDuration', stop)
    else: w.field('propagationStopParameter', stop)
    w.commit()


def mars_target(w):
    w.append('Target'); w.command_name('Target desired B-plane Coordinates')
    w.select('commandChoice_ExitMode', 'SaveAndContinue'); w.commit()
    branch='Target — Target desired B-plane Coordinates'
    mars_propagate(w, 'Prop 3 Days', 'NearEarth', '3', branch, days=True)
    mars_propagate(w, 'Prop 12 Days to TCM', 'DeepSpace', '12', branch, days=True)
    for i in range(1, 4):
        w.append('Vary', branch); w.command_name('Vary TCM.'+str(i)); w.field('commandField_Variable', 'TCM.Element'+str(i))
        for key, value in [('Initial value', ['0.003937696373137754','0.006042317048292264','-0.0006747125433520692'][i-1]), ('Perturbation', '0.00001'), ('Lower', '-10e300'), ('Upper', '10e300'), ('MaxStep', '0.002')]:
            w.field('commandField_'+key, value)
        w.commit()
    w.append('Maneuver', branch); w.command_name('Apply TCM'); w.field('commandField_Burn', 'TCM'); w.field('commandField_Spacecraft', 'MAVEN'); w.commit()
    mars_propagate(w, 'Prop 280 Days', 'DeepSpace', '280', branch, days=True)
    mars_propagate(w, 'Prop to Mars Periapsis', 'NearMars', 'MAVEN.Mars.Periapsis', branch)
    for parameter, value in [('BdotT','0'), ('BdotR','-7000')]:
        w.append('Achieve', branch); w.command_name('Achieve '+parameter); w.field('commandField_Goal', 'MAVEN.MarsInertial.'+parameter)
        w.field('commandField_Value', value); w.field('commandField_Tolerance', '0.00001'); w.commit()
    return branch


def mars_capture(w, function=False):
    w.append('Target'); w.command_name('Mars Capture'); w.select('commandChoice_ExitMode', 'SaveAndContinue'); w.commit()
    branch='Target — Mars Capture'
    w.append('Vary', branch); w.command_name('Vary MOI'); w.field('commandField_Variable', 'MOI.Element1')
    for key, value in [('Initial value','-1' if function else '-1.603439847094663'), ('Perturbation','0.00001'), ('Lower','-10e300'), ('Upper','10e300'), ('MaxStep','0.1')]:
        w.field('commandField_'+key, value)
    w.commit(); w.append('Maneuver', branch); w.command_name('Apply MOI'); w.field('commandField_Burn', 'MOI'); w.field('commandField_Spacecraft', 'MAVEN'); w.commit()
    mars_propagate(w, 'Prop to Mars Apoapsis', 'NearMars', 'MAVEN.Mars.Apoapsis', branch)
    w.append('Achieve', branch); w.command_name('Achieve Mars Apoapsis'); w.field('commandField_Goal', 'MAVEN.Mars.RMAG'); w.field('commandField_Value', '12000'); w.commit()
    mars_propagate(w, 'Prop One Day', 'NearMars', '1', days=True)
    return branch


def mars(function=False):
    w=Walk(); mars_resources(w, function=function)
    if function:
        w.create('Output', 'ReportFile', 'rf'); w.created()
        w.create('Functions', 'GmatFunction', 'TargeterInsideFunction'); w.click(name='resourceFunctionNew')
        w.field('fileNameEdit', '${WORK}/TargeterInsideFunction.gmf'); w.key('Return')
        w.key('Ctrl+A', target_name='functionFileText')
        guide=ET.parse(REPO/'doc/help/src/Tut_UsingGMATFunctions.xml').getroot()
        for line in ''.join(guide.find('.//d:programlisting', NS).itertext()).expandtabs(4).splitlines():
            if line: w.actions.append(dict(action='text', target_name='functionFileText', text=line))
            w.key('Return')
        w.click('Save'); w.created()
    w.tab('Mission'); w.tree('Propagate', button=3); w.click('Delete command', 'menu'); w.click('Yes')
    if function:
        w.append('Global'); w.command_name('Make Objects Global')
        w.field('commandField_Objects', 'MAVEN DeepSpace DeepSpace_ForceModel DefaultDC EarthView MainTank MarsView MOI NearEarth NearEarth_ForceModel NearMars NearMars_ForceModel rf SolarSystemView TCM SunEcliptic MarsInertial DefaultGroundTrackPlot')
        w.commit(); w.append('Call function'); w.command_name('Target Desired B-Plane Coord. From Inside Function')
        w.field('commandField_Outputs', ''); w.field('commandField_Inputs', '')
        w.field('commandField_Function', 'TargeterInsideFunction'); w.commit()
        w.append('Report'); w.command_name('Report Parameters'); w.field('commandField_Report file', 'rf')
        w.field('commandField_Parameters', 'MAVEN.UTCGregorian TCM.Element1 TCM.Element2 TCM.Element3 MAVEN.MarsInertial.BdotT MAVEN.MarsInertial.BdotR MAVEN.MarsInertial.INC'); w.commit()
    else:
        branch=mars_target(w)
    w.save('tutorial06-function.script' if function else 'tutorial04-bplane.script'); w.key('F5'); w.shot('guide-first-bplane-target')
    if not function:
        w.tab('Mission'); w.open(branch); w.click(name='applySolverCorrections'); w.click('Close'); w.key('Ctrl+S'); w.key('F5'); w.shot('guide-first-target-corrections')
    w.tab('Mission'); branch=mars_capture(w, function=function); w.key('Ctrl+S'); w.key('F5'); w.shot('guide-mars-capture')
    w.tab('Mission'); w.open(branch); w.click(name='applySolverCorrections'); w.click('Close'); w.key('Ctrl+S'); w.key('F5'); w.shot('guide-mars-capture-corrected')
    for command in ('Maneuver — Apply MOI', 'Propagate — Prop to Mars Apoapsis'):
        w.tab('Mission'); w.tree(command, button=3); w.click('Command summary', 'menu'); w.shot(command); w.click('Close')
    w.reopen('tutorial06-function.script' if function else 'tutorial04-bplane.script')
    return w


def assignment(w, destination, expression, branch=None):
    w.append('Assignment', branch); w.field('commandField_Destination', destination); w.field('commandField_Expression', expression); w.commit()


def propagate(w, setup, satellite, stop='0', units='Seconds', branch=None, single=False, label=None):
    w.append('Propagate', branch)
    if label: w.command_name(label)
    w.select('propagationPropagator', setup); w.select('propagationSpacecraft', satellite)
    if single: w.check('propagationSingleStep', True)
    elif units=='Other parameter':
        w.field('propagationStopParameter', stop)
        if not stop.endswith(('.Periapsis','.Apoapsis')): w.field('propagationDuration','0')
    else: w.select('propagationUnits', units); w.field('propagationDuration', stop)
    w.commit()


def target(w, label, solver, branch=None, exit_mode='SaveAndContinue'):
    w.append('Target', branch); w.command_name(label); w.field('commandField_Solver', solver)
    w.select('commandChoice_ExitMode', exit_mode); w.commit()
    return 'Target — '+label


def vary(w, branch, solver, variable, initial, perturbation, lower, upper, maximum):
    w.append('Vary', branch)
    for key, value in [('Solver',solver), ('Variable',variable), ('Initial value',initial),
                       ('Perturbation',perturbation), ('Lower',lower), ('Upper',upper), ('MaxStep',maximum)]: w.field('commandField_'+key, value)
    w.commit()


def maneuver(w, burn, satellite, branch=None):
    w.append('Maneuver', branch); w.field('commandField_Burn', burn); w.field('commandField_Spacecraft', satellite); w.commit()


def achieve(w, branch, solver, goal, value, tolerance):
    w.append('Achieve', branch)
    for key, text in [('Solver',solver), ('Goal',goal), ('Value',value), ('Tolerance',tolerance)]: w.field('commandField_'+key, text)
    w.commit()


def coordinate(w, name, origin, axes, dependent=None):
    w.create('Coordinate Systems', 'CoordinateSystem', name); w.created(); w.open(name)
    w.field('resource_Origin', origin); w.select('resource_Axes', axes)
    if dependent:
        w.tab('Advanced'); w.click(name='editCoordinateAxes'); w.select('coordinateAxisType', axes)
        for prop, value in dependent.items():
            if prop.endswith('Axis'): w.select('resource_'+prop, value)
            else: w.field('resource_'+prop, value)
        w.click('OK')
    w.commit()


def xy_plot(w, name, x, ys):
    w.create('Output', 'XYPlot', name); w.created(); w.open(name)
    w.field('xyXVariable', x); w.click(name='xySelectY'); w.click(name='reportParameterRemoveAll')
    for y in ys:
        w.field('reportParameterEntry', y); w.click(name='reportAddParameter')
    w.click('OK'); w.commit()


def orbit_objects(w, objects):
    w.click(name='orbitClearObject')
    for name in objects: w.select('orbitAvailableObjects', name); w.click(name='orbitAddObject')


def leo_stationkeeping():
    w=Walk(); w.new(); w.rename('DefaultSC', 'LEOsat'); w.open('LEOsat')
    w.select('resource_DateFormat', 'UTCGregorian'); w.field('resource_Epoch', '01 Jan 2000 11:59:28.000')
    for prop, value in zip(('X','Y','Z','VX','VY','VZ'), ('-4083.9','4691.8','-2576.7','-4.252','-5.509','-3.284')): w.labelled(prop, value)
    w.commit(); w.rename('DefaultProp','LEOprop'); w.open('LEOprop_ForceModel')
    w.field('resource_PointMasses', 'Luna, Sun'); w.select('resource_SRP', 'On'); w.tab('Advanced'); w.tab('Gravity field'); w.click(name='forceAtmosphere')
    w.select('atmosphere_AtmosphereModel','JacchiaRoberts')
    for key in ('HistoricWeatherSource','PredictedWeatherSource'): w.select('atmosphere_'+key,'ConstantFluxAndGeoMag')
    for key,value in [('F107','150'),('F107A','150'),('MagneticIndex','3')]: w.field('atmosphere_'+key,value)
    w.click('OK'); w.commit()
    w.create('Solvers','DifferentialCorrector','DC'); w.created()
    for name in ('TCM1','TCM2'): w.create('Burns','ImpulsiveBurn',name); w.created()
    xy_plot(w,'XYPlot1','LEOsat.A1ModJulian',['LEOsat.Earth.Altitude'])
    w.tab('Mission'); w.tree('Propagate',button=3); w.click('Delete command','menu'); w.click('Yes')
    w.append('While'); w.command_name('While1'); w.field('commandField_Condition','LEOsat.ElapsedDays < 10'); w.commit()
    propagate(w,'LEOprop','LEOsat',branch='While — While1',single=True,label='Prop One Step')
    w.save('tutorial16-part1.script'); w.key('F5'); w.shot('guide-uncontrolled-ten-day-run')
    w.tab('Mission'); w.append('If','While — While1'); w.command_name('If1'); w.field('commandField_Condition','LEOsat.Earth.Altitude < 342'); w.commit()
    w.tree('Else',button=3); w.click('Insert before','menu'); w.select('commandTemplate','Propagate'); w.command_name('Prop To Periapsis')
    w.select('propagationPropagator','LEOprop'); w.select('propagationSpacecraft','LEOsat'); w.field('propagationStopParameter','LEOsat.Earth.Periapsis'); w.commit()
    w.tree('Else',button=3); w.click('Insert before','menu'); w.select('commandTemplate','Target'); w.command_name('Target1')
    w.field('commandField_Solver','DC'); w.select('commandChoice_ExitMode','DiscardAndContinue'); w.commit(); branch='Target — Target1'
    vary(w,branch,'DC','TCM1.Element1','0.002','0.0001','-9.999999e300','9.999999e300','0.05')
    maneuver(w,'TCM1','LEOsat',branch)
    propagate(w,'LEOprop','LEOsat','LEOsat.Earth.Apoapsis','Other parameter',branch)
    achieve(w,branch,'DC','LEOsat.Earth.RMAG','6737','0.1')
    vary(w,branch,'DC','TCM2.Element1','1e-5','0.00005','-9.999999e300','9.999999e300','0.05')
    maneuver(w,'TCM2','LEOsat',branch); achieve(w,branch,'DC','LEOsat.Earth.ECC','0.00005','0.0001')
    w.key('Ctrl+Shift+S'); w.field('fileNameEdit','${WORK}/tutorial16-controlled.script'); w.key('Return')
    w.key('F5'); w.shot('guide-controlled-ten-day-run'); w.reopen('tutorial16-controlled.script'); return w


def ace_stationkeeping():
    w=Walk(); w.new(); w.rename('DefaultSC','ACE'); w.open('ACE')
    w.select('resource_DateFormat','UTCGregorian'); w.field('resource_Epoch','09 Dec 2005 13:00:00.000')
    for prop, value in zip(('X','Y','Z','VX','VY','VZ'), ('-528665.5773210001','-1325445.164527','-525969.843578','0.240435056','-0.068342458','0.0315715361')): w.labelled(prop,value)
    w.commit(); w.rename('DefaultProp','EarthSunMoon'); w.open('EarthSunMoon')
    for prop, value in [('MinStep','0'),('MaxStep','40000'),('MaxStepAttempts','40000')]: w.field('resource_'+prop,value)
    w.commit(); w.open('EarthSunMoon_ForceModel'); w.field('resource_PrimaryBodies',''); w.field('resource_PointMasses','Sun, Earth, Luna'); w.commit()
    w.create('Calculated Points','Barycenter','DefaultBC'); w.field('resource_BodyNames','Earth, Luna'); w.created()
    w.create('Calculated Points','LibrationPoint','ESL1'); w.field('resource_Primary','Sun'); w.field('resource_Secondary','Earth'); w.select('resource_Point','L1'); w.created()
    w.create('Variables, Arrays, Strings','Variable','I'); w.created()
    for name, origin in [('EarthSunL1CS','ESL1'),('EarthSunRotCS','Earth')]:
        coordinate(w,name,origin,'ObjectReferenced',dict(Primary='Sun',Secondary='Earth',XAxis='R',YAxis='',ZAxis='N'))
    w.create('Burns','ImpulsiveBurn','MCC'); w.created(); w.open('MCC'); w.select('burn_CoordinateSystem','EarthSunRotCS'); w.commit()
    w.create('Solvers','DifferentialCorrector','DefaultDC'); w.created()
    w.rename('DefaultOrbitView','L1View'); w.open('L1View')
    orbit_objects(w,['ACE','Earth','Sun'])
    w.select('orbit_CoordinateSystem','EarthSunL1CS'); w.select('orbit_ViewUpCoordinateSystem','EarthSunL1CS')
    w.select('orbit_ViewPointReference','ESL1'); w.select('orbit_ViewDirection','ESL1'); w.field('orbit_ViewScaleFactor','1'); w.select('orbit_ViewUpAxis','X')
    w.select('orbit_ViewPointVector','Vector')
    for i,v in enumerate(('0','0','540000'),1): w.field('orbit_ViewPointVector'+str(i),v)
    w.select('orbit_SolverIterations','Current'); w.commit()
    w.create('Output','OrbitView','EarthSunRotView'); w.created(); w.open('EarthSunRotView'); orbit_objects(w,['ACE','Earth','Luna','Sun'])
    for prop in ('CoordinateSystem','ViewUpCoordinateSystem'): w.select('orbit_'+prop,'EarthSunRotCS')
    w.select('orbit_ViewPointVector','Vector')
    for i,v in enumerate(('-3000000','-1500000','1000000'),1): w.field('orbit_ViewPointVector'+str(i),v)
    w.select('orbit_SolverIterations','Current'); w.commit()
    w.tab('Mission'); w.tree('Propagate',button=3); w.click('Delete command','menu'); w.click('Yes')
    w.append('For'); w.command_name('For1')
    for prop, value in [('Index','I'),('Start','1'),('Step','1'),('End','7')]: w.field('commandField_'+prop,value)
    w.commit(); loop='For — For1'; branch=target(w,'Target1','DefaultDC',loop,exit_mode='DiscardAndContinue')
    vary(w,branch,'DefaultDC','MCC.Element1','MCC.Element1','1e-5','-0.1','0.1','0.001')
    maneuver(w,'MCC','ACE',branch)
    w.append('Propagate',branch); w.select('propagationPropagator','EarthSunMoon'); w.select('propagationSpacecraft','ACE')
    w.field('propagationStopParameter','ACE.EarthSunL1CS.Y'); w.field('propagationDuration','0'); w.field('propagationTolerance','1e-5'); w.commit()
    achieve(w,branch,'DefaultDC','ACE.EarthSunL1CS.VX','0','1e-5')
    w.save('tutorial17-ace.script'); w.key('F5'); w.shot('guide-seven-half-orbits'); w.click(name='allPlotReplayLatest'); w.reopen('tutorial17-ace.script'); return w


def legacy_mars():
    w=Walk(); w.new(); w.rename('DefaultSC','MarsSupply'); w.open('MarsSupply')
    w.select('resource_DateFormat','TAIModJulian'); w.field('resource_Epoch','21160'); w.select('resource_DisplayStateType','Keplerian')
    for prop, value in zip(('SMA','ECC','INC','RAAN','AOP','TA'), ('9999.999999999996','0.09999999999999859','24.99999999999999','332.5000000000001','89.99999999999974','359.9999991462263')): w.labelled(prop,value)
    w.commit(); w.rename('DefaultProp','EarthProp')
    for name, origin, points, accuracy, maximum, attempts in [
        ('EarthProp','Earth','Earth','1e-12','86400','50'), ('SunProp','Sun','Sun, Earth, Luna','1e-12','160000','500'),
        ('MarsProp','Mars','Mars','1e-12','86400','100')]:
        if name!='EarthProp': w.create('Propagators','PropSetup',name); w.created()
        w.open(name)
        for prop,value in [('Accuracy',accuracy),('MinStep','0.001'),('MaxStep',maximum),('MaxStepAttempts',attempts)]: w.field('resource_'+prop,value)
        w.commit(); w.open(name+'_ForceModel'); w.field('resource_CentralBody',origin); w.field('resource_PrimaryBodies',''); w.field('resource_PointMasses',points); w.commit()
    for name, origin, vector in [('TOI','Earth',('2.85','0','0')),('MarsMCC','Sun',('0','-0.8593108944581399','0')),
                                  ('MarsBPlane','Sun',('-0.3309163493210106','0','0.6917117705660262')),('MarsOI','Mars',('-4','0','0'))]:
        w.create('Burns','ImpulsiveBurn',name); w.created(); w.open(name); w.select('burn_Origin',origin)
        for i,value in enumerate(vector,1): w.field('burn_Element'+str(i),value)
        w.commit()
    coordinate(w,'EarthSunRot','Earth','ObjectReferenced',dict(Primary='Sun',Secondary='Earth',XAxis='R',YAxis='',ZAxis='N'))
    for name, origin, axes in [('SunMJ2kEc','Sun','MJ2000Ec'),('SunMJ2kEq','Sun','MJ2000Eq'),('MarsMJ2000Eq','Mars','MJ2000Eq')]: coordinate(w,name,origin,axes)
    w.create('Solvers','DifferentialCorrector','MarsTOIDC'); w.field('resource_ReportFile','targeter_MarsTOIDC.data'); w.created()
    w.create('Output','ReportFile','Data'); w.field('resource_Filename','Ex_MarsBPlane.report'); w.field('resource_ColumnWidth','20'); w.created()
    w.rename('DefaultOrbitView','EarthView')
    for name, frame, origin, objects, scale in [('EarthView','EarthMJ2000Eq','Earth',['MarsSupply','Earth','Sun'],'1'),
                                                ('MarsView','MarsMJ2000Eq','Mars',['MarsSupply','Earth','Sun','Mars'],'2')]:
        if name!='EarthView': w.create('Output','OrbitView',name); w.created()
        w.open(name); orbit_objects(w,objects)
        for prop in ('CoordinateSystem','ViewUpCoordinateSystem'): w.select('orbit_'+prop,frame)
        for prop in ('ViewPointReference','ViewDirection'): w.select('orbit_'+prop,origin)
        w.select('orbit_ViewPointVector','Vector')
        for i,v in enumerate(('0','0','30000'),1): w.field('orbit_ViewPointVector'+str(i),v)
        w.field('orbit_ViewScaleFactor',scale); w.select('orbit_ViewUpAxis','Z'); w.check('orbit_Grid',True); w.check('orbit_SunLine',True); w.commit()
    w.shot('guide-legacy-mars-resources'); w.tab('Mission'); w.tree('Propagate',button=3); w.click('Delete command','menu'); w.click('Yes')
    parameters='MarsSupply.A1ModJulian '+' '.join('MarsSupply.'+p for p in ('X','Y','Z','VX','VY','VZ'))
    def report_row():
        w.append('Report'); w.field('commandField_Report file','Data'); w.field('commandField_Parameters',parameters); w.commit()
    report_row(); assignment(w,'Data.WriteHeaders','Off')
    propagate(w,'EarthProp','MarsSupply','MarsSupply.Earth.Periapsis','Other parameter'); report_row()
    maneuver(w,'TOI','MarsSupply'); report_row(); propagate(w,'EarthProp','MarsSupply','25','Days'); report_row()
    propagate(w,'SunProp','MarsSupply','135','Days'); report_row()
    branch=target(w,'Target1','MarsTOIDC',exit_mode='DiscardAndContinue')
    vary(w,branch,'MarsTOIDC','MarsMCC.Element2','0','0.1','-8','8','0.5'); maneuver(w,'MarsMCC','MarsSupply',branch)
    achieve(w,branch,'MarsTOIDC','MarsSupply.SunMJ2kEq.INC','24.677090','0.001'); report_row()
    branch=target(w,'Target2','MarsTOIDC',exit_mode='DiscardAndContinue')
    vary(w,branch,'MarsTOIDC','MarsBPlane.Element1','-0.233834','0.001','-3','3','0.1')
    vary(w,branch,'MarsTOIDC','MarsBPlane.Element3','0.668261','0.001','-3','3','0.12'); maneuver(w,'MarsBPlane','MarsSupply',branch)
    propagate(w,'SunProp','MarsSupply','50','Days',branch); propagate(w,'MarsProp','MarsSupply','MarsSupply.Mars.Periapsis','Other parameter',branch)
    for parameter,value in [('BdotT','10000'),('BdotR','-10000')]: achieve(w,branch,'MarsTOIDC','MarsSupply.MarsMJ2000Eq.'+parameter,value,'50')
    report_row(); maneuver(w,'MarsOI','MarsSupply'); report_row(); propagate(w,'MarsProp','MarsSupply','0.5','Days'); report_row()
    w.save('tutorial18-mars.script'); w.key('F5'); w.shot('guide-legacy-mars-completed')
    w.reopen('tutorial18-mars.script'); return w


def formation():
    w=Walk(); w.new(); w.rename('DefaultSC','Sat1'); w.rename('DefaultProp','PropRKV_LowEarth')
    rows=ET.parse(REPO/'doc/help/src/Tut_FormationRendezvous.xml').getroot().findall('.//d:informaltable/d:tgroup/d:tbody/d:row',NS)
    states=[]
    for row in rows:
        cells=[''.join(e.itertext()).strip() for e in row]
        if cells[0].startswith('Sat'): states.append((cells[0].split()[0],cells[1:]))
    assert len(states)==4
    for name, values in states:
        if name!='Sat1':
            w.tree('Sat1',button=3); w.click('Clone resource…','menu'); w.field('resourceName',name); w.click('Clone'); w.click('Close')
        w.open(name); w.select('resource_DateFormat','UTCGregorian'); w.field('resource_Epoch','01 Jan 2000 11:59:27.966'); w.select('resource_DisplayStateType','Keplerian')
        for prop,value in zip(('SMA','ECC','INC','RAAN','AOP','TA'),values): w.labelled(prop,value)
        w.commit()
    w.open('PropRKV_LowEarth'); w.field('resource_MinStep','0.001'); w.field('resource_MaxStep','900'); w.commit()
    w.open('PropRKV_LowEarth_ForceModel'); w.field('resource_PrimaryBodies',''); w.field('resource_PointMasses','Earth, Sun, Luna, Jupiter'); w.commit()
    w.create('Burns','ImpulsiveBurn','Burn1'); w.created(); w.open('Burn1'); w.field('burn_Element1','0.0001'); w.commit()
    w.create('Solvers','DifferentialCorrector','DC'); w.field('resource_ReportFile','DifferentialCorrectorDC.data'); w.created()
    coordinate(w,'Sat1LVLH','Sat1','ObjectReferenced',dict(Primary='Earth',Secondary='Sat1',XAxis='',YAxis='-N',ZAxis='-R'))
    coordinate(w,'Sat2VNB','Sat2','ObjectReferenced',dict(Primary='Earth',Secondary='Sat2',XAxis='V',YAxis='N',ZAxis=''))
    w.create('Output','ReportFile','DataReport'); w.field('resource_Filename','FormationRendezvousTutorial.data'); w.created()
    w.open('DataReport'); w.select('resource_SolverIterations','None'); w.commit()
    diagnostic_names=['PositionX','PositionY','PositionZ','PositionNorm','VelocityX','VelocityY','VelocityZ','VelocityNorm',
                      'PositionAtVelocityX','PositionAtVelocityY','PositionAtVelocityZ','PositionBurnV','PositionBurnN','PositionBurnB','VelocityBurnV','VelocityBurnN','VelocityBurnB']
    for name in diagnostic_names: w.create('Variables, Arrays, Strings','Variable',name); w.created()
    w.create('Output','ReportFile','RendezvousResiduals'); w.field('resource_Filename','FormationRendezvousResiduals.data'); w.created()
    for name,x,y in [('Sat2wrtSat1LVLH','Sat2.Sat1LVLH.X','Sat2.Sat1LVLH.Z'),('Sat1wrtSat2VNB','Sat1.Sat2VNB.X','Sat1.Sat2VNB.Y'),('Sat1wrtSat2VNB2','Sat1.Sat2VNB.Y','Sat1.Sat2VNB.Z')]: xy_plot(w,name,x,[y])
    w.rename('DefaultOrbitView','EarthView'); w.open('EarthView'); orbit_objects(w,['Sat1','Sat2','Sat3','Sat4','Earth']); w.commit()
    w.create('Output','OrbitView','Sat2InSat1LVLH'); w.created(); w.open('Sat2InSat1LVLH'); orbit_objects(w,['Sat1','Sat2','Sat3','Sat4','Earth'])
    w.select('orbit_CoordinateSystem','Sat1LVLH'); w.field('orbit_ViewScaleFactor','0.15'); w.select('orbit_ViewUpAxis','X'); w.commit()
    w.shot('guide-formation-resources'); w.tab('Mission'); w.tree('Propagate',button=3); w.click('Delete command','menu'); w.click('Yes')
    for name in ('Sat1wrtSat2VNB','Sat2InSat1LVLH','Sat1wrtSat2VNB2','Sat2wrtSat1LVLH'):
        w.append('Toggle'); w.field('commandField_Subscribers',name); w.select('commandToggleState','On'); w.commit()
    def report_group(branch=None):
        for satellite in ('Sat1','Sat2','Sat3','Sat4'):
            w.append('Report',branch); w.field('commandField_Report file','DataReport')
            w.field('commandField_Parameters',' '.join(satellite+'.EarthMJ2000Eq.'+p for p in ('X','Y','Z','VX','VY','VZ'))); w.commit()
    def formation_prop(value, units, branch=None):
        w.append('Propagate',branch); w.select('propagationPropagator','PropRKV_LowEarth'); w.select('propagationSpacecraft','Sat1'); w.select('propagationUnits',units); w.field('propagationDuration',value)
        w.click(name='editPropagationGroups'); w.key('Ctrl+Home',target_name='propagationGroupsTable'); w.click(name='propagationGroupChoose')
        for satellite in ('Sat1','Sat2','Sat3','Sat4'): w.actions.append(dict(action='check',target_text=satellite,target_role='list',checked=True))
        w.click('OK'); w.click('OK'); w.commit()
    report_group(); assignment(w,'DataReport.WriteHeaders','Off'); formation_prop('0.2','Days'); report_group()
    for i,limit,maximum in [(1,'0.3','0.05'),(2,'1','0.5')]:
        branch=target(w,'Target'+str(i),'DC',exit_mode='DiscardAndContinue')
        for j,initial in [(1,'-0.0001'),(2,'0'),(3,'0')]: vary(w,branch,'DC','Burn1.Element'+str(j),initial,'0.001','-'+limit,limit,maximum)
        maneuver(w,'Burn1','Sat2',branch); formation_prop('5498.65387429','Seconds',branch)
        for component in (('X','Y','Z') if i==1 else ('VX','VY','VZ')):
            achieve(w,branch,'DC','Sat2.'+('Sat1LVLH.' if i==1 else 'EarthMJ2000Eq.')+component,'0' if i==1 else 'Sat1.EarthMJ2000Eq.'+component,'0.0001')
        report_group(branch)
        if i==1:
            for component in ('X','Y','Z'): assignment(w,'Position'+component,'Sat2.Sat1LVLH.'+component)
            assignment(w,'PositionNorm','sqrt(PositionX^2 + PositionY^2 + PositionZ^2)')
            for component,element in zip(('V','N','B'),('1','2','3')): assignment(w,'PositionBurn'+component,'Burn1.Element'+element)
        else:
            for component in ('X','Y','Z'): assignment(w,'Velocity'+component,'Sat2.EarthMJ2000Eq.V'+component+' - Sat1.EarthMJ2000Eq.V'+component)
            assignment(w,'VelocityNorm','sqrt(VelocityX^2 + VelocityY^2 + VelocityZ^2)')
            for component in ('X','Y','Z'): assignment(w,'PositionAtVelocity'+component,'Sat2.Sat1LVLH.'+component)
            for component,element in zip(('V','N','B'),('1','2','3')): assignment(w,'VelocityBurn'+component,'Burn1.Element'+element)
            w.append('Report'); w.field('commandField_Report file','RendezvousResiduals'); w.field('commandField_Parameters',' '.join(diagnostic_names)); w.commit()
    formation_prop('0.2','Days'); report_group(); w.save('tutorial15-formation.script'); w.key('F5'); w.shot('guide-formation-two-targets'); w.reopen('tutorial15-formation.script'); return w


BUILDERS = {'1': orbit, '2': transfer, '3': finite_burn, '4': mars, '6': lambda: mars(function=True), '13': report,
            '15': formation, '16': leo_stationkeeping, '17': ace_stationkeeping, '18': legacy_mars}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tutorial', action='append', choices=sorted(BUILDERS), required=True)
    args = parser.parse_args()
    for number in args.tutorial:
        path = next(DEST.glob(f'{int(number):02}-*.json'))
        recipe = json.loads(path.read_text())
        if hashlib.sha256((REPO/recipe['guide']).read_bytes()).hexdigest()!=recipe['guide_sha256']:
            raise RuntimeError('Review the changed guide before regenerating its instructions')
        walk = BUILDERS[number]()
        recipe['stages'] = [dict(name='Current Help GUI instructions with QtTest selectors', input=None, requires_run=True, actions=walk.actions)]
        recipe['provenance'] = 'Reviewed current Help instructions, using semantic QtTest widget selectors. Values come from the Help chapter; the generator never reads a completed example mission. Literal code is entered only where the chapter explicitly teaches it.'
        path.write_text(json.dumps(recipe, indent=2)+'\n')
        print(recipe['id'], len(walk.actions), 'semantic GUI operations')


if __name__ == '__main__':
    main()
