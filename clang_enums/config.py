"""
Configuration of headers and enums to extract from Clang/LLVM.

This is the main configuration file that defines which enums to extract
from which headers. To add a new enum, simply add it to the appropriate
HeaderConfig or create a new one.
"""

from .models import (
    EnumConfig,
    HeaderConfig,
    remove_prefix,
    remove_prefix_and_upper,
    remove_suffix,
    to_upper,
    lookup_table,
)


# Lookup tables for enums that need custom mapping
INIT_STYLE_MAP = {
    "CInit": "CINIT",
    "CallInit": "CALL_INIT",
    "ListInit": "LIST_INIT",
}

NEW_INIT_STYLE_MAP = {
    "None": "NO_INIT",
    "Parens": "CALL_INIT",
    "Braces": "LIST_INIT",
}


# Header configurations
# Note: paths are relative to the LLVM include directory

HEADERS: list[HeaderConfig] = [
    # clang/AST/Type.h
    HeaderConfig(
        path="clang/AST/Type.h",
        enums=[
            EnumConfig(
                name="Kind",
                cpp_var_name="BUILTIN_KIND",
                exclude={"LastKind"},
                occurrence=3,
            ),
            EnumConfig(
                name="RefQualifierKind",
                cpp_var_name="REFERENCE_QUALIFIER",
                mapper=remove_prefix("RQ_"),
            ),
            EnumConfig(
                name="ArraySizeModifier",
                cpp_var_name="ARRAY_SIZE_MODIFIER",
            ),
            EnumConfig(
                name="UTTKind",
                cpp_var_name="UTT_KIND",
            ),
            EnumConfig(
                name="ElaboratedTypeKeyword",
                cpp_var_name="ELABORATED_TYPE_KEYWORD",
                mapper=remove_prefix("ETK_"),
            ),
            EnumConfig(
                name="TagTypeKind",
                cpp_var_name="TAG_KIND",
                mapper=remove_prefix("TTK_"),
            ),
        ],
    ),
    
    # clang/AST/OperationKinds.h
    HeaderConfig(
        path="clang/AST/OperationKinds.h",
        enums=[
            EnumConfig(
                name="CastKind",
                cpp_var_name="CAST_KIND",
                mapper=remove_prefix("CK_"),
            ),
            EnumConfig(
                name="BinaryOperatorKind",
                cpp_var_name="BINARY_OPERATOR_KIND",
                mapper=remove_prefix("BO_"),
            ),
            EnumConfig(
                name="UnaryOperatorKind",
                cpp_var_name="UNARY_OPERATOR_KIND",
                mapper=remove_prefix("UO_"),
            ),
        ],
    ),
    
    # clang/Basic/AttrKinds.h
    HeaderConfig(
        path="clang/Basic/AttrKinds.h",
        enums=[
            EnumConfig(
                name="Kind",
                cpp_var_name="ATTRIBUTES",
            ),
        ],
    ),
    
    # clang/Basic/Specifiers.h
    HeaderConfig(
        path="clang/Basic/Specifiers.h",
        enums=[
            EnumConfig(
                name="CallingConv",
                cpp_var_name="CALLING_CONVENTION",
                mapper=remove_prefix("CC_"),
            ),
            EnumConfig(
                name="AccessSpecifier",
                cpp_var_name="ACCESS_SPECIFIER",
                mapper=remove_prefix_and_upper("AS_"),
            ),
            EnumConfig(
                name="StorageClass",
                cpp_var_name="STORAGE_CLASS",
                mapper=remove_prefix("SC_"),
            ),
            EnumConfig(
                name="ExplicitSpecKind",
                cpp_var_name="EXPLICIT_SPEC_KIND",
            ),
            EnumConfig(
                name="TemplateSpecializationKind",
                cpp_var_name="TEMPLATE_SPECIALIZATION_KIND",
                mapper=remove_prefix("TSK_"),
            ),
        ],
    ),
    
    # clang/Basic/ExceptionSpecificationType.h
    HeaderConfig(
        path="clang/Basic/ExceptionSpecificationType.h",
        enums=[
            EnumConfig(
                name="ExceptionSpecificationType",
                cpp_var_name="EXCEPTION_SPECIFICATION_TYPE",
                mapper=remove_prefix("EST_"),
            ),
        ],
    ),
    
    # clang/Basic/Linkage.h
    HeaderConfig(
        path="clang/Basic/Linkage.h",
        enums=[
            EnumConfig(
                name="Linkage",
                cpp_var_name="LINKAGE",
                # Note: The JS code had a bug here - it used EST_ prefix for Linkage
                # which doesn't make sense. We'll use identity mapper.
            ),
        ],
    ),
    
    # clang/Basic/Visibility.h
    HeaderConfig(
        path="clang/Basic/Visibility.h",
        enums=[
            EnumConfig(
                name="Visibility",
                cpp_var_name="VISIBILITY",
                mapper=remove_suffix("Visibility"),
            ),
        ],
    ),
    
    # clang/AST/TemplateBase.h
    HeaderConfig(
        path="clang/AST/TemplateBase.h",
        enums=[
            EnumConfig(
                name="ArgKind",
                cpp_var_name="TEMPLATE_ARG_KIND",
            ),
        ],
    ),
    
    # clang/AST/Decl.h
    HeaderConfig(
        path="clang/AST/Decl.h",
        enums=[
            EnumConfig(
                name="InitializationStyle",
                cpp_var_name="INIT_STYLE",
                mapper=lookup_table(INIT_STYLE_MAP),
            ),
            EnumConfig(
                name="TLSKind",
                cpp_var_name="TLS_KIND",
                mapper=remove_prefix_and_upper("TLS_"),
            ),
        ],
    ),
    
    # clang/AST/ExprCXX.h
    HeaderConfig(
        path="clang/AST/ExprCXX.h",
        enums=[
            EnumConfig(
                name="CXXNewInitializationStyle",
                cpp_var_name="NEW_INIT_STYLE",
                mapper=lookup_table(NEW_INIT_STYLE_MAP),
            ),
            EnumConfig(
                name="CXXConstructionKind",
                cpp_var_name="CONSTRUCTION_KIND",
            ),
        ],
    ),
    
    # clang/Basic/TypeTraits.h
    HeaderConfig(
        path="clang/Basic/TypeTraits.h",
        enums=[
            EnumConfig(
                name="UnaryExprOrTypeTrait",
                cpp_var_name="UETT_KIND",
                mapper=remove_prefix("UETT_"),
                exclude={"UETT_Last"},
            ),
        ],
    ),
    
    # clang/AST/NestedNameSpecifier.h
    HeaderConfig(
        path="clang/AST/NestedNameSpecifier.h",
        enums=[
            EnumConfig(
                name="SpecifierKind",
                cpp_var_name="NESTED_NAMED_SPECIFIER",
            ),
        ],
    ),
    
    # clang/AST/DeclCXX.h
    HeaderConfig(
        path="clang/AST/DeclCXX.h",
        enums=[
            EnumConfig(
                name="LinkageSpecLanguageIDs",
                cpp_var_name="LINKAGE_LANGUAGE",
            ),
        ],
    ),
    
    # clang/Basic/Lambda.h
    HeaderConfig(
        path="clang/Basic/Lambda.h",
        enums=[
            EnumConfig(
                name="LambdaCaptureDefault",
                cpp_var_name="LAMBDA_CAPTURE_DEFAULT",
                mapper=remove_prefix("LCD_"),
            ),
            EnumConfig(
                name="LambdaCaptureKind",
                cpp_var_name="LAMBDA_CAPTURE_KIND",
                mapper=remove_prefix("LCK_"),
            ),
        ],
    ),
    
    # clang/AST/TemplateName.h
    HeaderConfig(
        path="clang/AST/TemplateName.h",
        enums=[
            EnumConfig(
                name="NameKind",
                cpp_var_name="TEMPLATE_NAME_KIND",
            ),
        ],
    ),
    
    # clang/AST/Attr.h
    HeaderConfig(
        path="clang/AST/Attr.h",
        enums=[
            EnumConfig(
                name="VisibilityType",
                cpp_var_name="VISIBILITY_ATTR_TYPE",
            ),
        ],
    ),
    
    # clang/AST/Expr.h
    HeaderConfig(
        path="clang/AST/Expr.h",
        enums=[
            EnumConfig(
                name="Kind",
                cpp_var_name="OFFSET_OF_NODE_KIND",
                mapper=to_upper(),
                class_name="OffsetOfNode",
                occurrence=10,
            ),
            EnumConfig(
                name="PredefinedIdentKind",
                cpp_var_name="PREDEFINED_ID_TYPE",
                class_name="PredefinedExpr",
            ),
            EnumConfig(
                name="StringLiteralKind",
                cpp_var_name="STRING_KIND",
                mapper=to_upper(),
            ),
        ],
    ),
]
