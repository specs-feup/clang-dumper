#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <vector>

namespace astwire::v1obj {

struct RangeT; struct SourceInfoT; struct NodeDataT;
struct CXXBaseSpecifierT; struct ExplicitSpecifierT;
struct TemplateDeclarationT; struct TemplateNullPtrT; struct TemplateTypeT;
struct TemplateExpressionT; struct TemplatePackT; struct TemplateIntegralT;
struct TemplateExpansionT; struct TemplateStructuralValueT;
struct TemplateArgumentT; struct DirectTemplateNameT;
struct QualifiedTemplateNameT; struct SubstitutedTemplateNameT;
struct UsingTemplateNameT; struct DependentTemplateNameT; struct TemplateNameT;
struct AnyMemberInitializerT; struct BaseInitializerT;
struct DelegatingInitializerT; struct CXXCtorInitializerT;
struct NoExceptionDetailsT; struct ComputedExceptionDetailsT;
struct UnevaluatedExceptionDetailsT; struct UninstantiatedExceptionDetailsT;
struct ExceptionSpecificationT; struct OffsetArrayT; struct OffsetFieldT;
struct OffsetIdentifierT; struct OffsetBaseT; struct OffsetOfComponentT;
struct FieldDesignatorT; struct ArrayDesignatorT; struct ArrayRangeDesignatorT;
struct DesignatorT; struct AsmInputT; struct AsmOutputT;
struct NamespaceSpecifierT; struct NamespaceAliasSpecifierT;
struct TypeSpecifierT; struct TypeWithTemplateSpecifierT;
struct GlobalSpecifierT; struct SuperSpecifierT; struct NestedNameSpecifierT;
struct DeclDataT; struct NamedDeclDataT; struct TypeDeclDataT;
struct TagDeclDataT; struct RecordDeclDataT; struct ValueDeclDataT;
struct DeclaratorDeclDataT; struct TemplateDeclDataT; struct FunctionDeclDataT;
struct CXXMethodDeclDataT; struct CXXConstructorDeclDataT;
struct CXXConversionDeclDataT; struct FieldDeclDataT; struct ParmVarDeclDataT;
struct VarDeclDataT; struct TemplateTypeParmDeclDataT;
struct UnresolvedUsingTypenameDeclDataT; struct EnumDeclDataT;
struct CXXRecordDeclDataT; struct ClassTemplateSpecializationDeclDataT;
struct NonTypeTemplateParmDeclDataT; struct TypedefNameDeclDataT;
struct AccessSpecDeclDataT; struct UsingDeclDataT;
struct UsingDirectiveDeclDataT; struct NamespaceDeclDataT;
struct NamespaceAliasDeclDataT; struct LinkageSpecDeclDataT;
struct StaticAssertDeclDataT; struct TemplateTemplateParmDeclDataT;
struct MSPropertyDeclDataT; struct ClassTemplatePartialSpecializationDeclDataT;
struct TypeDataT; struct QualTypeDataT; struct BuiltinTypeDataT;
struct PointerTypeDataT; struct FunctionTypeDataT; struct FunctionProtoTypeDataT;
struct ArrayTypeDataT; struct ConstantArrayTypeDataT;
struct VariableArrayTypeDataT; struct DependentSizedArrayTypeDataT;
struct TagTypeDataT; struct TypeWithKeywordDataT; struct ElaboratedTypeDataT;
struct TemplateTypeParmTypeDataT; struct TemplateSpecializationTypeDataT;
struct TypedefTypeDataT; struct AdjustedTypeDataT; struct DecayedTypeDataT;
struct DecltypeTypeDataT; struct AutoTypeDataT; struct ReferenceTypeDataT;
struct TypeOfExprTypeDataT; struct PackExpansionTypeDataT;
struct UnaryTransformTypeDataT; struct AttributedTypeDataT;
struct SubstTemplateTypeParmTypeDataT; struct ComplexTypeDataT;
struct ExprDataT; struct CastExprDataT; struct LiteralDataT;
struct CharacterLiteralDataT; struct IntegerLiteralDataT;
struct FloatingLiteralDataT; struct StringLiteralDataT;
struct CXXBoolLiteralExprDataT; struct CompoundLiteralExprDataT;
struct InitListExprDataT; struct DeclRefExprDataT; struct OverloadExprDataT;
struct CXXConstructExprDataT; struct CXXTemporaryObjectExprDataT;
struct MemberExprDataT; struct MaterializeTemporaryExprDataT;
struct BinaryOperatorDataT; struct UnresolvedMemberExprDataT;
struct UnresolvedLookupExprDataT; struct CallExprDataT;
struct CXXMemberCallExprDataT; struct CXXTypeidExprDataT;
struct ExplicitCastExprDataT; struct CXXNamedCastExprDataT;
struct CXXDependentScopeMemberExprDataT; struct UnaryOperatorDataT;
struct UnaryExprOrTypeTraitExprDataT; struct CXXNewExprDataT;
struct CXXDeleteExprDataT; struct OffsetOfExprDataT; struct LambdaExprDataT;
struct PredefinedExprDataT; struct SizeOfPackExprDataT;
struct ArrayInitLoopExprDataT; struct DesignatedInitExprDataT;
struct DependentScopeDeclRefExprDataT; struct CXXNoexceptExprDataT;
struct CXXPseudoDestructorExprDataT; struct PseudoObjectExprDataT;
struct MSPropertyRefExprDataT; struct StmtDataT; struct LabelStmtDataT;
struct GotoStmtDataT; struct AttributedStmtDataT; struct AsmStmtDataT;
struct GCCAsmStmtDataT; struct MSAsmStmtDataT; struct AttributeDataT;
struct AlignedAttrDataT; struct OpenCLUnrollHintAttrDataT;
struct FormatAttrDataT; struct NonNullAttrDataT; struct VisibilityAttrDataT;
struct FileT; struct ChildrenT; struct NodeClassT; struct TopLevelT;
struct IncludeT; struct PragmaT; struct TranslationUnitFileT;
struct CounterT; struct LanguageT; struct NodeT;

// Lightweight per-record object model used only while converting one native
// record to generated protobuf classes. It is deliberately not an AST store.
struct UnionValue {
  std::shared_ptr<void> object;
  std::type_index type = typeid(void);
  template <class T> void Set(T &&value) {
    using U = std::decay_t<T>;
    object = std::make_shared<U>(std::forward<T>(value));
    type = typeid(U);
  }
  template <class T> const T *get() const {
    return type == typeid(T) ? static_cast<const T *>(object.get()) : nullptr;
  }
};

enum class TopLevelKind : int32_t { Decl = 0, Type = 1, Attr = 2, };
enum class AttributeKind : int32_t { AddressSpace = 0, AnnotateType = 1, ArmIn = 2, ArmInOut = 3, ArmMveStrictPolymorphism = 4, ArmOut = 5, ArmPreserves = 6, ArmStreaming = 7, ArmStreamingCompatible = 8, BTFTypeTag = 9, CmseNSCall = 10, HLSLGroupSharedAddressSpace = 11, HLSLParamModifier = 12, NoDeref = 13, ObjCGC = 14, ObjCInertUnsafeUnretained = 15, ObjCKindOf = 16, OpenCLConstantAddressSpace = 17, OpenCLGenericAddressSpace = 18, OpenCLGlobalAddressSpace = 19, OpenCLGlobalDeviceAddressSpace = 20, OpenCLGlobalHostAddressSpace = 21, OpenCLLocalAddressSpace = 22, OpenCLPrivateAddressSpace = 23, Ptr32 = 24, Ptr64 = 25, SPtr = 26, TypeNonNull = 27, TypeNullUnspecified = 28, TypeNullable = 29, TypeNullableResult = 30, UPtr = 31, WebAssemblyFuncref = 32, CodeAlign = 33, FallThrough = 34, Likely = 35, MustTail = 36, OpenCLUnrollHint = 37, Unlikely = 38, AlwaysInline = 39, NoInline = 40, NoMerge = 41, Suppress = 42, AArch64SVEPcs = 43, AArch64VectorPcs = 44, AMDGPUKernelCall = 45, AcquireHandle = 46, AnyX86NoCfCheck = 47, CDecl = 48, FastCall = 49, IntelOclBicc = 50, LifetimeBound = 51, M68kRTD = 52, MSABI = 53, NSReturnsRetained = 54, ObjCOwnership = 55, Pascal = 56, Pcs = 57, PreserveAll = 58, PreserveMost = 59, RegCall = 60, StdCall = 61, SwiftAsyncCall = 62, SwiftCall = 63, SysVABI = 64, ThisCall = 65, VectorCall = 66, SwiftAsyncContext = 67, SwiftContext = 68, SwiftErrorResult = 69, SwiftIndirectResult = 70, Annotate = 71, CFConsumed = 72, CarriesDependency = 73, NSConsumed = 74, NonNull = 75, OSConsumed = 76, PassObjectSize = 77, ReleaseHandle = 78, UseHandle = 79, HLSLSV_DispatchThreadID = 80, HLSLSV_GroupIndex = 81, AMDGPUFlatWorkGroupSize = 82, AMDGPUNumSGPR = 83, AMDGPUNumVGPR = 84, AMDGPUWavesPerEU = 85, ARMInterrupt = 86, AVRInterrupt = 87, AVRSignal = 88, AcquireCapability = 89, AcquiredAfter = 90, AcquiredBefore = 91, AlignMac68k = 92, AlignNatural = 93, Aligned = 94, AllocAlign = 95, AllocSize = 96, AlwaysDestroy = 97, AnalyzerNoReturn = 98, AnyX86Interrupt = 99, AnyX86NoCallerSavedRegisters = 100, ArcWeakrefUnavailable = 101, ArgumentWithTypeTag = 102, ArmBuiltinAlias = 103, ArmLocallyStreaming = 104, ArmNew = 105, Artificial = 106, AsmLabel = 107, AssertCapability = 108, AssertExclusiveLock = 109, AssertSharedLock = 110, AssumeAligned = 111, Assumption = 112, Availability = 113, AvailableOnlyInDefaultEvalMethod = 114, BPFPreserveAccessIndex = 115, BPFPreserveStaticOffset = 116, BTFDeclTag = 117, Blocks = 118, Builtin = 119, C11NoReturn = 120, CFAuditedTransfer = 121, CFGuard = 122, CFICanonicalJumpTable = 123, CFReturnsNotRetained = 124, CFReturnsRetained = 125, CFUnknownTransfer = 126, CPUDispatch = 127, CPUSpecific = 128, CUDAConstant = 129, CUDADevice = 130, CUDADeviceBuiltinSurfaceType = 131, CUDADeviceBuiltinTextureType = 132, CUDAGlobal = 133, CUDAHost = 134, CUDAInvalidTarget = 135, CUDALaunchBounds = 136, CUDAShared = 137, CXX11NoReturn = 138, CallableWhen = 139, Callback = 140, Capability = 141, CapturedRecord = 142, Cleanup = 143, CmseNSEntry = 144, CodeModel = 145, CodeSeg = 146, Cold = 147, Common = 148, Const = 149, ConstInit = 150, Constructor = 151, Consumable = 152, ConsumableAutoCast = 153, ConsumableSetOnRead = 154, Convergent = 155, CoroDisableLifetimeBound = 156, CoroLifetimeBound = 157, CoroOnlyDestroyWhenComplete = 158, CoroReturnType = 159, CoroWrapper = 160, CountedBy = 161, DLLExport = 162, DLLExportStaticLocal = 163, DLLImport = 164, DLLImportStaticLocal = 165, Deprecated = 166, Destructor = 167, DiagnoseAsBuiltin = 168, DiagnoseIf = 169, DisableSanitizerInstrumentation = 170, DisableTailCalls = 171, EmptyBases = 172, EnableIf = 173, EnforceTCB = 174, EnforceTCBLeaf = 175, EnumExtensibility = 176, Error = 177, ExcludeFromExplicitInstantiation = 178, ExclusiveTrylockFunction = 179, ExternalSourceSymbol = 180, Final = 181, FlagEnum = 182, Flatten = 183, Format = 184, FormatArg = 185, FunctionReturnThunks = 186, GNUInline = 187, GuardedBy = 188, GuardedVar = 189, HIPManaged = 190, HLSLNumThreads = 191, HLSLResource = 192, HLSLResourceBinding = 193, HLSLShader = 194, Hot = 195, IBAction = 196, IBOutlet = 197, IBOutletCollection = 198, InitPriority = 199, InternalLinkage = 200, LTOVisibilityPublic = 201, LayoutVersion = 202, Leaf = 203, LockReturned = 204, LocksExcluded = 205, M68kInterrupt = 206, MIGServerRoutine = 207, MSAllocator = 208, MSConstexpr = 209, MSInheritance = 210, MSNoVTable = 211, MSP430Interrupt = 212, MSStruct = 213, MSVtorDisp = 214, MaxFieldAlignment = 215, MayAlias = 216, MaybeUndef = 217, MicroMips = 218, MinSize = 219, MinVectorWidth = 220, Mips16 = 221, MipsInterrupt = 222, MipsLongCall = 223, MipsShortCall = 224, NSConsumesSelf = 225, NSErrorDomain = 226, NSReturnsAutoreleased = 227, NSReturnsNotRetained = 228, NVPTXKernel = 229, Naked = 230, NoAlias = 231, NoCommon = 232, NoDebug = 233, NoDestroy = 234, NoDuplicate = 235, NoInstrumentFunction = 236, NoMicroMips = 237, NoMips16 = 238, NoProfileFunction = 239, NoRandomizeLayout = 240, NoReturn = 241, NoSanitize = 242, NoSpeculativeLoadHardening = 243, NoSplitStack = 244, NoStackProtector = 245, NoThreadSafetyAnalysis = 246, NoThrow = 247, NoUniqueAddress = 248, NoUwtable = 249, NotTailCalled = 250, OMPAllocateDecl = 251, OMPCaptureNoInit = 252, OMPDeclareTargetDecl = 253, OMPDeclareVariant = 254, OMPThreadPrivateDecl = 255, OSConsumesThis = 256, OSReturnsNotRetained = 257, OSReturnsRetained = 258, OSReturnsRetainedOnNonZero = 259, OSReturnsRetainedOnZero = 260, ObjCBridge = 261, ObjCBridgeMutable = 262, ObjCBridgeRelated = 263, ObjCException = 264, ObjCExplicitProtocolImpl = 265, ObjCExternallyRetained = 266, ObjCIndependentClass = 267, ObjCMethodFamily = 268, ObjCNSObject = 269, ObjCPreciseLifetime = 270, ObjCRequiresPropertyDefs = 271, ObjCRequiresSuper = 272, ObjCReturnsInnerPointer = 273, ObjCRootClass = 274, ObjCSubclassingRestricted = 275, OpenCLIntelReqdSubGroupSize = 276, OpenCLKernel = 277, OptimizeNone = 278, Override = 279, Owner = 280, Ownership = 281, Packed = 282, ParamTypestate = 283, PatchableFunctionEntry = 284, Pointer = 285, PragmaClangBSSSection = 286, PragmaClangDataSection = 287, PragmaClangRelroSection = 288, PragmaClangRodataSection = 289, PragmaClangTextSection = 290, PreferredName = 291, PreferredType = 292, PtGuardedBy = 293, PtGuardedVar = 294, Pure = 295, RISCVInterrupt = 296, RandomizeLayout = 297, ReadOnlyPlacement = 298, Reinitializes = 299, ReleaseCapability = 300, ReqdWorkGroupSize = 301, RequiresCapability = 302, Restrict = 303, Retain = 304, ReturnTypestate = 305, ReturnsNonNull = 306, ReturnsTwice = 307, SYCLKernel = 308, SYCLSpecialClass = 309, ScopedLockable = 310, Section = 311, SelectAny = 312, Sentinel = 313, SetTypestate = 314, SharedTrylockFunction = 315, SpeculativeLoadHardening = 316, StandaloneDebug = 317, StrictFP = 318, StrictGuardStackCheck = 319, SwiftAsync = 320, SwiftAsyncError = 321, SwiftAsyncName = 322, SwiftAttr = 323, SwiftBridge = 324, SwiftBridgedTypedef = 325, SwiftError = 326, SwiftImportAsNonGeneric = 327, SwiftImportPropertyAsAccessors = 328, SwiftName = 329, SwiftNewType = 330, SwiftPrivate = 331, TLSModel = 332, Target = 333, TargetClones = 334, TargetVersion = 335, TestTypestate = 336, TransparentUnion = 337, TrivialABI = 338, TryAcquireCapability = 339, TypeTagForDatatype = 340, TypeVisibility = 341, Unavailable = 342, Uninitialized = 343, UnsafeBufferUsage = 344, Unused = 345, Used = 346, UsingIfExists = 347, Uuid = 348, VecReturn = 349, VecTypeHint = 350, Visibility = 351, WarnUnused = 352, WarnUnusedResult = 353, Weak = 354, WeakImport = 355, WeakRef = 356, WebAssemblyExportName = 357, WebAssemblyImportModule = 358, WebAssemblyImportName = 359, WorkGroupSizeHint = 360, X86ForceAlignArgPointer = 361, XRayInstrument = 362, XRayLogArgs = 363, ZeroCallUsedRegs = 364, AbiTag = 365, Alias = 366, AlignValue = 367, BuiltinAlias = 368, CalledOnce = 369, IFunc = 370, InitSeg = 371, LoaderUninitialized = 372, LoopHint = 373, Mode = 374, NoBuiltin = 375, NoEscape = 376, OMPCaptureKind = 377, OMPDeclareSimdDecl = 378, OMPReferencedVar = 379, ObjCBoxable = 380, ObjCClassStub = 381, ObjCDesignatedInitializer = 382, ObjCDirect = 383, ObjCDirectMembers = 384, ObjCNonLazyClass = 385, ObjCNonRuntimeProtocol = 386, ObjCRuntimeName = 387, ObjCRuntimeVisible = 388, OpenCLAccess = 389, Overloadable = 390, RenderScriptKernel = 391, SwiftObjCMembers = 392, SwiftVersionedAddition = 393, SwiftVersionedRemoval = 394, Thread = 395, FirstAttr = 396, LastAttr = 397, FirstTypeAttr = 398, LastTypeAttr = 399, FirstStmtAttr = 400, LastStmtAttr = 401, FirstDeclOrStmtAttr = 402, LastDeclOrStmtAttr = 403, FirstInheritableAttr = 404, LastInheritableAttr = 405, FirstDeclOrTypeAttr = 406, LastDeclOrTypeAttr = 407, FirstInheritableParamAttr = 408, LastInheritableParamAttr = 409, FirstParameterABIAttr = 410, LastParameterABIAttr = 411, FirstHLSLAnnotationAttr = 412, LastHLSLAnnotationAttr = 413, };
enum class FormatAttrKind : int32_t { printf = 0, scanf = 1, strftime = 2, gnu_printf = 3, gnu_scanf = 4, gnu_strftime = 5, strfmon = 6, ms_printf = 7, ms_scanf = 8, ms_strftime = 9, NSString = 10, };
enum class VisibilityType : int32_t { Default = 0, Hidden = 1, Protected = 2, };
enum class ExplicitSpecKind : int32_t { ResolvedFalse = 0, ResolvedTrue = 1, Unresolved = 2, };
enum class InitializationStyle : int32_t { CINIT = 0, CALL_INIT = 1, LIST_INIT = 2, ParenListInit = 3, };
enum class LanguageId : int32_t { C = 0, CXX = 1, };
enum class Linkage : int32_t { Invalid = 0, None = 1, Internal = 2, UniqueExternal = 3, VisibleNone = 4, Module = 5, External = 6, };
enum class NameKind : int32_t { IDENTIFIER = 0, OBJ_C_ZERO_ARG_SELECTOR = 1, OBJ_C_ONE_ARG_SELECTOR = 2, OBJ_C_MULTI_ARG_SELECTOR = 3, CXX_CONSTRUCTOR_NAME = 4, CXX_DESTRUCTOR_NAME = 5, CXX_CONVERSION_FUNCTION_NAME = 6, CXX_DEDUCTION_GUIDE_NAME = 7, CXX_OPERATOR_NAME = 8, CXX_LITERAL_OPERATOR_NAME = 9, CXX_USING_DIRECTIVE = 10, };
enum class StorageClass : int32_t { None = 0, Extern = 1, Static = 2, PrivateExtern = 3, Auto = 4, Register = 5, };
enum class TemplateKind : int32_t { NON_TEMPLATE = 0, FUNCTION_TEMPLATE = 1, MEMBER_SPECIALIZATION = 2, FUNCTION_TEMPLATE_SPECIALIZATION = 3, DEPENDENT_FUNCTION_TEMPLATE_SPECIALIZATION = 4, };
enum class TemplateSpecializationKind : int32_t { Undeclared = 0, ImplicitInstantiation = 1, ExplicitSpecialization = 2, ExplicitInstantiationDeclaration = 3, ExplicitInstantiationDefinition = 4, };
enum class Visibility : int32_t { Hidden = 0, Protected = 1, Default = 2, };
enum class BinaryOperatorKind : int32_t { PtrMemD = 0, PtrMemI = 1, Mul = 2, Div = 3, Rem = 4, Add = 5, Sub = 6, Shl = 7, Shr = 8, Cmp = 9, LT = 10, GT = 11, LE = 12, GE = 13, EQ = 14, NE = 15, And = 16, Xor = 17, Or = 18, LAnd = 19, LOr = 20, Assign = 21, MulAssign = 22, DivAssign = 23, RemAssign = 24, AddAssign = 25, SubAssign = 26, ShlAssign = 27, ShrAssign = 28, AndAssign = 29, XorAssign = 30, OrAssign = 31, Comma = 32, };
enum class CharacterKind : int32_t { ASCII = 0, WIDE = 1, UTF8 = 2, UTF16 = 3, UTF32 = 4, };
enum class ConstructionKind : int32_t { Complete = 0, NonVirtualBase = 1, VirtualBase = 2, Delegating = 3, };
enum class LambdaCaptureDefault : int32_t { None = 0, ByCopy = 1, ByRef = 2, };
enum class LambdaCaptureKind : int32_t { This = 0, StarThis = 1, ByCopy = 2, ByRef = 3, VLAType = 4, };
enum class NewInitStyle : int32_t { NO_INIT = 0, CALL_INIT = 1, LIST_INIT = 2, };
enum class ObjectKind : int32_t { ORDINARY = 0, BIT_FIELD = 1, OBJ_C_PROPERTY = 2, OBJ_C_SUBSCRIPT = 3, VECTOR_COMPONENT = 4, };
enum class PredefinedIdType : int32_t { Func = 0, Function = 1, LFunction = 2, FuncDName = 3, FuncSig = 4, LFuncSig = 5, PrettyFunction = 6, PrettyFunctionNoVirtual = 7, };
enum class StringKind : int32_t { ORDINARY = 0, WIDE = 1, UTF8 = 2, UTF16 = 3, UTF32 = 4, UNEVALUATED = 5, };
enum class UnaryOperatorKind : int32_t { PostInc = 0, PostDec = 1, PreInc = 2, PreDec = 3, AddrOf = 4, Deref = 5, Plus = 6, Minus = 7, Not = 8, LNot = 9, Real = 10, Imag = 11, Extension = 12, Coawait = 13, };
enum class UnaryOperatorPosition : int32_t { PREFIX = 0, POSTFIX = 1, };
enum class ValueKind : int32_t { R_VALUE = 0, L_VALUE = 1, X_VALUE = 2, };
enum class AddressSpaceQualifierV2 : int32_t { NONE = 0, GLOBAL = 1, LOCAL = 2, CONSTANT = 3, GENERIC = 4, DEFAULT = 5, PRIVATE = 6, CUDA_CONSTANT = 7, CUDA_DEVICE = 8, CUDA_SHARED = 9, };
enum class ArraySizeModifier : int32_t { Normal = 0, Static = 1, Star = 2, };
enum class BuiltinKind : int32_t { OCLImage1dRO = 0, OCLImage1dArrayRO = 1, OCLImage1dBufferRO = 2, OCLImage2dRO = 3, OCLImage2dArrayRO = 4, OCLImage2dDepthRO = 5, OCLImage2dArrayDepthRO = 6, OCLImage2dMSAARO = 7, OCLImage2dArrayMSAARO = 8, OCLImage2dMSAADepthRO = 9, OCLImage2dArrayMSAADepthRO = 10, OCLImage3dRO = 11, OCLImage1dWO = 12, OCLImage1dArrayWO = 13, OCLImage1dBufferWO = 14, OCLImage2dWO = 15, OCLImage2dArrayWO = 16, OCLImage2dDepthWO = 17, OCLImage2dArrayDepthWO = 18, OCLImage2dMSAAWO = 19, OCLImage2dArrayMSAAWO = 20, OCLImage2dMSAADepthWO = 21, OCLImage2dArrayMSAADepthWO = 22, OCLImage3dWO = 23, OCLImage1dRW = 24, OCLImage1dArrayRW = 25, OCLImage1dBufferRW = 26, OCLImage2dRW = 27, OCLImage2dArrayRW = 28, OCLImage2dDepthRW = 29, OCLImage2dArrayDepthRW = 30, OCLImage2dMSAARW = 31, OCLImage2dArrayMSAARW = 32, OCLImage2dMSAADepthRW = 33, OCLImage2dArrayMSAADepthRW = 34, OCLImage3dRW = 35, OCLIntelSubgroupAVCMcePayload = 36, OCLIntelSubgroupAVCImePayload = 37, OCLIntelSubgroupAVCRefPayload = 38, OCLIntelSubgroupAVCSicPayload = 39, OCLIntelSubgroupAVCMceResult = 40, OCLIntelSubgroupAVCImeResult = 41, OCLIntelSubgroupAVCRefResult = 42, OCLIntelSubgroupAVCSicResult = 43, OCLIntelSubgroupAVCImeResultSingleRefStreamout = 44, OCLIntelSubgroupAVCImeResultDualRefStreamout = 45, OCLIntelSubgroupAVCImeSingleRefStreamin = 46, OCLIntelSubgroupAVCImeDualRefStreamin = 47, SveInt8 = 48, SveInt16 = 49, SveInt32 = 50, SveInt64 = 51, SveUint8 = 52, SveUint16 = 53, SveUint32 = 54, SveUint64 = 55, SveFloat16 = 56, SveFloat32 = 57, SveFloat64 = 58, SveBFloat16 = 59, SveInt8x2 = 60, SveInt16x2 = 61, SveInt32x2 = 62, SveInt64x2 = 63, SveUint8x2 = 64, SveUint16x2 = 65, SveUint32x2 = 66, SveUint64x2 = 67, SveFloat16x2 = 68, SveFloat32x2 = 69, SveFloat64x2 = 70, SveBFloat16x2 = 71, SveInt8x3 = 72, SveInt16x3 = 73, SveInt32x3 = 74, SveInt64x3 = 75, SveUint8x3 = 76, SveUint16x3 = 77, SveUint32x3 = 78, SveUint64x3 = 79, SveFloat16x3 = 80, SveFloat32x3 = 81, SveFloat64x3 = 82, SveBFloat16x3 = 83, SveInt8x4 = 84, SveInt16x4 = 85, SveInt32x4 = 86, SveInt64x4 = 87, SveUint8x4 = 88, SveUint16x4 = 89, SveUint32x4 = 90, SveUint64x4 = 91, SveFloat16x4 = 92, SveFloat32x4 = 93, SveFloat64x4 = 94, SveBFloat16x4 = 95, SveBool = 96, VectorQuad = 97, VectorPair = 98, RvvInt8mf8 = 99, RvvInt8mf4 = 100, RvvInt8mf2 = 101, RvvInt8m1 = 102, RvvInt8m2 = 103, RvvInt8m4 = 104, RvvInt8m8 = 105, RvvUint8mf8 = 106, RvvUint8mf4 = 107, RvvUint8mf2 = 108, RvvUint8m1 = 109, RvvUint8m2 = 110, RvvUint8m4 = 111, RvvUint8m8 = 112, RvvInt16mf4 = 113, RvvInt16mf2 = 114, RvvInt16m1 = 115, RvvInt16m2 = 116, RvvInt16m4 = 117, RvvInt16m8 = 118, RvvUint16mf4 = 119, RvvUint16mf2 = 120, RvvUint16m1 = 121, RvvUint16m2 = 122, RvvUint16m4 = 123, RvvUint16m8 = 124, RvvInt32mf2 = 125, RvvInt32m1 = 126, RvvInt32m2 = 127, RvvInt32m4 = 128, RvvInt32m8 = 129, RvvUint32mf2 = 130, RvvUint32m1 = 131, RvvUint32m2 = 132, RvvUint32m4 = 133, RvvUint32m8 = 134, RvvInt64m1 = 135, RvvInt64m2 = 136, RvvInt64m4 = 137, RvvInt64m8 = 138, RvvUint64m1 = 139, RvvUint64m2 = 140, RvvUint64m4 = 141, RvvUint64m8 = 142, RvvFloat16mf4 = 143, RvvFloat16mf2 = 144, RvvFloat16m1 = 145, RvvFloat16m2 = 146, RvvFloat16m4 = 147, RvvFloat16m8 = 148, RvvFloat32mf2 = 149, RvvFloat32m1 = 150, RvvFloat32m2 = 151, RvvFloat32m4 = 152, RvvFloat32m8 = 153, RvvFloat64m1 = 154, RvvFloat64m2 = 155, RvvFloat64m4 = 156, RvvFloat64m8 = 157, RvvBool1 = 158, RvvBool2 = 159, RvvBool4 = 160, RvvBool8 = 161, RvvBool16 = 162, RvvBool32 = 163, RvvBool64 = 164, Void = 165, Bool = 166, Char_U = 167, UChar = 168, WChar_U = 169, Char8 = 170, Char16 = 171, Char32 = 172, UShort = 173, UInt = 174, ULong = 175, ULongLong = 176, UInt128 = 177, Char_S = 178, SChar = 179, WChar_S = 180, Short = 181, Int = 182, Long = 183, LongLong = 184, Int128 = 185, ShortAccum = 186, Accum = 187, LongAccum = 188, UShortAccum = 189, UAccum = 190, ULongAccum = 191, ShortFract = 192, Fract = 193, LongFract = 194, UShortFract = 195, UFract = 196, ULongFract = 197, SatShortAccum = 198, SatAccum = 199, SatLongAccum = 200, SatUShortAccum = 201, SatUAccum = 202, SatULongAccum = 203, SatShortFract = 204, SatFract = 205, SatLongFract = 206, SatUShortFract = 207, SatUFract = 208, SatULongFract = 209, Half = 210, Float = 211, Double = 212, LongDouble = 213, Float16 = 214, BFloat16 = 215, Float128 = 216, Ibm128 = 217, NullPtr = 218, ObjCId = 219, ObjCClass = 220, ObjCSel = 221, OCLSampler = 222, OCLEvent = 223, OCLClkEvent = 224, OCLQueue = 225, OCLReserveID = 226, Dependent = 227, Overload = 228, BoundMember = 229, PseudoObject = 230, UnknownAny = 231, BuiltinFn = 232, ARCUnbridgedCast = 233, IncompleteMatrixIdx = 234, OMPArraySection = 235, OMPArrayShaping = 236, OMPIterator = 237, };
enum class C99Qualifier : int32_t { CONST = 0, RESTRICT = 1, RESTRICT_C99 = 2, VOLATILE = 3, };
enum class CallingConvention : int32_t { C = 0, X86StdCall = 1, X86FastCall = 2, X86ThisCall = 3, X86VectorCall = 4, X86Pascal = 5, Win64 = 6, X86_64SysV = 7, X86RegCall = 8, AAPCS = 9, AAPCS_VFP = 10, IntelOclBicc = 11, SpirFunction = 12, OpenCLKernel = 13, Swift = 14, SwiftAsync = 15, PreserveMost = 16, PreserveAll = 17, AArch64VectorCall = 18, AArch64SVEPCS = 19, AMDGPUKernelCall = 20, M68kRTD = 21, };
enum class ElaboratedTypeKeyword : int32_t { Struct = 0, Interface = 1, Union = 2, Class = 3, Enum = 4, Typename = 5, None = 6, };
enum class ExceptionSpecificationType : int32_t { None = 0, DynamicNone = 1, Dynamic = 2, MSAny = 3, NoThrow = 4, BasicNoexcept = 5, DependentNoexcept = 6, NoexceptFalse = 7, NoexceptTrue = 8, Unevaluated = 9, Uninstantiated = 10, Unparsed = 11, };
enum class TypeDependency : int32_t { DEPENDENT = 0, INSTANTIATION_DEPENDENT = 1, NONE = 2, };
enum class UnaryTransformTypeKind : int32_t { AddLvalueReference = 0, AddPointer = 1, AddRvalueReference = 2, Decay = 3, MakeSigned = 4, MakeUnsigned = 5, RemoveAllExtents = 6, RemoveConst = 7, RemoveCV = 8, RemoveCVRef = 9, RemoveExtent = 10, RemovePointer = 11, RemoveReference = 12, RemoveRestrict = 13, RemoveVolatile = 14, EnumUnderlyingType = 15, };
enum class AccessSpecifier : int32_t { PUBLIC = 0, PROTECTED = 1, PRIVATE = 2, NONE = 3, };
enum class CastKind : int32_t { Dependent = 0, BitCast = 1, LValueBitCast = 2, LValueToRValueBitCast = 3, LValueToRValue = 4, NoOp = 5, BaseToDerived = 6, DerivedToBase = 7, UncheckedDerivedToBase = 8, Dynamic = 9, ToUnion = 10, ArrayToPointerDecay = 11, FunctionToPointerDecay = 12, NullToPointer = 13, NullToMemberPointer = 14, BaseToDerivedMemberPointer = 15, DerivedToBaseMemberPointer = 16, MemberPointerToBoolean = 17, ReinterpretMemberPointer = 18, UserDefinedConversion = 19, ConstructorConversion = 20, IntegralToPointer = 21, PointerToIntegral = 22, PointerToBoolean = 23, ToVoid = 24, MatrixCast = 25, VectorSplat = 26, IntegralCast = 27, IntegralToBoolean = 28, IntegralToFloating = 29, FloatingToFixedPoint = 30, FixedPointToFloating = 31, FixedPointCast = 32, FixedPointToIntegral = 33, IntegralToFixedPoint = 34, FixedPointToBoolean = 35, FloatingToIntegral = 36, FloatingToBoolean = 37, BooleanToSignedIntegral = 38, FloatingCast = 39, CPointerToObjCPointerCast = 40, BlockPointerToObjCPointerCast = 41, AnyPointerToBlockPointerCast = 42, ObjCObjectLValueCast = 43, FloatingRealToComplex = 44, FloatingComplexToReal = 45, FloatingComplexToBoolean = 46, FloatingComplexCast = 47, FloatingComplexToIntegralComplex = 48, IntegralRealToComplex = 49, IntegralComplexToReal = 50, IntegralComplexToBoolean = 51, IntegralComplexCast = 52, IntegralComplexToFloatingComplex = 53, ARCProduceObject = 54, ARCConsumeObject = 55, ARCReclaimReturnedObject = 56, ARCExtendBlockObject = 57, AtomicToNonAtomic = 58, NonAtomicToAtomic = 59, CopyAndAutoreleaseBlockObject = 60, BuiltinFnToFnPtr = 61, ZeroToOCLOpaqueType = 62, AddressSpaceConversion = 63, IntToOCLSampler = 64, };
enum class ReferenceQualifier : int32_t { LValue = 0, RValue = 1, None = 2, };
enum class TLSKind : int32_t { NONE = 0, STATIC = 1, DYNAMIC = 2, };
enum class TagKind : int32_t { STRUCT = 0, INTERFACE = 1, UNION = 2, CLASS = 3, ENUM = 4, NO_KIND = 5, };
enum class TemplateTypeParmKind : int32_t { TYPENAME = 0, CLASS = 1, };
enum class UnaryExprOrTypeTrait : int32_t { SizeOf = 0, AlignOf = 1, PreferredAlignOf = 2, VecStep = 3, OpenMPRequiredSimdAlign = 4, };
enum class EnumScopeType : int32_t { CLASS = 0, STRUCT = 1, NO_SCOPE = 2, };

struct RangeT {
  uint32_t file{};
  uint32_t line{};
  uint32_t column{};
  uint32_t end_file{};
  uint32_t end_line{};
  uint32_t end_column{};
};
struct SourceInfoT {
  std::unique_ptr<RangeT> expansion{};
  bool is_macro{};
  std::unique_ptr<RangeT> spelling{};
  bool system_header{};
};
struct NodeDataT {
  std::unique_ptr<SourceInfoT> source{};
};
struct CXXBaseSpecifierT {
  bool is_virtual{};
  bool is_pack_expansion{};
  AccessSpecifier access_specifier_as_written{};
  AccessSpecifier access_specifier_semantic{};
  int64_t type{};
};
struct ExplicitSpecifierT {
  ExplicitSpecKind kind{};
  int64_t expr{};
  bool is_specified{};
};
struct TemplateDeclarationT {
  int64_t decl{};
};
struct TemplateNullPtrT {
  int64_t type{};
};
struct TemplateTypeT {
  int64_t type{};
};
struct TemplateExpressionT {
  int64_t expr{};
};
struct TemplatePackT {
  std::vector<std::unique_ptr<TemplateArgumentT>> arguments;
};
struct TemplateIntegralT {
  std::string integral{};
};
struct TemplateExpansionT {
  uint32_t num_expansions{};
  std::unique_ptr<TemplateNameT> template_name{};
};
struct TemplateStructuralValueT {
  int64_t type{};
};
struct TemplateArgumentT {
  UnionValue value;
};
struct DirectTemplateNameT {
  int64_t template_decl{};
};
struct QualifiedTemplateNameT {
  std::string qualifier{};
  bool has_template_keyword{};
  int64_t template_decl{};
};
struct SubstitutedTemplateNameT {
  int64_t parameter{};
  std::unique_ptr<TemplateNameT> replacement{};
};
struct UsingTemplateNameT {
  int64_t using_shadow_decl{};
};
struct DependentTemplateNameT {
  std::string qualifier{};
  std::string name{};
};
struct TemplateNameT {
  UnionValue value;
};
struct AnyMemberInitializerT {
  int64_t any_member_decl{};
};
struct BaseInitializerT {
  int64_t base_class{};
};
struct DelegatingInitializerT {
  int64_t delegated_type{};
};
struct CXXCtorInitializerT {
  UnionValue target;
  int64_t init_expr{};
  bool is_in_class_member_initializer{};
  bool is_written{};
};
struct NoExceptionDetailsT {
};
struct ComputedExceptionDetailsT {
  int64_t noexcept_expr{};
};
struct UnevaluatedExceptionDetailsT {
  int64_t source_decl{};
};
struct UninstantiatedExceptionDetailsT {
  int64_t source_decl{};
  int64_t source_template{};
};
struct ExceptionSpecificationT {
  ExceptionSpecificationType kind{};
  std::vector<int64_t> exception_types;
  UnionValue details;
};
struct OffsetArrayT {
  int64_t expr{};
};
struct OffsetFieldT {
  std::string field_name{};
};
struct OffsetIdentifierT {
  std::string field_name{};
};
struct OffsetBaseT {
  int64_t type{};
};
struct OffsetOfComponentT {
  UnionValue value;
};
struct FieldDesignatorT {
  std::string field_name{};
};
struct ArrayDesignatorT {
  int32_t index{};
};
struct ArrayRangeDesignatorT {
  int32_t index{};
};
struct DesignatorT {
  UnionValue value;
};
struct AsmInputT {
  int64_t expr{};
  std::string constraint{};
};
struct AsmOutputT {
  int64_t expr{};
  std::string constraint{};
  bool is_plus_constraint{};
};
struct NamespaceSpecifierT {
  int64_t namespace_decl{};
};
struct NamespaceAliasSpecifierT {
  int64_t namespace_alias{};
};
struct TypeSpecifierT {
  int64_t type{};
};
struct TypeWithTemplateSpecifierT {
  int64_t type{};
};
struct GlobalSpecifierT {
};
struct SuperSpecifierT {
  int64_t super_decl{};
};
struct NestedNameSpecifierT {
  UnionValue value;
};
struct DeclDataT {
  std::unique_ptr<NodeDataT> base{};
  bool is_implicit{};
  bool is_used{};
  bool is_referenced{};
  bool is_invalid_decl{};
  bool is_module_private{};
  std::vector<int64_t> attributes;
};
struct NamedDeclDataT {
  std::unique_ptr<DeclDataT> base{};
  std::string qualified_prefix{};
  std::string decl_name{};
  NameKind name_kind{};
  bool is_cxx_class_member{};
  bool is_cxx_instance_member{};
  Linkage linkage{};
  Visibility visibility{};
};
struct TypeDeclDataT {
  std::unique_ptr<NamedDeclDataT> base{};
  int64_t type_for_decl{};
};
struct TagDeclDataT {
  std::unique_ptr<TypeDeclDataT> base{};
  TagKind tag_kind{};
  bool is_complete_definition{};
};
struct RecordDeclDataT {
  std::unique_ptr<TagDeclDataT> base{};
  bool is_anonymous{};
};
struct ValueDeclDataT {
  std::unique_ptr<NamedDeclDataT> base{};
  int64_t type{};
  bool is_weak{};
};
struct DeclaratorDeclDataT {
  std::unique_ptr<ValueDeclDataT> base{};
};
struct TemplateDeclDataT {
  std::unique_ptr<NamedDeclDataT> base{};
  std::vector<int64_t> template_parameters;
  int64_t templated_decl{};
};
struct FunctionDeclDataT {
  std::unique_ptr<DeclaratorDeclDataT> base{};
  bool is_constexpr{};
  TemplateKind template_kind{};
  StorageClass storage_class{};
  bool is_inline_specified{};
  bool is_virtual_as_written{};
  bool is_pure{};
  bool is_deleted{};
  bool is_explicitly_defaulted{};
  int64_t previous_decl{};
  int64_t canonical_decl{};
  int64_t primary_template_decl{};
  std::vector<std::unique_ptr<TemplateArgumentT>> template_arguments;
};
struct CXXMethodDeclDataT {
  std::unique_ptr<FunctionDeclDataT> base{};
  int64_t record{};
  std::vector<int64_t> overridden_methods;
  bool is_static{};
  bool is_instance{};
  bool is_const{};
  bool is_volatile{};
  bool is_virtual{};
  bool is_copy_assignment_operator{};
  bool is_move_assignment_operator{};
  int64_t this_type{};
  int64_t this_object_type{};
  bool has_inline_body{};
  bool is_lambda_static_invoker{};
};
struct CXXConstructorDeclDataT {
  std::unique_ptr<CXXMethodDeclDataT> base{};
  std::vector<std::unique_ptr<CXXCtorInitializerT>> constructor_inits;
  bool is_default_constructor{};
  bool is_explicit{};
  std::unique_ptr<ExplicitSpecifierT> explicit_specifier{};
};
struct CXXConversionDeclDataT {
  std::unique_ptr<CXXMethodDeclDataT> base{};
  bool is_explicit{};
  bool is_lambda_to_block_pointer_conversion{};
  int64_t conversion_type{};
};
struct FieldDeclDataT {
  std::unique_ptr<DeclaratorDeclDataT> base{};
  bool is_mutable{};
};
struct ParmVarDeclDataT {
  std::unique_ptr<VarDeclDataT> base{};
  bool has_inherited_default_arg{};
};
struct VarDeclDataT {
  std::unique_ptr<ValueDeclDataT> base{};
  StorageClass storage_class{};
  TLSKind tls_kind{};
  bool is_nrvo_variable{};
  InitializationStyle init_style{};
  bool is_constexpr{};
  bool is_static_data_member{};
  bool is_out_of_line{};
  bool has_global_storage{};
};
struct TemplateTypeParmDeclDataT {
  std::unique_ptr<TypeDeclDataT> base{};
  TemplateTypeParmKind kind{};
  bool is_parameter_pack{};
  int64_t default_argument{};
};
struct UnresolvedUsingTypenameDeclDataT {
  std::unique_ptr<TypeDeclDataT> base{};
  std::string qualifier{};
  bool is_pack_expansion{};
};
struct EnumDeclDataT {
  std::unique_ptr<TagDeclDataT> base{};
  EnumScopeType enum_scope_kind{};
  int64_t integer_type{};
};
struct CXXRecordDeclDataT {
  std::unique_ptr<RecordDeclDataT> base{};
  std::vector<std::unique_ptr<CXXBaseSpecifierT>> record_bases;
  int64_t record_definition{};
};
struct ClassTemplateSpecializationDeclDataT {
  std::unique_ptr<CXXRecordDeclDataT> base{};
  int64_t specialized_template{};
  TemplateSpecializationKind specialization_kind{};
  std::vector<std::unique_ptr<TemplateArgumentT>> template_arguments;
};
struct NonTypeTemplateParmDeclDataT {
  std::unique_ptr<DeclaratorDeclDataT> base{};
  int64_t default_argument{};
  bool default_argument_was_inherited{};
  bool is_parameter_pack{};
  bool is_pack_expansion{};
  bool is_expanded_parameter_pack{};
  std::vector<int64_t> expansion_types;
};
struct TypedefNameDeclDataT {
  std::unique_ptr<TypeDeclDataT> base{};
  int64_t underlying_type{};
};
struct AccessSpecDeclDataT {
  std::unique_ptr<DeclDataT> base{};
  AccessSpecifier access_specifier{};
};
struct UsingDeclDataT {
  std::unique_ptr<NamedDeclDataT> base{};
  std::unique_ptr<NestedNameSpecifierT> nested_name_specifier{};
};
struct UsingDirectiveDeclDataT {
  std::unique_ptr<NamedDeclDataT> base{};
  std::string qualifier{};
  int64_t namespace_{};
  int64_t namespace_as_written{};
};
struct NamespaceDeclDataT {
  std::unique_ptr<NamedDeclDataT> base{};
  std::string source_literal{};
};
struct NamespaceAliasDeclDataT {
  std::unique_ptr<NamedDeclDataT> base{};
  std::string nested_prefix{};
  int64_t aliased_namespace{};
};
struct LinkageSpecDeclDataT {
  std::unique_ptr<DeclDataT> base{};
  LanguageId linkage_type{};
};
struct StaticAssertDeclDataT {
  std::unique_ptr<DeclDataT> base{};
  bool is_failed{};
};
struct TemplateTemplateParmDeclDataT {
  std::unique_ptr<TemplateDeclDataT> base{};
  std::unique_ptr<TemplateArgumentT> default_argument{};
  bool is_parameter_pack{};
  bool is_pack_expansion{};
  bool is_expanded_parameter_pack{};
};
struct MSPropertyDeclDataT {
  std::unique_ptr<DeclaratorDeclDataT> base{};
  std::string getter_name{};
  std::string setter_name{};
};
struct ClassTemplatePartialSpecializationDeclDataT {
  std::unique_ptr<ClassTemplateSpecializationDeclDataT> base{};
};
struct TypeDataT {
  std::string type_as_string{};
  TypeDependency type_dependency{};
  bool is_variably_modified{};
  bool contains_unexpanded_parameter_pack{};
  bool is_from_ast{};
  int64_t unqualified_desugared_type{};
};
struct QualTypeDataT {
  std::unique_ptr<TypeDataT> base{};
  std::vector<C99Qualifier> c99_qualifiers;
  AddressSpaceQualifierV2 address_space_qualifier{};
  int64_t address_space{};
  int64_t unqualified_type{};
};
struct BuiltinTypeDataT {
  std::unique_ptr<TypeDataT> base{};
  BuiltinKind kind{};
  std::string kind_literal{};
};
struct PointerTypeDataT {
  std::unique_ptr<TypeDataT> base{};
  int64_t pointee_type{};
};
struct FunctionTypeDataT {
  std::unique_ptr<TypeDataT> base{};
  bool is_const{};
  bool is_volatile{};
  bool is_restrict{};
  bool no_return{};
  bool produces_result{};
  bool uses_reg_parm{};
  int64_t reg_parm{};
  CallingConvention calling_convention{};
  int64_t return_type{};
};
struct FunctionProtoTypeDataT {
  std::unique_ptr<FunctionTypeDataT> base{};
  int32_t num_parameters{};
  std::vector<int64_t> parameters_types;
  bool has_trailing_returns{};
  bool is_variadic{};
  ReferenceQualifier reference_qualifier{};
  std::unique_ptr<ExceptionSpecificationT> exception_specification{};
};
struct ArrayTypeDataT {
  std::unique_ptr<TypeDataT> base{};
  ArraySizeModifier array_size_modifier{};
  std::vector<C99Qualifier> index_type_qualifiers;
  int64_t element_type{};
};
struct ConstantArrayTypeDataT {
  std::unique_ptr<ArrayTypeDataT> base{};
  std::string array_size{};
};
struct VariableArrayTypeDataT {
  std::unique_ptr<ArrayTypeDataT> base{};
  int64_t size_expr{};
};
struct DependentSizedArrayTypeDataT {
  std::unique_ptr<ArrayTypeDataT> base{};
  int64_t size_expr{};
};
struct TagTypeDataT {
  std::unique_ptr<TypeDataT> base{};
  int64_t decl{};
};
struct TypeWithKeywordDataT {
  std::unique_ptr<TypeDataT> base{};
  ElaboratedTypeKeyword elaborated_type_keyword{};
};
struct ElaboratedTypeDataT {
  std::unique_ptr<TypeWithKeywordDataT> base{};
  std::string qualifier{};
  int64_t named_type{};
};
struct TemplateTypeParmTypeDataT {
  std::unique_ptr<TypeDataT> base{};
  int32_t depth{};
  int32_t index{};
  bool is_packed{};
  int64_t decl{};
};
struct TemplateSpecializationTypeDataT {
  std::unique_ptr<TypeDataT> base{};
  bool is_type_alias{};
  int64_t aliased_type{};
  std::string template_name{};
  int64_t template_decl{};
  std::vector<std::unique_ptr<TemplateArgumentT>> template_arguments;
};
struct TypedefTypeDataT {
  std::unique_ptr<TypeDataT> base{};
  int64_t decl{};
};
struct AdjustedTypeDataT {
  std::unique_ptr<TypeDataT> base{};
  int64_t original_type{};
  int64_t adjusted_type{};
};
struct DecayedTypeDataT {
  std::unique_ptr<AdjustedTypeDataT> base{};
  int64_t decayed_type{};
  int64_t pointee_type{};
};
struct DecltypeTypeDataT {
  std::unique_ptr<TypeDataT> base{};
  bool is_sugared{};
  int64_t underlying_expr{};
};
struct AutoTypeDataT {
  std::unique_ptr<TypeDataT> base{};
  int64_t deduced_type{};
};
struct ReferenceTypeDataT {
  std::unique_ptr<TypeDataT> base{};
  int64_t pointee_type_as_written{};
};
struct TypeOfExprTypeDataT {
  std::unique_ptr<TypeDataT> base{};
  bool is_sugared{};
  int64_t underlying_expr{};
};
struct PackExpansionTypeDataT {
  std::unique_ptr<TypeDataT> base{};
  int32_t num_expansions{};
  int64_t pattern{};
};
struct UnaryTransformTypeDataT {
  std::unique_ptr<TypeDataT> base{};
  UnaryTransformTypeKind kind{};
  int64_t underlying_type{};
  int64_t base_type{};
};
struct AttributedTypeDataT {
  std::unique_ptr<TypeDataT> base{};
  int64_t modified_type{};
  int64_t equivalent_type{};
};
struct SubstTemplateTypeParmTypeDataT {
  std::unique_ptr<TypeDataT> base{};
  int64_t replaced_parameter{};
  int64_t replacement_type{};
};
struct ComplexTypeDataT {
  std::unique_ptr<TypeDataT> base{};
  int64_t element_type{};
};
struct ExprDataT {
  std::unique_ptr<StmtDataT> base{};
  int64_t type{};
  ValueKind value_kind{};
  ObjectKind object_kind{};
  bool is_default_argument{};
};
struct CastExprDataT {
  std::unique_ptr<ExprDataT> base{};
  CastKind cast_kind{};
};
struct LiteralDataT {
  std::unique_ptr<ExprDataT> base{};
  std::string source_literal{};
};
struct CharacterLiteralDataT {
  std::unique_ptr<LiteralDataT> base{};
  int64_t value{};
  CharacterKind kind{};
};
struct IntegerLiteralDataT {
  std::unique_ptr<LiteralDataT> base{};
  std::string value{};
};
struct FloatingLiteralDataT {
  std::unique_ptr<LiteralDataT> base{};
  double value{};
};
struct StringLiteralDataT {
  std::unique_ptr<LiteralDataT> base{};
  StringKind string_kind{};
  int64_t length{};
  int32_t char_byte_width{};
  std::vector<uint8_t> string_bytes;
};
struct CXXBoolLiteralExprDataT {
  std::unique_ptr<LiteralDataT> base{};
  bool value{};
};
struct CompoundLiteralExprDataT {
  std::unique_ptr<LiteralDataT> base{};
  bool is_file_scope{};
};
struct InitListExprDataT {
  std::unique_ptr<ExprDataT> base{};
  int64_t array_filler{};
  bool is_explicit{};
  bool is_string_literal_init{};
  int64_t syntactic_form{};
  int64_t semantic_form{};
};
struct DeclRefExprDataT {
  std::unique_ptr<ExprDataT> base{};
  std::string qualifier{};
  std::vector<std::unique_ptr<TemplateArgumentT>> template_arguments;
  int64_t decl{};
};
struct OverloadExprDataT {
  std::unique_ptr<ExprDataT> base{};
  std::string qualifier{};
  std::string name{};
  std::vector<int64_t> unresolved_decls;
  std::vector<std::unique_ptr<TemplateArgumentT>> template_arguments;
};
struct CXXConstructExprDataT {
  std::unique_ptr<ExprDataT> base{};
  bool is_elidable{};
  bool requires_zero_initialization{};
  bool is_list_initialization{};
  bool is_std_list_initialization{};
  ConstructionKind construction_kind{};
  bool is_temporary_object{};
  int64_t constructor_decl{};
};
struct CXXTemporaryObjectExprDataT {
  std::unique_ptr<CXXConstructExprDataT> base{};
};
struct MemberExprDataT {
  std::unique_ptr<ExprDataT> base{};
  bool is_arrow{};
  std::string member_name{};
  int64_t member_decl{};
  int64_t found_decl{};
  AccessSpecifier found_decl_access_specifier{};
};
struct MaterializeTemporaryExprDataT {
  std::unique_ptr<ExprDataT> base{};
  int64_t extending_decl{};
};
struct BinaryOperatorDataT {
  std::unique_ptr<ExprDataT> base{};
  BinaryOperatorKind op{};
};
struct UnresolvedMemberExprDataT {
  std::unique_ptr<OverloadExprDataT> base{};
};
struct UnresolvedLookupExprDataT {
  std::unique_ptr<OverloadExprDataT> base{};
  bool requires_adl{};
};
struct CallExprDataT {
  std::unique_ptr<ExprDataT> base{};
  int64_t direct_callee{};
};
struct CXXMemberCallExprDataT {
  std::unique_ptr<CallExprDataT> base{};
  int64_t method_decl{};
};
struct CXXTypeidExprDataT {
  std::unique_ptr<ExprDataT> base{};
  bool is_type_operand{};
  int64_t operand{};
};
struct ExplicitCastExprDataT {
  std::unique_ptr<CastExprDataT> base{};
  int64_t type_as_written{};
};
struct CXXNamedCastExprDataT {
  std::unique_ptr<ExplicitCastExprDataT> base{};
  std::string cast_name{};
};
struct CXXDependentScopeMemberExprDataT {
  std::unique_ptr<ExprDataT> base{};
  bool is_arrow{};
  std::string member_name{};
  bool is_implicit_access{};
  std::string qualifier{};
  bool has_template_keyword{};
  std::vector<std::unique_ptr<TemplateArgumentT>> template_arguments;
};
struct UnaryOperatorDataT {
  std::unique_ptr<ExprDataT> base{};
  UnaryOperatorKind op{};
  UnaryOperatorPosition position{};
};
struct UnaryExprOrTypeTraitExprDataT {
  std::unique_ptr<ExprDataT> base{};
  UnaryExprOrTypeTrait kind{};
  bool is_argument_type{};
  int64_t arg_type{};
  std::string source_literal{};
};
struct CXXNewExprDataT {
  std::unique_ptr<ExprDataT> base{};
  bool is_global{};
  bool is_array{};
  bool initialization_present{};
  NewInitStyle init_style{};
  int64_t initializer{};
  int64_t construct_expr{};
  int64_t array_size{};
  int64_t operator_new{};
};
struct CXXDeleteExprDataT {
  std::unique_ptr<ExprDataT> base{};
  bool is_global{};
  bool is_array{};
  bool is_array_as_written{};
};
struct OffsetOfExprDataT {
  std::unique_ptr<ExprDataT> base{};
  int64_t source_type{};
  std::vector<std::unique_ptr<OffsetOfComponentT>> components;
};
struct LambdaExprDataT {
  std::unique_ptr<ExprDataT> base{};
  bool is_generic_lambda{};
  bool is_mutable{};
  bool has_explicit_parameters{};
  bool has_explicit_result_type{};
  LambdaCaptureDefault capture_default{};
  int64_t lambda_class{};
  std::vector<LambdaCaptureKind> capture_kinds;
};
struct PredefinedExprDataT {
  std::unique_ptr<ExprDataT> base{};
  PredefinedIdType predefined_type{};
};
struct SizeOfPackExprDataT {
  std::unique_ptr<ExprDataT> base{};
  bool is_partially_substituted{};
  int64_t pack{};
  std::vector<std::unique_ptr<TemplateArgumentT>> partial_arguments;
};
struct ArrayInitLoopExprDataT {
  std::unique_ptr<ExprDataT> base{};
};
struct DesignatedInitExprDataT {
  std::unique_ptr<ExprDataT> base{};
  bool uses_gnu_syntax{};
  std::vector<std::unique_ptr<DesignatorT>> designators;
};
struct DependentScopeDeclRefExprDataT {
  std::unique_ptr<ExprDataT> base{};
  std::string decl_name{};
  std::string qualifier{};
  bool has_template_keyword{};
  std::vector<std::unique_ptr<TemplateArgumentT>> template_arguments;
};
struct CXXNoexceptExprDataT {
  std::unique_ptr<ExprDataT> base{};
  bool value{};
};
struct CXXPseudoDestructorExprDataT {
  std::unique_ptr<ExprDataT> base{};
  std::string qualifier{};
  bool is_arrow{};
  int64_t destroyed_type{};
};
struct PseudoObjectExprDataT {
  std::unique_ptr<ExprDataT> base{};
  int32_t result_expr_index{};
};
struct MSPropertyRefExprDataT {
  std::unique_ptr<ExprDataT> base{};
  int64_t base_expr{};
  int64_t property_decl{};
  bool is_implicit_access{};
  bool is_arrow{};
};
struct StmtDataT {
  std::unique_ptr<NodeDataT> base{};
};
struct LabelStmtDataT {
  std::unique_ptr<StmtDataT> base{};
  int64_t label{};
};
struct GotoStmtDataT {
  std::unique_ptr<StmtDataT> base{};
  int64_t label{};
};
struct AttributedStmtDataT {
  std::unique_ptr<StmtDataT> base{};
  std::vector<int64_t> stmt_attributes;
};
struct AsmStmtDataT {
  std::unique_ptr<StmtDataT> base{};
  bool is_simple{};
  bool is_volatile{};
  std::vector<std::string> clobbers;
  std::vector<std::unique_ptr<AsmOutputT>> outputs;
  std::vector<std::unique_ptr<AsmInputT>> inputs;
};
struct GCCAsmStmtDataT {
  std::unique_ptr<AsmStmtDataT> base{};
  std::string asm_string{};
};
struct MSAsmStmtDataT {
  std::unique_ptr<AsmStmtDataT> base{};
  std::string asm_string{};
};
struct AttributeDataT {
  std::unique_ptr<NodeDataT> base{};
  AttributeKind kind{};
  bool is_implicit{};
  bool is_inherited{};
  bool is_late_parsed{};
  bool is_pack_expansion{};
};
struct AlignedAttrDataT {
  std::unique_ptr<AttributeDataT> base{};
  std::string spelling{};
  bool is_expression{};
  int64_t alignment{};
};
struct OpenCLUnrollHintAttrDataT {
  std::unique_ptr<AttributeDataT> base{};
  int32_t unroll_hint{};
};
struct FormatAttrDataT {
  std::unique_ptr<AttributeDataT> base{};
  FormatAttrKind type{};
  int32_t format_index{};
  int32_t first_arg{};
};
struct NonNullAttrDataT {
  std::unique_ptr<AttributeDataT> base{};
  std::vector<int32_t> arguments;
};
struct VisibilityAttrDataT {
  std::unique_ptr<AttributeDataT> base{};
  VisibilityType visibility_type{};
};
struct FileT {
  uint32_t id{};
  std::string path{};
};
struct ChildrenT {
  int64_t node{};
  std::vector<int64_t> children;
};
struct NodeClassT {
  int64_t node{};
  std::string class_name{};
};
struct TopLevelT {
  TopLevelKind kind{};
  int64_t node{};
};
struct IncludeT {
  std::string source{};
  std::string name{};
  uint32_t line{};
  bool angled{};
};
struct PragmaT {
  std::string source{};
  uint32_t line{};
  uint32_t column{};
};
struct TranslationUnitFileT {
  int32_t id{};
  std::string path{};
};
struct CounterT {
  int32_t value{};
};
struct LanguageT {
  std::string file{};
  bool line_comment{};
  bool gnu_inline{};
  bool c99{};
  bool c11{};
  bool c_plus_plus{};
  bool c_plus_plus_11{};
  bool c_plus_plus_14{};
  bool c_plus_plus_17{};
  bool c_plus_plus_20{};
  bool c_plus_plus_23{};
  bool c_plus_plus_26{};
  bool has_digraphs{};
  bool is_gnu{};
  bool hex_floats{};
  bool open_cl{};
  uint32_t open_cl_version{};
  bool native_half_type{};
  bool cuda{};
  bool has_bool{};
  bool has_half{};
  bool has_wchar{};
  uint32_t char_width{};
  uint32_t float_width{};
  uint32_t double_width{};
  uint32_t long_double_width{};
  uint32_t bool_width{};
  uint32_t short_width{};
  uint32_t int_width{};
  uint32_t long_width{};
  uint32_t long_long_width{};
};
struct NodeT {
  int64_t id{};
  std::string class_name{};
  UnionValue payload;
};

using TemplateValue = UnionValue;
using TemplateNameValue = UnionValue;
using InitializerTarget = UnionValue;
using ExceptionDetails = UnionValue;
using OffsetValue = UnionValue;
using DesignatorValue = UnionValue;
using NestedNameValue = UnionValue;
using DeclPayload = UnionValue;
using TypePayload = UnionValue;
using NodePayload = UnionValue;

} // namespace astwire::v1obj
