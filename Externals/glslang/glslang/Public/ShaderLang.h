//
//Copyright (C) 2002-2005  3Dlabs Inc. Ltd.
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without
//modification, are permitted provided that the following conditions
//are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above
//    copyright notice, this list of conditions and the following
//    disclaimer in the documentation and/or other materials provided
//    with the distribution.
//
//    Neither the name of 3Dlabs Inc. Ltd. nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
//COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
//LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
//ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//POSSIBILITY OF SUCH DAMAGE.
//
#ifndef _COMPILER_INTERFACE_INCLUDED_
#define _COMPILER_INTERFACE_INCLUDED_

#include "../Include/ResourceLimits.h"
#include "../MachineIndependent/Versions.h"

#include <string.h>

#ifdef _WIN32
#define C_DECL __cdecl
//#ifdef SH_EXPORTING
//    #define SH_IMPORT_EXPORT __declspec(dllexport)
//#else
//    #define SH_IMPORT_EXPORT __declspec(dllimport)
//#endif
#define SH_IMPORT_EXPORT
#else
#define SH_IMPORT_EXPORT
#ifndef __fastcall
#define __fastcall
#endif
#define C_DECL
#endif

//
// This is the platform independent interface between an OGL driver
// and the shading language compiler/linker.
//

#ifdef __cplusplus
    extern "C" {
#endif

//
// Driver must call this first, once, before doing any other
// compiler/linker operations.
//
// (Call once per process, not once per thread.)
//
SH_IMPORT_EXPORT int ShInitialize();

//
// Driver should call this at process shutdown.
//
#ifdef __FreeBSD__
SH_IMPORT_EXPORT int ShFinalize();
#else
SH_IMPORT_EXPORT int __fastcall ShFinalize();
#endif

//
// Types of languages the compiler can consume.
//
typedef enum {
    EShLangVertex,
    EShLangTessControl,
    EShLangTessEvaluation,
    EShLangGeometry,
    EShLangFragment,
    EShLangCompute,
    EShLangCount,
} EShLanguage;         // would be better as stage, but this is ancient now

typedef enum {
    EShLangVertexMask         = (1 << EShLangVertex),
    EShLangTessControlMask    = (1 << EShLangTessControl),
    EShLangTessEvaluationMask = (1 << EShLangTessEvaluation),
    EShLangGeometryMask       = (1 << EShLangGeometry),
    EShLangFragmentMask       = (1 << EShLangFragment),
    EShLangComputeMask        = (1 << EShLangCompute),
} EShLanguageMask;

namespace glslang {

typedef enum {
    EShSourceNone,
    EShSourceGlsl,
    EShSourceHlsl,
} EShSource;          // if EShLanguage were EShStage, this could be EShLanguage instead

const char* StageName(EShLanguage);

} // end namespace glslang

//
// Types of output the linker will create.
//
typedef enum {
    EShExVertexFragment,
    EShExFragment
} EShExecutable;

//
// Optimization level for the compiler.
//
typedef enum {
    EShOptNoGeneration,
    EShOptNone,
    EShOptSimple,       // Optimizations that can be done quickly
    EShOptFull,         // Optimizations that will take more time
} EShOptimizationLevel;

//
// Message choices for what errors and warnings are given.
//
enum EShMessages {
    EShMsgDefault          = 0,         // default is to give all required errors and extra warnings
    EShMsgRelaxedErrors    = (1 << 0),  // be liberal in accepting input
    EShMsgSuppressWarnings = (1 << 1),  // suppress all warnings, except those required by the specification
    EShMsgAST              = (1 << 2),  // print the AST intermediate representation
    EShMsgSpvRules         = (1 << 3),  // issue messages for SPIR-V generation
    EShMsgVulkanRules      = (1 << 4),  // issue messages for Vulkan-requirements of GLSL for SPIR-V
    EShMsgOnlyPreprocessor = (1 << 5),  // only print out errors produced by the preprocessor
    EShMsgReadHlsl         = (1 << 6),  // use HLSL parsing rules and semantics
    EShMsgCascadingErrors  = (1 << 7),  // get cascading errors; risks error-recovery issues, instead of an early exit
};

//
// Build a table for bindings.  This can be used for locating
// attributes, uniforms, globals, etc., as needed.
//
typedef struct {
    const char* name;
    int binding;
} ShBinding;

typedef struct {
    int numBindings;
    ShBinding* bindings;  // array of bindings
} ShBindingTable;

//
// ShHandle held by but opaque to the driver.  It is allocated,
// managed, and de-allocated by the compiler/linker. It's contents 
// are defined by and used by the compiler and linker.  For example,
// symbol table information and object code passed from the compiler 
// to the linker can be stored where ShHandle points.
//
// If handle creation fails, 0 will be returned.
//
typedef void* ShHandle;

//
// Driver calls these to create and destroy compiler/linker
// objects.
//
SH_IMPORT_EXPORT ShHandle ShConstructCompiler(const EShLanguage, int debugOptions);  // one per shader
SH_IMPORT_EXPORT ShHandle ShConstructLinker(const EShExecutable, int debugOptions);  // one per shader pair
SH_IMPORT_EXPORT ShHandle ShConstructUniformMap();                 // one per uniform namespace (currently entire program object)
SH_IMPORT_EXPORT void ShDestruct(ShHandle);

//
// The return value of ShCompile is boolean, non-zero indicating
// success.
//
// The info-log should be written by ShCompile into 
// ShHandle, so it can answer future queries.
//
SH_IMPORT_EXPORT int ShCompile(
    const ShHandle,
    const char* const shaderStrings[],
    const int numStrings,
    const int* lengths,
    const EShOptimizationLevel,
    const TBuiltInResource *resources,
    int debugOptions,
    int defaultVersion = 110,            // use 100 for ES environment, overridden by #version in shader
    bool forwardCompatible = false,      // give errors for use of deprecated features
    EShMessages messages = EShMsgDefault // warnings and errors
    );

SH_IMPORT_EXPORT int ShLink(
    const ShHandle,               // linker object
    const ShHandle h[],           // compiler objects to link together
    const int numHandles,
    ShHandle uniformMap,          // updated with new uniforms
    short int** uniformsAccessed,  // returned with indexes of uniforms accessed
    int* numUniformsAccessed);

SH_IMPORT_EXPORT int ShLinkExt(
    const ShHandle,               // linker object
    const ShHandle h[],           // compiler objects to link together
    const int numHandles);

//
// ShSetEncrpytionMethod is a place-holder for specifying
// how source code is encrypted.
//
SH_IMPORT_EXPORT void ShSetEncryptionMethod(ShHandle);

//
// All the following return 0 if the information is not
// available in the object passed down, or the object is bad.
//
SH_IMPORT_EXPORT const char* ShGetInfoLog(const ShHandle);
SH_IMPORT_EXPORT const void* ShGetExecutable(const ShHandle);
SH_IMPORT_EXPORT int ShSetVirtualAttributeBindings(const ShHandle, const ShBindingTable*);   // to detect user aliasing
SH_IMPORT_EXPORT int ShSetFixedAttributeBindings(const ShHandle, const ShBindingTable*);     // to force any physical mappings
//
// Tell the linker to never assign a vertex attribute to this list of physical attributes
//
SH_IMPORT_EXPORT int ShExcludeAttributes(const ShHandle, int *attributes, int count);

//
// Returns the location ID of the named uniform.
// Returns -1 if error.
//
SH_IMPORT_EXPORT int ShGetUniformLocation(const ShHandle uniformMap, const char* name);

#ifdef __cplusplus
    }  // end extern "C"
#endif

////////////////////////////////////////////////////////////////////////////////////////////
//
// Deferred-Lowering C++ Interface
// -----------------------------------
//
// Below is a new alternate C++ interface that might potentially replace the above
// opaque handle-based interface.  
//    
// The below is further designed to handle multiple compilation units per stage, where
// the intermediate results, including the parse tree, are preserved until link time,
// rather than the above interface which is designed to have each compilation unit
// lowered at compile time.  In the above model, linking occurs on the lowered results,
// whereas in this model intra-stage linking can occur at the parse tree
// (treeRoot in TIntermediate) level, and then a full stage can be lowered.
//

#include <list>
#include <string>
#include <utility>

class TCompiler;
class TInfoSink;

namespace glslang {

const char* GetEsslVersionString();
const char* GetGlslVersionString();
int GetKhronosToolId();

class TIntermediate;
class TProgram;
class TPoolAllocator;

// Call this exactly once per process before using anything else
bool InitializeProcess();

// Call once per process to tear down everything
void FinalizeProcess();

// Make one TShader per shader that you will link into a program.  Then provide
// the shader through setStrings() or setStringsWithLengths(), then call parse(),
// then query the info logs.
// Optionally use setPreamble() to set a special shader string that will be
// processed before all others but won't affect the validity of #version.
//
// N.B.: Does not yet support having the same TShader instance being linked into
// multiple programs.
//
// N.B.: Destruct a linked program *before* destructing the shaders linked into it.
//
class TShader {
public:
    explicit TShader(EShLanguage);
    virtual ~TShader();
    void setStrings(const char* const* s, int n);
    void setStringsWithLengths(const char* const* s, const int* l, int n);
    void setStringsWithLengthsAndNames(
        const char* const* s, const int* l, const char* const* names, int n);
    void setPreamble(const char* s) { preamble = s; }
    void setEntryPoint(const char* entryPoint);

    // Interface to #include handlers.
    //
    // To support #include, a client of Glslang does the following:
    // 1. Call setStringsWithNames to set the source strings and associated
    //    names.  For example, the names could be the names of the files
    //    containing the shader sources.
    // 2. Call parse with an Includer.
    //
    // When the Glslang parser encounters an #include directive, it calls
    // the Includer's include method with the requested include name
    // together with the current string name.  The returned IncludeResult
    // contains the fully resolved name of the included source, together
    // with the source text that should replace the #include directive
    // in the source stream.  After parsing that source, Glslang will
    // release the IncludeResult object.
    class Includer {
    public:
        typedef enum {
          EIncludeRelative, // For #include "something"
          EIncludeStandard  // Reserved. For #include <something>
        } IncludeType;

        // An IncludeResult contains the resolved name and content of a source
        // inclusion.
        struct IncludeResult {
            IncludeResult(const std::string& file_name, const char* const file_data, const size_t file_length, void* user_data) :
                file_name(file_name), file_data(file_data), file_length(file_length), user_data(user_data) { }
            // For a successful inclusion, the fully resolved name of the requested
            // include.  For example, in a file system-based includer, full resolution
            // should convert a relative path name into an absolute path name.
            // For a failed inclusion, this is an empty string.
            const std::string file_name;
            // The content and byte length of the requested inclusion.  The
            // Includer producing this IncludeResult retains ownership of the
            // storage.
            // For a failed inclusion, the file_data
            // field points to a string containing error details.
            const char* const file_data;
            const size_t file_length;
            // Include resolver's context.
            void* user_data;
        protected:
            IncludeResult& operator=(const IncludeResult&);
            IncludeResult();
        };

        // Resolves an inclusion request by name, type, current source name,
        // and include depth.
        // On success, returns an IncludeResult containing the resolved name
        // and content of the include.  On failure, returns an IncludeResult
        // with an empty string for the file_name and error details in the
        // file_data field.  The Includer retains ownership of the contents
        // of the returned IncludeResult value, and those contents must
        // remain valid until the releaseInclude method is called on that
        // IncludeResult object.
        virtual IncludeResult* include(const char* requested_source,
                                      IncludeType type,
                                      const char* requesting_source,
                                      size_t inclusion_depth) = 0;
        // Signals that the parser will no longer use the contents of the
        // specified IncludeResult.
        virtual void releaseInclude(IncludeResult* result) = 0;
#ifdef __ANDROID__
        virtual ~Includer() {} // Pacify -Werror=non-virtual-dtor
#endif
    };

    // Returns an error message for any #include directive.
    class ForbidInclude : public Includer {
    public:
        IncludeResult* include(const char*, IncludeType, const char*, size_t) override
        {
            const char* unexpected_include = "unexpected include directive";
            return new IncludeResult(std::string(""), unexpected_include, strlen(unexpected_include), nullptr);
        }
        virtual void releaseInclude(IncludeResult* result) override
        {
            delete result;
        }
    };

    bool parse(const TBuiltInResource* res, int defaultVersion, EProfile defaultProfile, bool forceDefaultVersionAndProfile,
               bool forwardCompatible, EShMessages messages)
    {
        TShader::ForbidInclude includer;
        return parse(res, defaultVersion, defaultProfile, forceDefaultVersionAndProfile, forwardCompatible, messages, includer);
    }

    bool parse(const TBuiltInResource*, int defaultVersion, EProfile defaultProfile, bool forceDefaultVersionAndProfile,
               bool forwardCompatible, EShMessages, Includer&);

    // Equivalent to parse() without a default profile and without forcing defaults.
    // Provided for backwards compatibility.
    bool parse(const TBuiltInResource*, int defaultVersion, bool forwardCompatible, EShMessages);
    bool preprocess(const TBuiltInResource* builtInResources,
                    int defaultVersion, EProfile defaultProfile, bool forceDefaultVersionAndProfile,
                    bool forwardCompatible, EShMessages message, std::string* outputString,
                    Includer& includer);

    const char* getInfoLog();
    const char* getInfoDebugLog();

    EShLanguage getStage() const { return stage; }

protected:
    TPoolAllocator* pool;
    EShLanguage stage;
    TCompiler* compiler;
    TIntermediate* intermediate;
    TInfoSink* infoSink;
    // strings and lengths follow the standard for glShaderSource:
    //     strings is an array of numStrings pointers to string data.
    //     lengths can be null, but if not it is an array of numStrings
    //         integers containing the length of the associated strings.
    //         if lengths is null or lengths[n] < 0  the associated strings[n] is
    //         assumed to be null-terminated.
    // stringNames is the optional names for all the strings. If stringNames
    // is null, then none of the strings has name. If a certain element in
    // stringNames is null, then the corresponding string does not have name.
    const char* const* strings;
    const int* lengths;
    const char* const* stringNames;
    const char* preamble;
    int numStrings;

    friend class TProgram;

private:
    TShader& operator=(TShader&);
};

class TReflection;

// Make one TProgram per set of shaders that will get linked together.  Add all 
// the shaders that are to be linked together.  After calling shader.parse()
// for all shaders, call link().
//
// N.B.: Destruct a linked program *before* destructing the shaders linked into it.
//
class TProgram {
public:
    TProgram();
    virtual ~TProgram();
    void addShader(TShader* shader) { stages[shader->stage].push_back(shader); }

    // Link Validation interface
    bool link(EShMessages);
    const char* getInfoLog();
    const char* getInfoDebugLog();

    TIntermediate* getIntermediate(EShLanguage stage) const { return intermediate[stage]; }

    // Reflection Interface
    bool buildReflection();                          // call first, to do liveness analysis, index mapping, etc.; returns false on failure
    int getNumLiveUniformVariables();                // can be used for glGetProgramiv(GL_ACTIVE_UNIFORMS)
    int getNumLiveUniformBlocks();                   // can be used for glGetProgramiv(GL_ACTIVE_UNIFORM_BLOCKS)
    const char* getUniformName(int index);           // can be used for "name" part of glGetActiveUniform()
    const char* getUniformBlockName(int blockIndex); // can be used for glGetActiveUniformBlockName()
    int getUniformBlockSize(int blockIndex);         // can be used for glGetActiveUniformBlockiv(UNIFORM_BLOCK_DATA_SIZE)
    int getUniformIndex(const char* name);           // can be used for glGetUniformIndices()
    int getUniformBlockIndex(int index);             // can be used for glGetActiveUniformsiv(GL_UNIFORM_BLOCK_INDEX)
    int getUniformType(int index);                   // can be used for glGetActiveUniformsiv(GL_UNIFORM_TYPE)
    int getUniformBufferOffset(int index);           // can be used for glGetActiveUniformsiv(GL_UNIFORM_OFFSET)
    int getUniformArraySize(int index);              // can be used for glGetActiveUniformsiv(GL_UNIFORM_SIZE)
    int getNumLiveAttributes();                      // can be used for glGetProgramiv(GL_ACTIVE_ATTRIBUTES)
    const char *getAttributeName(int index);         // can be used for glGetActiveAttrib()
    int getAttributeType(int index);                 // can be used for glGetActiveAttrib()
    void dumpReflection();

protected:
    bool linkStage(EShLanguage, EShMessages);

    TPoolAllocator* pool;
    std::list<TShader*> stages[EShLangCount];
    TIntermediate* intermediate[EShLangCount];
    bool newedIntermediate[EShLangCount];      // track which intermediate were "new" versus reusing a singleton unit in a stage
    TInfoSink* infoSink;
    TReflection* reflection;
    bool linked;

private:
    TProgram& operator=(TProgram&);
};

} // end namespace glslang

#endif // _COMPILER_INTERFACE_INCLUDED_
