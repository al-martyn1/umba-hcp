/*! \file
    \brief
 */

#include "umba/umba.h"
//
#include "umba/tokenizer.h"
#include "umba/assert.h"
#include "umba/filename.h"
#include "umba/filesys.h"
#include "umba/tokenizer/token_filters.h"
#include "umba/tokenizer/lang/cpp.h"
#include "umba/string_plus.h"
//
#include "umba/debug_helpers.h"

//
#include "umba/text_position_info.h"
#include "umba/iterator.h"
#include "umba/the.h"
#include "marty_cpp/src_normalization.h"
//

#include <iostream>
#include <map>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <deque>
#include <sstream>
#include <list>



#define USE_TRY_CATCH


using std::cout;
using std::cerr;


enum IncludeMode
{
    userIncludes = 1,
    systemIncludes = 2,
    allIncludes = 3
};




struct TokenInfo
{
    umba::tokenizer::payload_type                        tokenType;
    umba::iterator::TextPositionCountingIterator<char>   b;
    umba::iterator::TextPositionCountingIterator<char>   e;
};


int printHelp(int res)
{
    std::cout << "Usage: umba-hcp [OPTIONS] FILE [FILE...]\n"
                 "where OPTIONS are:\n"
                 "  -IPATH[,PATH...]  - add include search paths.\n"
                 "  -oPATH            - set destination path.\n"
                 "  -A, --all           - process all includes.\n"
                 "  -S, --system        - process system includes only.\n"
                 "  -U, --user          - process user includes only.\n"
                 ;

    return res;
}

std::string findInclude(const std::string &incPath, const std::string &file)
{
    std::string fullName = umba::filename::makeAbsPath(file, incPath);
    if (umba::filesys::isFileExist(fullName))
        return fullName;
    return std::string();
}

std::string findInclude(const std::vector<std::string> &incPaths, const std::string &file)
{
    for(const auto &p : incPaths)
    {
        auto fullName = findInclude(p, file);
        if (fullName.empty())
            return fullName;
    }

    return std::string();
}

bool findRelName(std::string &relNameFound, const std::vector<std::string> &incPaths, const std::string &file )
{
    for(const auto &p : incPaths)
    {
       if (umba::filename::makeRelPath(relNameFound, p, file))
       {
           return true;
       }
    }

    return false;
}



int main(int argc, char* argv[])
{

    std::vector<std::string> argsVec;
    for(int argIdx=1; argIdx<argc; ++argIdx)
    {
        argsVec.emplace_back(argv[argIdx]);
    }

    if (umba::isDebuggerPresent() /*  || inputFiles.empty() */ )
    {
        argsVec.clear();

        std::string cwd = umba::filesys::getCurrentDirectory<std::string>();
        std::cout << "Working Dir: " << cwd << "\n";
        std::string rootPath;

        #if (defined(WIN32) || defined(_WIN32))


            if (winhelpers::isProcessHasParentOneOf({"devenv"}))
            {
                // По умолчанию студия задаёт текущим каталогом На  уровень выше от того, где лежит бинарник
                rootPath = umba::filename::makeCanonical(umba::filename::appendPath<std::string>(cwd, "..\\..\\..\\"));
            }
            else if (winhelpers::isProcessHasParentOneOf({"code"}))
            {
                // По умолчанию VSCode задаёт текущим каталогом тот, где лежит бинарник
                rootPath = umba::filename::makeCanonical(umba::filename::appendPath<std::string>(cwd, "..\\..\\..\\..\\"));
            }
            else
            {
            }

            if (!rootPath.empty())
                rootPath = umba::filename::appendPathSepCopy(rootPath);

        #endif

        argsVec.clear();
        argsVec.emplace_back("-Id:/umbas;" + rootPath + "_libs");
        argsVec.emplace_back(rootPath + "_libs/umba/tokenizer.h");
        argsVec.emplace_back("-o" + rootPath + "tests/hcp");

    }


    using namespace umba::tokenizer;

    std::list<std::string>   inputFiles;
    std::string              inputFilename;

    std::vector<std::string> incPaths;
    std::string              dstPath;
    IncludeMode              includeMode = userIncludes;

    for(auto arg : argsVec)
    {

        if (arg.empty())
            continue;

        if (arg[0]!='-')
        {
            inputFiles.emplace_back(umba::filename::makeAbsPath(arg));
            continue;
        }

        // Разбираем опции
        if (umba::string_plus::starts_with_and_strip(arg, "-o"))
        {
            if (arg.empty())
            {
                std::cerr << "Invalid option '-o' value: '" << arg << "'\n" << std::flush;
                return printHelp(1);
            }

            dstPath = umba::filename::makeAbsPath(arg);
            continue;
        }
        else if (umba::string_plus::starts_with_and_strip(arg, "-I"))
        {
            if (arg.empty())
            {
                std::cerr << "Invalid option '-I' value: '" << arg << "'\n" << std::flush;
                return printHelp(1);
            }

            std::vector<std::string> paths = umba::filename::splitPathList(arg);
            for(const auto &p: paths)
                incPaths.emplace_back(umba::filename::makeAbsPath(p));
        }
        else if (arg=="-A" || arg=="--all")
        {
            includeMode = allIncludes;
        }
        else if (arg=="-S" || arg=="--system")
        {
            includeMode = systemIncludes;
        }
        else if (arg=="-U" || arg=="--user")
        {
            includeMode = userIncludes;
        }
    }


    if (incPaths.empty())
    {
        std::cerr << "No include paths taken (-I option)\n" << std::flush;
        return printHelp(1);
    }

    if (inputFiles.empty())
    {
        std::cerr << "No input files taken\n" << std::flush;
        return printHelp(1);
    }

    if (dstPath.empty())
    {
        std::cerr << "No destination path taken\n" << std::flush;
        return printHelp(1);
    }


    bool bOk = true;

    auto tokenizerBuilder = umba::tokenizer::makeTokenizerBuilderCpp<char>();
    using TokenizerBuilderType = decltype(tokenizerBuilder);
    using tokenizer_type       = typename TokenizerBuilderType::tokenizer_type;


    using InputIteratorType    = typename tokenizer_type::iterator_type;
    using tokenizer_char_type  = typename tokenizer_type::value_type;
    using string_type          = typename tokenizer_type::string_type;
    using messages_string_type = typename tokenizer_type::messages_string_type;
    using token_parsed_data    = typename tokenizer_type::token_parsed_data;


    bool inPreprocessor = false;
    bool inInclude      = false;


    auto tokenHandler =      [&]( auto &tokenizer
                                , bool bLineStart, payload_type tokenType
                                , InputIteratorType b, InputIteratorType e
                                , token_parsed_data parsedData // std::basic_string_view<tokenizer_char_type> parsedData
                                , messages_string_type &errMsg
                                ) -> bool
                             {
                                 UMBA_USED(parsedData);

                                 if (tokenType==UMBA_TOKENIZER_TOKEN_CTRL_FIN)
                                 {
                                     return true;
                                 }
                                 else if (tokenType==UMBA_TOKENIZER_TOKEN_CTRL_CC_PP_START)
                                 {
                                     inPreprocessor = true;
                                     return true;
                                 }
                                 else if (tokenType==UMBA_TOKENIZER_TOKEN_CTRL_CC_PP_END)
                                 {
                                     inPreprocessor = false;
                                     inInclude      = false;
                                     return true;
                                 }
                                 else if (tokenType==UMBA_TOKENIZER_TOKEN_CTRL_CC_PP_DEFINE)
                                 {
                                     return true;
                                 }
                                 else if (tokenType==UMBA_TOKENIZER_TOKEN_CTRL_CC_PP_INCLUDE)
                                 {
                                     // !!!
                                     inInclude = true;
                                     return true;
                                 }

                                 if (tokenType&UMBA_TOKENIZER_TOKEN_CTRL_FLAG)
                                 {
                                     return true; // Управляющий токен, не надо выводить, никакой нагрузки при нём нет
                                 }


                                 if (inInclude && (tokenType==UMBA_TOKENIZER_TOKEN_STRING_LITERAL || tokenType==UMBA_TOKENIZER_TOKEN_ANGLE_BACKETS_STRING_LITERAL))
                                 {
                                     auto stringLiteralData = std::get<typename tokenizer_type::StringLiteralData>(parsedData);
                                     auto includeFileName = string_type(stringLiteralData.data);
                                 
                                     if (tokenType==UMBA_TOKENIZER_TOKEN_STRING_LITERAL && (includeMode==userIncludes || includeMode==allIncludes))
                                     {
                                         auto path = umba::filename::getPath(inputFilename);
                                         // Файл может быть задан относительно текущего файла

                                         auto foundFile = findInclude(path, includeFileName);
                                         if (!foundFile.empty())
                                         {
                                             inputFiles.emplace_back(foundFile);
                                         }
                                         else
                                         {
                                             foundFile = findInclude(incPaths, includeFileName);
                                             if (!foundFile.empty())
                                             {
                                                 inputFiles.emplace_back(foundFile);
                                             }
                                         }
                                     }
                                     else if (tokenType==UMBA_TOKENIZER_TOKEN_ANGLE_BACKETS_STRING_LITERAL && (includeMode==systemIncludes || includeMode==allIncludes))
                                     {
                                         auto foundFile = findInclude(incPaths, includeFileName);
                                         if (!foundFile.empty())
                                         {
                                             inputFiles.emplace_back(foundFile);
                                         }
                                     }
                                 }

                                 return true;
                             };

    auto tokenizer = umba::tokenizer::makeTokenizerCpp( tokenizerBuilder
                                                      , tokenHandler
                                                      );


    #if defined(WIN32) || defined(_WIN32)
        marty_cpp::ELinefeedType outputLinefeed = marty_cpp::ELinefeedType::crlf;
    #else
        marty_cpp::ELinefeedType outputLinefeed = marty_cpp::ELinefeedType::lf;
    #endif

    std::cout << "Umba Header Copy Tool v1.0\n\n";


    std::set<std::string>   filesToCopy;
    std::set<std::string>   processedFiles;

    while(!inputFiles.empty())
    {
        std::string fn = inputFiles.front();
        inputFiles.pop_front();

        if (processedFiles.find(fn)!=processedFiles.end())
            continue;

        processedFiles.insert(fn);

        inputFilename = fn;
        std::string text;
        std::cout << "Processing: '" << inputFilename << "' - ";

        if (!umba::filesys::readFile(inputFilename, text))
        {
            std::cout << "Failed to read\n";
            continue;
        }

        std::cout << "Found\n";

        filesToCopy.emplace(inputFilename);

        bOk = true;

#if defined(USE_TRY_CATCH)
        try
        {
#endif
            auto itBegin = InputIteratorType(text.data(), text.size());
            auto itEnd   = InputIteratorType();
            tokenizer.tokenizeInit();
            InputIteratorType it = itBegin;
            for(; it!=itEnd && bOk; ++it)
            {
                if (!tokenizer.tokenize(it, itEnd))
                {
                    bOk = false;
                }
            }

            if (bOk)
            {
                bOk = tokenizer.tokenizeFinalize(itEnd);
            }

#if defined(USE_TRY_CATCH)
        }
        catch(const std::exception &e)
        {

        }
#endif

    } // while(!inputFiles.empty())


    std::cout << "\nCopying files:\n";
    for(auto f : filesToCopy)
    {
        std::string relName;

        //bool isRel = umba::filename::makeRelPath( relName, srcPath, f );
        bool isRel = findRelName(relName, incPaths, f);

        std::cout << (isRel?"+ ":"- ") << f << " ";

        if (isRel)
        {
            auto copyTo = umba::filename::makeCanonical(umba::filename::appendPath(dstPath, relName));

            std::cout << "-> " << copyTo << "\n";

            umba::filesys::createDirectoryEx( umba::filename::getPath(copyTo), true /* forceCreatePath */ );

            if (!CopyFileA( f.c_str(), copyTo.c_str(), FALSE))
            {
                std::cout << ": ! Failed to copy file\n";
            }
        }
        else
        {
            std::cout << ": ! File is not from the source directory or it's subdirs\n";
        }
    }

    return 0;

}

