/**
 * EXTRA Visitors
 */

#include "../Clang/ClangNodes.h"
#include "../ClangAstDumper/ClangAstDumper.h"
#include "../ClangEnums/ClangEnums.h"

#include <string>

void ClangAstDumper::VisitNestedNameSpecifierChildren(
    NestedNameSpecifier *qualifier) {

    auto qualifierKind = qualifier->getKind();

    switch (qualifierKind) {
    case clang::NestedNameSpecifier::Namespace:
        VisitDeclTop(qualifier->getAsNamespace());
        break;
    case clang::NestedNameSpecifier::NamespaceAlias:
        VisitDeclTop(qualifier->getAsNamespaceAlias());
        break;
    case clang::NestedNameSpecifier::TypeSpec:
        VisitTypeTop(qualifier->getAsType());
        break;
    case clang::NestedNameSpecifier::TypeSpecWithTemplate:
        VisitTypeTop(qualifier->getAsType());
        break;
    case clang::NestedNameSpecifier::Global:
        break;
    case clang::NestedNameSpecifier::Super:
        VisitDeclTop(qualifier->getAsRecordDecl());
        break;
    default:
        throw std::invalid_argument(
            "ClangAstDumper::VisitNestedNameSpecifierChildren("
            "NestedNameSpecifier):: Case not implemented, '" +
            clava::NESTED_NAMED_SPECIFIER[qualifier->getKind()] + "'");
    }
}
