function stop = GmatQtFminconOutputFcn(X, optimValues, state)
% Publish actual MATLAB iteration metadata with GMAT values evaluated at X.
% Active-set 'interrupt' callbacks repeat the last iteration; omit duplicates.
stop = false;
if strcmp(state,'interrupt')
    return
end
event = struct;
event.state = state;
event.iteration = optimValues.iteration;
event.funccount = optimValues.funccount;
event.fval = optimValues.fval;
[F, ~] = GmatQtFminconCall(X,event);
% Optional acceptance diagnostics. Normal runs do not retain an unbounded trace.
global GMAT_QT_ITERATION_AUDIT GMAT_QT_ITERATION_HISTORY
if isequal(GMAT_QT_ITERATION_AUDIT,true)
    event.x = num2cell(X(:).');
    event.native_fval = F;
    GMAT_QT_ITERATION_HISTORY(end+1) = event;
end
end
