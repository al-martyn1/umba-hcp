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
//
#include "umba/debug_helpers.h"

//
#include "umba/text_position_info.h"
#include "umba/iterator.h"
#include "umba/the.h"
//
// #include "utils.h"
//
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



#define USE_SIMPLE_NUMBER_SUFFIX_GLUING_FILTER
// #define NUMBER_PRINTING_PRINT_PARSED_VALUE
// #define PRINT_ONLY_NUMBERS
#define USE_TRY_CATCH

//#define DUPLICATE_TO_STD_OUT



using std::cout;
using std::cerr;


enum IncludeMode
{
    userIncludes    = 1,
    systemIncludes  = 2,
    allIncludes     = 3
}

IncludeMode  includeMode = userIncludes;


struct TokenInfo
{
    umba::tokenizer::payload_type                        tokenType;
    umba::iterator::TextPositionCountingIterator<char>   b;
    umba::iterator::TextPositionCountingIterator<char>   e;
};





int main(int argc, char* argv[])
{
    // auto t1 = getCharClassTable();
    // auto t2 = getTrieVector();
    // auto t3 = getString();
    // auto t4 = getIterator();

    using namespace umba::tokenizer;

    //std::vector<std::string>  inputFiles;

    std::string srcPath;
    std::string srcFile;
    std::string dstPath;

    std::string inputFilename;
    std::list<std::string> inputFiles;


    if (argc>3)
    {
        srcPath = argv[1];
        srcFile = argv[2];
        dstPath = argv[3];
    }

    srcPath = umba::filename::makeAbsPath(srcPath);
    srcFile = umba::filename::makeAbsPath(srcFile);
    dstPath = umba::filename::makeAbsPath(dstPath);


    if (umba::isDebuggerPresent() /*  || inputFiles.empty() */ )
    {

        srcPath.clear();
        srcFile.clear();
        dstPath.clear();

        std::string cwd = umba::filesys::getCurrentDirectory<std::string>();
        std::cout << "Working Dir: " << cwd << "\n";
        std::string rootPath;

        #if (defined(WIN32) || defined(_WIN32))


            if (winhelpers::isProcessHasParentOneOf({"devenv"}))
            {
                // По умолчанию студия задаёт текущим каталогом На  уровень выше от того, где лежит бинарник
                rootPath = umba::filename::makeCanonical(umba::filename::appendPath<std::string>(cwd, "..\\..\\..\\"));
                //argsParser.args.push_back("--batch-output-root=D:/temp/mdpp-test");
            }
            else if (winhelpers::isProcessHasParentOneOf({"code"}))
            {
                // По умолчанию VSCode задаёт текущим каталогом тот, где лежит бинарник
                rootPath = umba::filename::makeCanonical(umba::filename::appendPath<std::string>(cwd, "..\\..\\..\\..\\"));
                //argsParser.args.push_back("--batch-output-root=C:/work/temp/mdpp-test");

            }
            else
            {
                //rootPath = umba::filename::makeCanonical(umba::filename::appendPath<std::string>(cwd, "..\\..\\..\\"));
            }

            //#endif

            if (!rootPath.empty())
                rootPath = umba::filename::appendPathSepCopy(rootPath);

        #endif

        srcPath = rootPath + "_libs";
        srcFile = "umba/tokenizer.h";
        dstPath = rootPath + "tests/hcp";

    }

    if (dstPath.empty())
    {
        std::cout << "Usage: umba-hcp INC_PATH FILE_TO_PROCESS DST_PATH\n";
        return 1;
    }


    payload_type numberTokenId = UMBA_TOKENIZER_TOKEN_NUMBER_USER_LITERAL_FIRST;

    umba::tokenizer::CppEscapedSimpleQuotedStringLiteralParser<char>  cppEscapedSimpleQuotedStringLiteralParser;
    umba::tokenizer::SimpleQuotedStringLiteralParser<char>            simpleQuotedStringLiteralParser;


    auto tokenizer = TokenizerBuilder<char>().generateStandardCharClassTable()

                                             .addNumbersPrefix("0b", numberTokenId++ | UMBA_TOKENIZER_TOKEN_NUMBER_LITERAL_BASE_BIN)
                                             .addNumbersPrefix("0B", numberTokenId++ | UMBA_TOKENIZER_TOKEN_NUMBER_LITERAL_BASE_BIN)

                                             .addNumbersPrefix("0d", numberTokenId++ | UMBA_TOKENIZER_TOKEN_NUMBER_LITERAL_BASE_DEC)
                                             .addNumbersPrefix("0D", numberTokenId++ | UMBA_TOKENIZER_TOKEN_NUMBER_LITERAL_BASE_DEC)

                                             .addNumbersPrefix("0" , numberTokenId++ | UMBA_TOKENIZER_TOKEN_NUMBER_LITERAL_BASE_OCT | UMBA_TOKENIZER_TOKEN_NUMBER_LITERAL_FLAG_MISS_DIGIT)

                                             .addNumbersPrefix("0x", numberTokenId++ | UMBA_TOKENIZER_TOKEN_NUMBER_LITERAL_BASE_HEX)
                                             .addNumbersPrefix("0X", numberTokenId++ | UMBA_TOKENIZER_TOKEN_NUMBER_LITERAL_BASE_HEX)


                                             .addBrackets("{}", UMBA_TOKENIZER_TOKEN_CURLY_BRACKETS )
                                             .addBrackets("()", UMBA_TOKENIZER_TOKEN_ROUND_BRACKETS )
                                             .addBrackets("<>", UMBA_TOKENIZER_TOKEN_ANGLE_BRACKETS )
                                             .addBrackets("[]", UMBA_TOKENIZER_TOKEN_SQUARE_BRACKETS)


                                             .addSingleLineComment("//", UMBA_TOKENIZER_TOKEN_OPERATOR_SINGLE_LINE_COMMENT_FIRST)
                                             .setMultiLineComment("/*", "*/")

                                             // Операторы # и ## доступны только внутри директивы define препроцессора.
                                             // Для этого вначале работы мы сбрасываем признак umba::tokenizer::CharClass::opchar,
                                             // при получении маркера директивы define - устанавливаем его,
                                             // и при окончании блока препроцессора опять сбрасываем
                                             .addOperator("#"  , UMBA_TOKENIZER_TOKEN_OPERATOR_CC_PP_CONCAT                  )
                                             .addOperator("##" , UMBA_TOKENIZER_TOKEN_OPERATOR_CC_PP_STRINGIFY               )

                                             .addOperator("."  , UMBA_TOKENIZER_TOKEN_OPERATOR_DOT                           )
                                             .addOperator("...", UMBA_TOKENIZER_TOKEN_OPERATOR_VA_ARGS                       )
                                             .addOperator("+"  , UMBA_TOKENIZER_TOKEN_OPERATOR_ADDITION                      )
                                             .addOperator("-"  , UMBA_TOKENIZER_TOKEN_OPERATOR_SUBTRACTION                   )
                                             .addOperator("*"  , UMBA_TOKENIZER_TOKEN_OPERATOR_MULTIPLICATION                )
                                             .addOperator("/"  , UMBA_TOKENIZER_TOKEN_OPERATOR_DIVISION                      )
                                             .addOperator("%"  , UMBA_TOKENIZER_TOKEN_OPERATOR_MODULO                        )
                                             .addOperator("++" , UMBA_TOKENIZER_TOKEN_OPERATOR_INCREMENT                     )
                                             .addOperator("--" , UMBA_TOKENIZER_TOKEN_OPERATOR_DECREMENT                     )
                                             .addOperator("==" , UMBA_TOKENIZER_TOKEN_OPERATOR_EQUAL                         )
                                             .addOperator("!=" , UMBA_TOKENIZER_TOKEN_OPERATOR_NOT_EQUAL                     )
                                             .addOperator(">"  , UMBA_TOKENIZER_TOKEN_OPERATOR_GREATER                       )
                                             .addOperator("<"  , UMBA_TOKENIZER_TOKEN_OPERATOR_LESS                          )
                                             .addOperator(">=" , UMBA_TOKENIZER_TOKEN_OPERATOR_GREATER_EQUAL                 )
                                             .addOperator("<=" , UMBA_TOKENIZER_TOKEN_OPERATOR_LESS_EQUAL                    )
                                             .addOperator("<=>", UMBA_TOKENIZER_TOKEN_OPERATOR_THREE_WAY_COMPARISON          )
                                             .addOperator("!"  , UMBA_TOKENIZER_TOKEN_OPERATOR_LOGICAL_NOT                   )
                                             .addOperator("&&" , UMBA_TOKENIZER_TOKEN_OPERATOR_LOGICAL_AND                   )
                                             .addOperator("||" , UMBA_TOKENIZER_TOKEN_OPERATOR_LOGICAL_OR                    )
                                             .addOperator("~"  , UMBA_TOKENIZER_TOKEN_OPERATOR_BITWISE_NOT                   )
                                             .addOperator("&"  , UMBA_TOKENIZER_TOKEN_OPERATOR_BITWISE_AND                   )
                                             .addOperator("|"  , UMBA_TOKENIZER_TOKEN_OPERATOR_BITWISE_OR                    )
                                             .addOperator("^"  , UMBA_TOKENIZER_TOKEN_OPERATOR_BITWISE_XOR                   )
                                             .addOperator("<<" , UMBA_TOKENIZER_TOKEN_OPERATOR_BITWISE_SHIFT_LEFT            )
                                             .addOperator(">>" , UMBA_TOKENIZER_TOKEN_OPERATOR_BITWISE_SHIFT_RIGHT           )
                                             .addOperator("="  , UMBA_TOKENIZER_TOKEN_OPERATOR_ASSIGNMENT                    )
                                             .addOperator("+=" , UMBA_TOKENIZER_TOKEN_OPERATOR_ADDITION_ASSIGNMENT           )
                                             .addOperator("-=" , UMBA_TOKENIZER_TOKEN_OPERATOR_SUBTRACTION_ASSIGNMENT        )
                                             .addOperator("*=" , UMBA_TOKENIZER_TOKEN_OPERATOR_MULTIPLICATION_ASSIGNMENT     )
                                             .addOperator("/=" , UMBA_TOKENIZER_TOKEN_OPERATOR_DIVISION_ASSIGNMENT           )
                                             .addOperator("%=" , UMBA_TOKENIZER_TOKEN_OPERATOR_MODULO_ASSIGNMENT             )
                                             .addOperator("&=" , UMBA_TOKENIZER_TOKEN_OPERATOR_BITWISE_AND_ASSIGNMENT        )
                                             .addOperator("|=" , UMBA_TOKENIZER_TOKEN_OPERATOR_BITWISE_OR_ASSIGNMENT         )
                                             .addOperator("^=" , UMBA_TOKENIZER_TOKEN_OPERATOR_BITWISE_XOR_ASSIGNMENT        )
                                             .addOperator("<<=", UMBA_TOKENIZER_TOKEN_OPERATOR_BITWISE_SHIFT_LEFT_ASSIGNMENT )
                                             .addOperator(">>=", UMBA_TOKENIZER_TOKEN_OPERATOR_BITWISE_SHIFT_RIGHT_ASSIGNMENT)
                                             .addOperator(","  , UMBA_TOKENIZER_TOKEN_OPERATOR_COMMA)
                                             .addOperator("->" , UMBA_TOKENIZER_TOKEN_OPERATOR_MEMBER_OF_POINTER             )
                                             .addOperator("->*", UMBA_TOKENIZER_TOKEN_OPERATOR_POINTER_TO_MEMBER_OF_POINTER  )
                                             .addOperator(".*" , UMBA_TOKENIZER_TOKEN_OPERATOR_POINTER_TO_MEMBER_OF_OBJECT   )
                                             .addOperator(":"  , UMBA_TOKENIZER_TOKEN_OPERATOR_TERNARY_ALTERNATIVE           )
                                             .addOperator("?"  , UMBA_TOKENIZER_TOKEN_OPERATOR_TERNARY_CONDITIONAL           )
                                             .addOperator("::" , UMBA_TOKENIZER_TOKEN_OPERATOR_SCOPE_RESOLUTION              )
                                             .addOperator(";"  , UMBA_TOKENIZER_TOKEN_OPERATOR_EXPRESSION_END                )
                                             //.addOperator( )


                                             .addStringLiteralParser("\'", &cppEscapedSimpleQuotedStringLiteralParser, UMBA_TOKENIZER_TOKEN_CHAR_LITERAL)
                                             .addStringLiteralParser("\"", &cppEscapedSimpleQuotedStringLiteralParser, UMBA_TOKENIZER_TOKEN_STRING_LITERAL)
                                             .addStringLiteralParser("<" , &simpleQuotedStringLiteralParser, UMBA_TOKENIZER_TOKEN_ANGLE_BACKETS_STRING_LITERAL)


                                             .makeTokenizer();



    bool bOk = true;

    //using tokenizer_type      = std::decay<decltype(tokenizer)>;


    using tokenizer_type       = decltype(tokenizer);
    using InputIteratorType    = typename tokenizer_type::iterator_type;
    using tokenizer_char_type  = typename tokenizer_type::value_type;
    using string_type          = typename tokenizer_type::string_type;
    using messages_string_type = typename tokenizer_type::messages_string_type;
    using token_parsed_data    = typename tokenizer_type::token_parsed_data;


    bool inPreprocessor = false;
    bool inInclude      = false;

    tokenizer.setResetCharClassFlags('#', umba::tokenizer::CharClass::none, umba::tokenizer::CharClass::opchar); // Ничего не устанавливаем, сбрасываем opchar

    tokenizer.tokenHandler = [&]( auto &tokenizer
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


                                 // Обрабатываем только файлы, которые инклюдятся через кавычки, а не угловые скобки
                                 if (tokenType==UMBA_TOKENIZER_TOKEN_STRING_LITERAL)
                                 {
                                     //inputFilename
                                     auto stringLiteralData = std::get<typename tokenizer_type::StringLiteralData>(parsedData);
                                     auto dataStr = string_type(stringLiteralData.data);

                                     if (inInclude)
                                     {
                                         auto path = umba::filename::getPath(inputFilename);
                                         // Файл может быть задан относительно текущего файла
                                         // или искаться в инклюд путях

                                         auto nameRelativeToCurFile = umba::filename::makeCanonical(umba::filename::appendPath(path, dataStr));
                                         if (umba::filesys::isPathExist(nameRelativeToCurFile) && umba::filesys::isPathFile(nameRelativeToCurFile))
                                             inputFiles.emplace_back(nameRelativeToCurFile);
                                         else
                                         {
                                             auto nameRelativeToSrcPath = umba::filename::makeCanonical(umba::filename::appendPath(srcPath, dataStr));
                                             if (umba::filesys::isPathExist(nameRelativeToSrcPath) && umba::filesys::isPathFile(nameRelativeToSrcPath))
                                                 inputFiles.emplace_back(nameRelativeToSrcPath);
                                         }
                                     }
                                 }

                                 return true;
                             };


    // Фильтры, установленные позже, отрабатывают раньше

    tokenizer.installTokenFilter<umba::tokenizer::filters::CcPreprocessorFilter<tokenizer_type> >();
    tokenizer.installTokenFilter<umba::tokenizer::filters::SimpleSuffixGluingFilter<tokenizer_type> >();

    // if (inputFiles.empty())
    // {
    //     std::cout << "No input files taken\n";
    //     return 1;
    // }

    #if defined(WIN32) || defined(_WIN32)
        marty_cpp::ELinefeedType outputLinefeed = marty_cpp::ELinefeedType::crlf;
    #else
        marty_cpp::ELinefeedType outputLinefeed = marty_cpp::ELinefeedType::lf;
    #endif

    // std::string srcPath;
    // std::string srcFile;
    // std::string dstPath;

    std::cout << "Umba Header Copy Tool v1.0\n\n";

    //inputFiles.emplace_back( umba::filename::makeCanonical(umba::filename::appendPath(srcPath, srcFile)));
    //inputFiles.emplace_back( umba::filename::makeCanonical(umba::filename::makeAbsPath(srcFile, srcPath)));
    inputFiles.emplace_back( umba::filename::makeCanonical(srcFile));

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

        // if (text.back()!='\n' && text.back()!='\r')
        // {
        //     std::cout << "Warning: no linefeed at end of file\n";
        // }

        //oss = std::ostringstream();
        bOk = true;

        //oss<<"<!DOCTYPE html>\n<html>\n<head>\n<meta charset=\"utf-8\"/>\n<style>\n" << cssStyle << "\n</style>\n</head>\n<body>\n<pre>\n";

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
        bool isRel = umba::filename::makeRelPath( relName, srcPath, f );

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


