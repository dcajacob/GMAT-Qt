function [F, GradF] = GmatQtFminconCall(X, iteration)
% Evaluate the real GMAT optimizer branch through the Qt callback service.
% This protocol has no facility to submit a MATLAB-to-GMAT script.
global GMAT_QT_CALLBACK
global NonLinearEqCon JacNonLinearEqCon NonLinearIneqCon JacNonLinearIneqCon
assert(isstruct(GMAT_QT_CALLBACK) && isfield(GMAT_QT_CALLBACK,'port') && ...
    isfield(GMAT_QT_CALLBACK,'token'), 'GMAT:QtCallbackSession', 'Missing Qt callback session');
assert(isnumeric(X) && isreal(X) && all(isfinite(X(:))), ...
    'GMAT:QtCallbackVariables', 'Callback variables must be finite real numbers');
client = tcpclient('127.0.0.1', GMAT_QT_CALLBACK.port, 'Timeout', 3600);
cleanup = onCleanup(@()delete(client)); %#ok<NASGU>
workspace = GmatQtOptimizerWorkspace('enter',GMAT_QT_CALLBACK.token);
workspaceCleanup = onCleanup(@()cleanupWorkspace(workspace)); %#ok<NASGU>
configureTerminator(client,'LF');
request = struct;
request.token = GMAT_QT_CALLBACK.token;
request.op = 'evaluate';
request.id = 1;
request.protocol = 2;
% A cell array preserves the JSON array shape even for one control variable.
request.x = num2cell(X(:).');
if nargin > 1
    request.op = 'iteration';
    request.state = iteration.state;
    request.iteration = iteration.iteration;
    request.funccount = iteration.funccount;
    request.fval = iteration.fval;
end
writeline(client,jsonencode(request));
while true
    raw = readline(client);
    assert(~isempty(raw), 'GMAT:QtCallbackTimeout', 'No response from the GMAT callback service');
    response = jsondecode(char(raw));
    if ~isstruct(response) || ~isfield(response,'kind') || ~strcmp(response.kind,'matlab'), break; end
    assert(isfield(response,'id') && response.id == request.id && ...
        isfield(response,'token') && strcmp(response.token,request.token) && ...
        isfield(response,'job') && isfield(response,'request') && isfield(response,'result'), ...
        'GMAT:QtCallbackResponse','Invalid nested MATLAB work identity');
    reply = struct('op','matlab_result','token',request.token,'id',request.id, ...
        'job',response.job,'ok',true,'error','');
    try
        GmatQtMatlabJob(response.request,response.result);
    catch failure
        reply.ok = false; reply.error = failure.message;
    end
    writeline(client,jsonencode(reply));
end
assert(isstruct(response) && isfield(response,'ok') && isfield(response,'id') && response.id == request.id, ...
    'GMAT:QtCallbackResponse', 'Invalid callback response identity');
GmatQtOptimizerWorkspace('leave',workspace.session,workspace);
if ~response.ok
    error('GMAT:QtCallbackEvaluation','%s',response.error);
end
F = response.F;
GradF = response.GradF(:);
NonLinearEqCon = response.NonLinearEqCon(:);
NonLinearIneqCon = response.NonLinearIneqCon(:);
JacNonLinearEqCon = response.JacNonLinearEqCon;
JacNonLinearIneqCon = response.JacNonLinearIneqCon;
assert(isscalar(F) && isfinite(F) && all(isfinite(GradF)) && ...
    all(isfinite(NonLinearEqCon)) && all(isfinite(NonLinearIneqCon)), ...
    'GMAT:QtCallbackResult', 'Nonfinite optimizer callback result');
end

function cleanupWorkspace(workspace)
try
    GmatQtOptimizerWorkspace('leave',workspace.session,workspace);
catch failure
    % The normal path reports restoration errors directly. Preserve an earlier
    % evaluation/transport exception while still reporting cleanup failure.
    try
        warning('GMAT:OptimizerWorkspaceCleanup','Could not restore optimizer workspace: %s',failure.message);
    catch
    end
end
end
