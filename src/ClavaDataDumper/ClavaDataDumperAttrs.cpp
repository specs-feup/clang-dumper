//
// Created by JoaoBispo on 12/04/2018.
//

#include "../Clang/ClangNodes.h"
#include "../Clava/HandlerCoverage.h"
#include "../ClangEnums/ClangEnums.h"
#include "../ClavaDataDumper/ClavaDataDumper.h"

#include <map>

#define ATTR_DATA_ENTRY(CLASS)                                                 \
  {#CLASS, {#CLASS, [](clava::ClavaDataDumper &self, const Attr *A) {          \
    self.Dump##CLASS##Data(static_cast<const CLASS *>(A));                     \
  }}}

const std::map<std::string, clava::ClavaDataDumper::AttrDataEntry>
    clava::ClavaDataDumper::ATTR_DATA_DUMPERS = {
        ATTR_DATA_ENTRY(AlignedAttr),
        ATTR_DATA_ENTRY(OpenCLUnrollHintAttr),
        ATTR_DATA_ENTRY(FormatAttr),
        ATTR_DATA_ENTRY(NonNullAttr),
        ATTR_DATA_ENTRY(VisibilityAttr),
};

void clava::ClavaDataDumper::dump(const Attr *A) {
    const std::string classname = clava::getClassName(A);
    auto it = ATTR_DATA_DUMPERS.find(classname);
    // NOTE: the legacy section name for the generic attribute data was
    // "<AttributeData>", not "<AttrData>".
    const char *dataName =
        it != ATTR_DATA_DUMPERS.end() ? it->second.dataName : "Attribute";
    clava::recordHandlerEncounter("attr data", classname);

    // Dump header
    llvm::errs() << "<" << dataName << "Data>\n";
    llvm::errs() << clava::getId(A, id) << "\n";
    llvm::errs() << clava::getClassName(A) << "\n";

    if (it != ATTR_DATA_DUMPERS.end()) {
        it->second.dump(*this, A);
    } else {
        clava::recordHandlerFallback("attr data", classname);
        // Default: plain Attr data
        DumpAttrData(A);
    }
}

void clava::ClavaDataDumper::DumpAttrData(const Attr *A) {
    clava::dumpSourceInfo(Context, A->getRange().getBegin(),
                          A->getRange().getEnd());

    clava::dump(clava::getAttrKind(A));
    clava::dump(A->isImplicit());
    clava::dump(A->isInherited());
    clava::dump(A->isLateParsed());
    clava::dump(A->isPackExpansion());
}

void clava::ClavaDataDumper::DumpAlignedAttrData(const AlignedAttr *A) {
    // Common
    DumpAttrData(A);

    clava::dump(A->getSpelling());
    clava::dump(A->isAlignmentExpr());
    if (A->isAlignmentExpr()) {
        clava::dump(clava::getId(A->getAlignmentExpr(), id));
    } else {
        clava::dump(A->getAlignmentType()->getType(), id);
    }
}

void clava::ClavaDataDumper::DumpOpenCLUnrollHintAttrData(
    const OpenCLUnrollHintAttr *A) {
    // Common
    DumpAttrData(A);

    clava::dump(A->getUnrollHint());
}

void clava::ClavaDataDumper::DumpFormatAttrData(const FormatAttr *A) {
    // Common
    DumpAttrData(A);

    clava::dump(A->getType()->getName());
    clava::dump(A->getFormatIdx());
    clava::dump(A->getFirstArg());
}

void clava::ClavaDataDumper::DumpNonNullAttrData(const NonNullAttr *A) {
    // Common
    DumpAttrData(A);

    // Dump args
    clava::dump(A->args_size());
    for (auto I = A->args_begin(), E = A->args_end(); I != E; ++I) {
        clava::dump((*I).getSourceIndex());
    }
}

void clava::ClavaDataDumper::DumpVisibilityAttrData(const VisibilityAttr *A) {
    // Common
    DumpAttrData(A);

    clava::dump(clava::VISIBILITY_ATTR_TYPE[A->getVisibility()]);
}
