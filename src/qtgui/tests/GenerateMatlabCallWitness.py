"""Add finite MATLAB call cases without changing the protected I/O driver.

The whole-object case reports scalar snapshots obtained by a separate MATLAB
reader because script expressions and Report reject direct ChemicalTank fields.
Its native object, input isolation, output and report assertions are retained.
StringLiteral adds an encoded constant containing
Unicode, punctuation and an apostrophe in main, local and nested calls.
ObjectInOut receives a whole-object result through a formal input alias.
WrongAliasPayload exercises a real MATLAB input error and caller isolation.
The supplementary recovery cases induce an explicit MATLAB error and check
temporary cleanup and unchanged caller values before recovery or close.
Every substitution is recorded and must reverse exactly to the original.
"""
import argparse
import hashlib
import json
from pathlib import Path


def generate(source: Path, output: Path, receipt: Path) -> None:
    original = source.read_text(encoding="utf-8")
    replacements = [
        ('#include "Array.hpp"', '#include "Array.hpp"\n#include "ScriptStringLiteral.hpp"'),
        ('write(matlab,body.toUtf8());rig.save("MatIo.m",body.toUtf8());',
         'if(mode=="LiteralRecovery"||mode=="LiteralClose")body.replace("out = 3*x + 1;", "if x == -3.5\\n error(\'GMAT:LiteralProbe\', \'Deliberate literal failure\');\\nend\\nout = 3*x + 1;");\n   write(matlab,body.toUtf8());rig.save("MatIo.m",body.toUtf8());'),
        ('"UndeclaredOutput","WrongAliasType"}.contains(mode)',
         '"UndeclaredOutput","WrongAliasType","StringLiteral","ObjectInOut","WrongAliasPayload"}.contains(mode)'),
        ('QStringList{"String","QuotedString","StringProperty"}.contains(mode)',
         'QStringList{"String","QuotedString","StringProperty","StringLiteral"}.contains(mode)'),
        ('if(kind=="String")return (mode=="QuotedString"?',
         'if(mode=="StringLiteral")return QString::fromUtf8("literal (Ω), pilot\'s log");\n if(kind=="String")return (mode=="QuotedString"?'),
        ('return {name+(kind=="Object"?".FuelMass":"")};',
         'return {kind=="Object"?"Snapshot"+name:name};'),
        ('if(kind=="String")return name+" = \'"+value.toString()+"\';\\n";',
         'if(kind=="String")return name+" = "+QString::fromStdString(GmatScriptString::Quote(value.toString().toStdString()))+";\\n";'),
        ('else functionBody+="[out] = MatIo("+QString(mode=="NoInput"?"":"input")+");\\n";',
         'else if(mode=="StringLiteral")functionBody+="[out] = MatIo("+QString::fromStdString(GmatScriptString::Quote(input(kind,mode,0).toString().toStdString()))+");\\n";\n   else functionBody+="[out] = MatIo("+QString(mode=="NoInput"?"":"input")+");\\n";'),
        ('script+="Create ReportFile Values;\\nValues.Filename = "+quoted(rig.report)',
         'if(kind=="Object"){const auto reader=directory+"/ReadMass.m";write(reader,"function out = ReadMass(x)\\nout = x.FuelMass;\\nend\\n");script+="Create MatlabFunction ReadMass;\\nReadMass.FunctionPath = "+quoted(reader)+";\\n";for(const auto &name:inputs+outputs)script+="Create Variable Snapshot"+name+";\\n";}\n   script+="Create ReportFile Values;\\nValues.Filename = "+quoted(rig.report)'),
        ('mode=="StringProperty"?QString("Carrier.UTCGregorian"):QString("Input"))',
         'mode=="StringProperty"?QString("Carrier.UTCGregorian"):mode=="StringLiteral"?QString::fromStdString(GmatScriptString::Quote(input(kind,mode,0).toString().toStdString())):QString("Input"))'),
        ('script+="Report Values "+columns.join(\' \')',
         'if(kind=="Object")for(const auto &name:inputs+outputs)script+="[Snapshot"+name+"] = ReadMass("+name+");\\n";\n   script+="Report Values "+columns.join(\' \')'),
        ('if(mode=="Literals")expected=-9.5;',
         'if(mode=="Literals")expected=-9.5;if(mode=="StringLiteral")expected=input(kind,mode,0).toString()+"|done";'),
        ('{"matlab_calls",4}', '{"matlab_calls",kind=="Object"?12:4}'),
        ('mode=="Object"?"Object":"Scalar";',
         'QStringList{"Object","ObjectInOut"}.contains(mode)?"Object":"Scalar";'),
    ]
    # Extend the existing strict in/out and rejection contexts. The replacement
    # deliberately contains no original needle, so each occurrence is changed
    # once and the reverse check can recover every original assertion.
    for needle, replacement, expected in [
        ('mode=="InOut"', 'QStringList{"InOut","ObjectInOut"}.contains(mode)', 4),
        ('mode=="WrongAliasType"',
         'QStringList{"WrongAliasType","WrongAliasPayload"}.contains(mode)', 5),
    ]:
        if original.count(needle) != expected:
            raise ValueError("Original MATLAB alias contexts changed")
        replacements.extend([(needle, replacement)] * expected)
    replacements += [
        ('write(matlab,body.toUtf8());rig.save("MatIo.m",body.toUtf8());',
         'if(mode=="WrongAliasPayload")body.replace("out = 3*x + 1;", "if isstruct(x)\\n error(\'GMAT:NumericInputRequired\', \'Expected numeric input; received ChemicalTank structure\');\\nend\\nout = 3*x + 1;");\n   write(matlab,body.toUtf8());rig.save("MatIo.m",body.toUtf8());'),
        ('diagnostic.contains("unsupported MATLAB output type")&&diagnostic.contains("ChemicalTank")',
         '(mode=="WrongAliasPayload"?diagnostic.contains("Expected numeric input; received ChemicalTank structure"):(diagnostic.contains("unsupported MATLAB output type")&&diagnostic.contains("ChemicalTank")))'),
        ('auto *payload=Moderator::Instance()->GetInternalObject("Payload");',
         'if(mode=="WrongAliasPayload"){MatlabInterface::Instance()->RunMatlabString("assert(isempty(who(\'gmat_io_*\')));");require(Moderator::Instance()->GetInternalObject("FunctionCalls")->GetRealParameter("Value")==1&&!QFileInfo::exists(rig.report),"MATLAB payload rejection did not preserve execution boundary");}\n    auto *payload=Moderator::Instance()->GetInternalObject("Payload");'),
        ('{"checkpoint","matlab-function-output-type-rejection"}',
         '{"checkpoint",mode=="WrongAliasPayload"?"matlab-function-numeric-payload-rejection":"matlab-function-output-type-rejection"}'),
    ]
    # Keep all the protected recovery assertions, but give the supplementary
    # variant an actual MATLAB failure rather than the obsolete null-runner
    # failure. Record each exact replacement for the reverse-source check.
    old_diagnostic = 'FunctionRunner::ValidateInputs() function is NULL'
    for context in [f'diagnostic.contains("{old_diagnostic}")',
                    f'{{"diagnostic","{old_diagnostic}"}}']:
        count = original.count(context)
        if count < 1:
            raise ValueError("Original recovery diagnostic context changed")
        # The metadata context occurs in both close and recovery receipts.
        for _ in range(count):
            replacements.append((context, context.replace(old_diagnostic, 'Deliberate literal failure')))
    replacements += [
        ('require(rig.source->toPlainText()==invalidScript&&rig.source->document()->availableUndoSteps()==undo,',
         'MatlabInterface::Instance()->RunMatlabString("assert(isempty(who(\'gmat_io_*\')));");\n    require(Moderator::Instance()->GetInternalObject("Main")->GetRealParameter("Value")==2.5&&Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==41&&Moderator::Instance()->GetInternalObject("FunctionCalls")->GetRealParameter("Value")==0,"Failed literal call changed caller values/counter");\n    require(rig.source->toPlainText()==invalidScript&&rig.source->document()->availableUndoSteps()==undo,'),
        ('require(status==MainWindow::RunResult::Completed,"Actual MATLAB function I/O mission failed");',
         'require(status==MainWindow::RunResult::Completed,"Actual MATLAB function I/O mission failed");MatlabInterface::Instance()->RunMatlabString("assert(isempty(who(\'gmat_io_*\')));");'),
    ]
    generated = original
    for before, after in replacements:
        if before not in generated:
            raise ValueError(f"Ambiguous or changed original call witness: {before}")
        generated = generated.replace(before, after, 1)
    reversed_text = generated
    for before, after in reversed(replacements):
        reversed_text = reversed_text.replace(after, before, 1)
    if reversed_text != original:
        raise ValueError("Supplementary call witness changed unrelated source")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(generated, encoding="utf-8")
    digest = lambda text: hashlib.sha256(text.encode()).hexdigest()
    receipt.write_text(json.dumps({
        "source": str(source), "source_sha256": digest(original),
        "generated": str(output), "generated_sha256": digest(generated),
        "substitutions": replacements, "exact_reverse_recovers_original": True,
        "new_requirement_credit": 0,
    }, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("receipt", type=Path)
    args = parser.parse_args()
    generate(args.source, args.output, args.receipt)
