function result = GmatQtOptimizerWorkspace(action, session, identity)
% Private workspace ownership for nested optimizer jobs in a Qt callback.
if nargin < 3, identity = []; end
assert(ischar(session) && isrow(session) && ~isempty(session), ...
    'GMAT:OptimizerWorkspace','Invalid optimizer workspace session');
persistent workspaces callbacks nextWorkspace nextCallback currentSession
if isempty(currentSession) || ~strcmp(currentSession,session)
    assert(isempty(workspaces) && isempty(callbacks), ...
        'GMAT:OptimizerWorkspace','Cannot replace a session with active workspace owners');
    workspaces = {}; callbacks = {}; nextWorkspace = uint64(0); nextCallback = uint64(0);
    currentSession = session;
end
result = [];
switch action
    case 'enter'
        assert(nextCallback < intmax('uint64'),'GMAT:OptimizerWorkspace','Callback identities are exhausted');
        nextCallback = nextCallback+uint64(1);
        result = struct('session',session,'identity',nextCallback,'depth',numel(workspaces));
        callbacks{end+1} = result;
    case 'push'
        assert(~isempty(callbacks),'GMAT:OptimizerWorkspace','No callback owns the optimizer workspace');
        assert(nextWorkspace < intmax('uint64'),'GMAT:OptimizerWorkspace','Optimizer identities are exhausted');
        entry.variables = captureVariables();
        nextWorkspace = nextWorkspace+uint64(1);
        entry.identity = ['gmat_optimizer_' char(string(nextWorkspace))];
        entry.callback = callbacks{end}.identity;
        workspaces{end+1} = entry;
        result = entry.identity;
    case 'pop'
        assert(~isempty(callbacks) && ~isempty(workspaces) && ischar(identity) && ...
            strcmp(workspaces{end}.identity,identity) && workspaces{end}.callback == callbacks{end}.identity, ...
            'GMAT:OptimizerWorkspace','The optimizer workspace owner or order has changed');
        restoreVariables(workspaces{end}.variables);
        % Keep the capture until restoration succeeds, so callback unwinding can
        % retry a partial restoration instead of losing the parent value.
        workspaces(end) = [];
    case 'leave'
        assert(isstruct(identity) && isscalar(identity) && ...
            all(isfield(identity,{'session','identity','depth'})) && strcmp(identity.session,session) && ...
            isa(identity.identity,'uint64') && isscalar(identity.identity) && identity.identity >= uint64(1) && ...
            identity.identity <= nextCallback, 'GMAT:OptimizerWorkspace','Invalid callback workspace owner');
        position = find(cellfun(@(entry)entry.identity == identity.identity,callbacks),1);
        if isempty(position), return; end % Already retired; cleanup is idempotent.
        assert(position == numel(callbacks) && callbacks{end}.depth == identity.depth && ...
            numel(workspaces) >= identity.depth, 'GMAT:OptimizerWorkspace','Callback workspace order has changed');
        while numel(workspaces) > identity.depth
            assert(workspaces{end}.callback == identity.identity, ...
                'GMAT:OptimizerWorkspace','A different callback owns the pending workspace');
            restoreVariables(workspaces{end}.variables);
            workspaces(end) = [];
        end
        callbacks(end) = [];
    otherwise
        error('GMAT:OptimizerWorkspace','Unknown optimizer workspace operation');
end
end

function value = emptyVariable()
value = struct('name','','baseExists',false,'baseGlobal',false,'baseValue',[], ...
    'globalExists',false,'globalValue',[]);
end

function names = privateStatusNames(baseInfo,globalInfo)
names = [{baseInfo.name},{globalInfo.name}];
names = names(~cellfun('isempty',regexp(names,'^gmat_eval_status_[0-9]+$','once')));
names = unique(names,'stable');
end

function entries = captureVariables()
% Fixed driver/startup names are private to the nested operation. User MATLAB
% variables outside these names and the evaluation-status family remain shared.
names = {'GMAToptions','X0','Lower','Upper','X','fVal','exitFlag','GMATfminconOutput', ...
    'GMAT_QT_CALLBACK','NonLinearEqCon','JacNonLinearEqCon','NonLinearIneqCon','JacNonLinearIneqCon', ...
    'GMAT_QT_ITERATION_AUDIT','GMAT_QT_ITERATION_HISTORY','callbackResults','errormsg', ...
    'fminconexist','startupexist','whichStr','driverexist','objectiveexist','constraintexist','callbackexist', ...
    'pwdStr','pathStr','GMAT_KEYWORD_DIR','GMAT_OPT_DIR'};
baseInfo = evalin('base','whos'); globalInfo = whos('global');
names = unique([names,privateStatusNames(baseInfo,globalInfo)],'stable');
entries = repmat(emptyVariable(),1,numel(names));
for index = 1:numel(names)
    entry = emptyVariable(); entry.name = names{index};
    baseIndex = find(strcmp({baseInfo.name},entry.name),1);
    entry.baseExists = ~isempty(baseIndex);
    if entry.baseExists
        entry.baseGlobal = baseInfo(baseIndex).('global');
        entry.baseValue = evalin('base',entry.name);
    end
    entry.globalExists = any(strcmp({globalInfo.name},entry.name));
    assert(~entry.baseGlobal || entry.globalExists, ...
        'GMAT:OptimizerWorkspace','The parent global binding has changed');
    if entry.globalExists, entry.globalValue = readGlobal(entry.name); end
    entries(index) = entry;
end
end

function restoreVariables(entries)
baseInfo = evalin('base','whos'); globalInfo = whos('global');
% A child can choose another private status suffix. Remove newly introduced
% status variables as well as restoring the ones that existed at capture.
newNames = setdiff(privateStatusNames(baseInfo,globalInfo),{entries.name},'stable');
for index = 1:numel(newNames)
    entry = emptyVariable(); entry.name = newNames{index}; entries(end+1) = entry; %#ok<AGROW>
end
for index = 1:numel(entries)
    entry = entries(index);
    if entry.globalExists
        writeGlobal(entry.name,entry.globalValue);
    elseif any(strcmp({globalInfo.name},entry.name))
        clear('global',entry.name);
    end
end
% Clearing a newly created global can also remove its base binding. Inspect
% bindings after the global pass and never clear an unrelated function name.
baseInfo = evalin('base','whos');
for index = 1:numel(entries)
    entry = entries(index);
    position = find(strcmp({baseInfo.name},entry.name),1);
    existsNow = ~isempty(position);
    globalNow = existsNow && baseInfo(position).('global');
    if ~entry.baseExists
        if existsNow, evalin('base',['clear ' entry.name ';']); end
    elseif entry.baseGlobal
        if existsNow && ~globalNow, evalin('base',['clear ' entry.name ';']); end
        if ~globalNow, evalin('base',['global ' entry.name ';']); end
        assignin('base',entry.name,entry.baseValue);
    else
        if globalNow, evalin('base',['clear ' entry.name ';']); end
        assignin('base',entry.name,entry.baseValue);
    end
end
end

function value = readGlobal(name)
% This function has no variables named like the fixed driver/status names.
% Declare only an existing global, without changing the base binding.
eval(['global ' name]);
value = eval(name);
end

function writeGlobal(name,value)
eval(['global ' name]);
eval([name ' = value;']);
end
