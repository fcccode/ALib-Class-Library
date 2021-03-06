// #################################################################################################
//  ALib - A-Worx Utility Library
//
//  Copyright 2013-2018 A-Worx GmbH, Germany
//  Published under 'Boost Software License' (a free software license, see LICENSE.txt)
// #################################################################################################
#include "alib/alib.hpp"

#if !defined (HPP_ALIB_LANG_EXCEPTION)
    #include "alib/lang/exception.hpp"
#endif
#if !defined (HPP_ALIB_CONFIG_CONFIGURATION)
    #include "alib/config/configuration.hpp"
#endif
#if !defined (HPP_ALIB_CONFIG_INI_FILE)
    #include "alib/config/inifile.hpp"
#endif
#if !defined (HPP_ALIB_SYSTEM_DIRECTORY)
    #include "alib/system/directory.hpp"
#endif
#if !defined (HPP_ALIB_SYSTEM_PROCESSINFO)
    #include "alib/system/process.hpp"
#endif
#if !defined (HPP_ALIB_STRINGS_UTIL_TOKENIZER)
    #include "alib/strings/util/tokenizer.hpp"
#endif
#if !defined (HPP_ALIB_COMPATIBILITY_STD_IOSTREAM)
    #include "alib/compatibility/std_iostream.hpp"
#endif
#if !defined (HPP_ALIB_STRINGS_UTIL_SPACES)
    #include "alib/strings/util/spaces.hpp"
#endif
#if !defined (HPP_ALIB_STRINGS_FORMAT_SIMPLETEXT)
    #include "alib/strings/format/simpletext.hpp"
#endif
#if !defined (HPP_ALIB_STRINGS_UTIL_STRING_IO)
    #include "alib/strings/util/stringio.hpp"
#endif


#include <fstream>
#if defined(_MSC_VER)
    #include <algorithm>
#endif


namespace aworx { namespace lib { namespace config {

// #################################################################################################
// static/globals
// #################################################################################################
String                IniFile::DefaultFileExtension                                  = ASTR(".ini");

// #################################################################################################
// class InMemoryPlugin::Entry
// #################################################################################################
void IniFile::Entry::ToVariable( const InMemoryPlugin& parent, Variable& variable )
{
    // if this had been done before, use base method
    if ( Values.size() > 0 )
    {
        InMemoryPlugin::Entry::ToVariable( parent, variable );
        return;
    }

    // store delim and comment
    Delim= variable.Delim;
    variable.Comments._()._( Comments );

    // parse the INI file content

    //-----  remove INI-File specific from raw value -----
    String512 raw;
    ALIB_WARN_ONCE_PER_INSTANCE_DISABLE( raw,  ReplaceExternalBuffer);
    raw._( RawValue );

    // remove '='
    raw.TrimStart();
    if ( raw.CharAtStart() == '=' )
        raw.DeleteStart(1).TrimStart();
    else
    {
        ALIB_WARNING( ASTR("No equal sign in INI-file variable \""), variable.Fullname.ToCString(), ASTR("\".") );
    }


    // remove "\\n"
    integer startIdx= 0;
    while ( (startIdx= raw.IndexOf( '\n', startIdx )) >= 0 )
    {
        // find \\n and trim around this location
        int delLen= 2;
        if ( raw.CharAt( --startIdx) == '\r' )
        {
            delLen= 3;
            --startIdx;
        }
        ALIB_ASSERT( raw.CharAt(startIdx) == '\\' );
        raw.Delete( startIdx, delLen );

        startIdx= raw.TrimAt( startIdx );
        if( startIdx >= raw.Length() )
            break;

        // remove endquote of first line and start quote of second
        if( startIdx >0 && raw[startIdx-1] == '"' && raw[startIdx] == '"' )
        {
            startIdx--;
            raw.Delete( startIdx, 2);
        }


        // if now the next value is starting with a comment symbol, we remove to the next \n
        for(;;)
        {
            character c= raw.CharAt( startIdx );
            if (     c != '#'
                &&   c != ';'
                && ( c != '/' || raw.CharAt( startIdx + 1 ) != '/' ) )
                break;

            integer idx= raw.IndexOf( '\n', startIdx );
            if (idx < 0 ) idx= raw.Length();
            raw.Delete( startIdx, idx - startIdx + 1 );
            if( startIdx >= raw.Length() )
                break;
            startIdx= raw.TrimAt( startIdx );
        }
    }

    // now convert
    parent.StringConverter->LoadFromString( variable, raw );

    // copy the parsed values back to our entry
    for( int i= 0; i < variable.Size() ; i++ )
        Values.emplace_back( AString( variable.GetString( i ) ) );
}

void IniFile::Entry::FromVariable( const InMemoryPlugin& parent, Variable& variable )
{
    RawValue._();
    InMemoryPlugin::Entry::FromVariable( parent, variable );
}


// #################################################################################################
// Constructor/Destructor
// #################################################################################################
IniFile::IniFile( const String& fileName )
: InMemoryPlugin( NullString )
{

    // don't read anything
    if ( fileName.StartsWith( ASTR("*") ) )
    {
        AutoSave= false;
        return;
    }

    if ( fileName.CharAtStart() != '/'
      #if defined(_WIN32)
         && fileName.CharAtStart() != '\\'
         && fileName.CharAt(1)     != ':'
      #endif
       )
    {
        Directory dir( Directory::SpecialFolder::HomeConfig );
        FileName._( dir.Path      )
                ._( DirectorySeparator );
    }

    if ( fileName.IsEmpty()  )
    {
        FileName._( ProcessInfo::Current().Name  );

        #if defined(_WIN32)
            if( FileName.EndsWith( ASTR(".exe") ) )
                FileName.DeleteEnd( 4 );
        #endif

        FileName._( IniFile::DefaultFileExtension );
    }
    else
        FileName <<  fileName;

    ReadFile();

}

void  IniFile::Reset()
{
    InMemoryPlugin::Reset();
    FileComments.Clear();
    LinesWithReadErrors.clear();
}

IniFile::Section*  IniFile::createSection(const String& sectionName )
{
    auto* section= new IniFile::Section( sectionName );
    String comments= lib::CONFIG.GetTry(String128(ASTR("INI_CMT_"))._(sectionName) );
    if( comments.IsNotEmpty() )
    {
        SimpleText text;
        text.LineWidth= 100;
        text.AddMarked( comments );
        text.RemoveLastNewLine();
        section->Comments << text.Text;
    }
    return section;
}


// #################################################################################################
// Read/Write files
// #################################################################################################

//! @cond NO_DOX
bool startsWithCommentSymbol( Substring& subs );
bool startsWithCommentSymbol( Substring& subs )
{
    integer i= String(ASTR("#;/")).IndexOfOrLength( subs.CharAtStart() );
    return       i <  2
            || ( i == 2 && subs.Length() > 1 && subs[1] == '/'  );
}
//! @endcond

void  IniFile::ReadFile()
{
    Reset();

    // open file
    ALIB_STD_TO_NARROW_TSTRING(FileName, nFileName)
    std::ifstream file( nFileName.ToCString() );


    if ( !file.is_open() )
    {
        int errNo= errno;

        // file does not exist ?
        if (   errNo == EnumValue(system::SystemErrors::enoent) )
            return;

        // other errors: throw
        auto& systemErrorMetaData= *EnumMetaData<system::SystemErrors>::GetSingleton();
        systemErrorMetaData.CheckLoad();
        throw Exception( ALIB_CALLER_NULLED,  system::SystemErrors(errNo),
                         systemErrorMetaData.Value<1>( system::SystemErrors(errNo) ),  errNo )
                   .Add( ALIB_CALLER_NULLED,  config::Exceptions::ErrorOpeningFile,
                         ASTR("INI-"), FileName                                                        );
    }

    AString     lineAS;
    String128   name;
    AString     value;
    AString     comments;
    Section*    actSection= dynamic_cast<IniFile::Section*>( Sections[0] );

    int         lineNo= 0;
    bool        fileHeaderRead= false;

    LinesWithReadErrors.clear();

    StringReader reader;
    reader.SetStream( &file );

    String16 separatorCharacters( '=' );
    separatorCharacters._( DefaultWhitespaces );

    while( !reader.IsEOF() )
    {
        reader.Read( lineAS );
        lineNo++;
        Substring line( lineAS );

        bool isEmpty=       line.Trim().IsEmpty();
        bool isCommentLine= startsWithCommentSymbol( line );

        if ( isCommentLine )
        {
            if ( comments.IsNotEmpty() )
                comments.NewLine();
            comments._(line);
            continue;
        }

        // still processing file header?
        if ( !fileHeaderRead )
        {
            fileHeaderRead= true;
            FileComments=   comments;
            comments.Clear();
        }

        // empty line?
        if ( isEmpty )
        {
            if ( comments.IsNotEmpty() )
                comments.NewLine();
            continue;
        }

        // section line
        if ( line.ConsumeChar('[') )
        {
            fileHeaderRead= true;

            // we do not care if there is no closing bracket. But if there is one, we remove it.
            if( !line.ConsumeCharFromEnd( ']' ) )
                LinesWithReadErrors.insert( LinesWithReadErrors.end(), lineNo );


            // search the section in our section list (if section existed already, new comments
            // are dropped)
            actSection= dynamic_cast<IniFile::Section*>( SearchOrCreateSection( line, comments ) );
            comments.Clear();

            continue;
        }

        // Variable line
        value.Clear();
        integer idx= line.IndexOfAny<Inclusion::Include>( separatorCharacters );
        if( idx < 0 )
        {
            name._()._( line );
            line.Clear();
        }
        else
        {
            name._()._( line, 0, idx );
            line.ConsumeChars( idx );
            value._(line);
        }

        // read continues as long as lines end with '\' (must not be '\\')
        while (    line.CharAtEnd()  == '\\'
                && (line.Length() == 1 || line.CharAt<false>( line.Length() -2 ) != '\\' ) )
        {
            value.NewLine();
            reader.Read( lineAS );
            if ( reader.IsEOF() )
            {
                // last line of the file ended with '\' !
                line.Clear();
                break;
            }
            lineAS.TrimEnd();
            line= lineAS;

            value._( lineAS );
        }

        // insert entry with raw value
        {
            IniFile::Entry* entry= dynamic_cast<IniFile::Entry*>( actSection->GetEntry( name, true ) );
            entry->Values  .clear();
            entry->Comments._()._( comments );
            entry->RawValue._()._( value );

            // if there is just no raw value, we add an empty string to the entries' values
            if ( value.IsEmpty() )
                entry->Values.insert( entry->Values.end(), AString() );
        }

        comments.Clear();

    }
    file.close();
}

void  IniFile::writeComments( StringWriter& writer, const AString& comments )
{
    // is empty when trimmed?
    if ( Substring(comments).Trim().IsEmpty() )
        return;

    // tokenize by NEWLINE character
    Tokenizer tknzr( comments, '\n' );
    tknzr.Whitespaces= ASTR(" \r\t"); // \n is not a whitespace

    while( tknzr.Next(lang::Whitespaces::Keep).IsNotNull() )
    {
        if ( !startsWithCommentSymbol( tknzr.Actual ) )
            writer.Write( DefaultCommentPrefix );
        writer.Write( tknzr.Actual );
        *writer.GetStream() << std::endl;
    }

    tknzr.Whitespaces=  DefaultWhitespaces;
}

//! @cond NO_DOX
namespace {
    int getAssignmentPos( const AString& value, const String& alignmentSeparator )
    {
        integer idx= value.IndexOf( alignmentSeparator );
        if( idx > 0 )
        {
            integer idxQuote= value.IndexOfOrLength( '"' );
            if (  idxQuote > idx )
                return static_cast<int>(idx);
        }
        return -1;
    }
}
//! @endcond

void IniFile::WriteFile()
{
    // read all variables
    ALIB_STD_TO_NARROW_TSTRING(FileName,nFileName)
    std::ofstream outputFileStream ( nFileName.ToCString() );
    if ( !outputFileStream.is_open() )
    {
        int errNo= errno;
        auto& systemErrorMetaData= *EnumMetaData<system::SystemErrors>::GetSingleton();
        systemErrorMetaData.CheckLoad();
        throw Exception( ALIB_CALLER_NULLED, system::SystemErrors(errNo),
                         systemErrorMetaData.Value<1>( system::SystemErrors(errNo) ), errNo )
                  .Add(  ALIB_CALLER_NULLED, config::Exceptions::ErrorWritingFile,
                         "INI-", FileName                                                       );
    }

    StringWriter writer;
    writer.SetStream( &outputFileStream );

    // write file header
    if ( FileComments.IsNotEmpty() )
    {
        writeComments( writer, FileComments );
        outputFileStream << std::endl;
    }

    // loop over all sections
    int cntVars= 0;
    for ( InMemoryPlugin::Section* section : Sections )
    {
        // comments, name
        if ( cntVars > 0 )
            outputFileStream << std::endl;

        // write section comments and name
        writeComments( writer, section->Comments );
        if ( section->Name.IsNotEmpty() )
            writer.Write( String256() << '[' << section->Name << ']' << NewLine );

        // variables
        integer maxVarLength= 0;
        for ( InMemoryPlugin::Entry* entry : section->Entries )
            maxVarLength= (std::max)( maxVarLength, entry->Name.Length() );

        bool previousVarHasComments= true;
        for ( InMemoryPlugin::Entry* entry : section->Entries )
        {
            cntVars++;

            // write comments
            if( entry->Comments.IsNotEmpty() )
            {
                // we make an extra empty line if previous var had no comments
                if( !previousVarHasComments)
                    outputFileStream << std::endl;

                writeComments(writer, entry->Comments );
            }

            // write name =
            writer.Write( entry->Name );

            // either write raw value (if it was not used by the application)
            if ( dynamic_cast<IniFile::Entry*>(entry)->RawValue.IsNotEmpty() )
            {
                writer.Write( dynamic_cast<IniFile::Entry*>(entry)->RawValue );
            }

            // or write the values parsed by the software
            else
            {
                outputFileStream << '=';
                Spaces::Write( outputFileStream, maxVarLength - entry->Name.Length() + 1 );

                bool      isFirst=      true;
                String256 externalizedValue;
                ALIB_WARN_ONCE_PER_INSTANCE_DISABLE( externalizedValue, ReplaceExternalBuffer )

                //-------- write as single-line ----------
                if ( !EnumContains( entry->FmtHints, FormatHints::MultLine  ) )
                {
                    bool delimSpaces=  (! EnumContains( entry->FmtHints, FormatHints::NoDelimSpaces ) );
                    for( AString& value : entry->Values )
                    {
                        // write delim and backslash of previous line, newline and then spaces of actual line
                        if ( !isFirst )
                        {
                            ALIB_ASSERT_ERROR( entry->Delim != 0,
                                               ASTR("No delimiter given for multi-value variable \""),
                                               entry->Name.ToCString(), ASTR("\".") );

                            if( delimSpaces && FormatSpaceBeforeDelim)
                                outputFileStream << ' ';

                            writer.Write( String8() << entry->Delim );

                            if( delimSpaces && FormatSpaceAfterDelim)
                                outputFileStream << ' ';
                        }

                        // externalize value
                        Substring src( value );
                        externalizedValue._();
                        StringConverter->ExternalizeValue( src, externalizedValue, entry->Delim );
                        writer.Write( externalizedValue );
                        isFirst= false;
                    }
                }

                // ---------- write as multi-line ----------
                else
                {
                    ALIB_WARN_ONCE_PER_INSTANCE_DISABLE( externalizedValue,  ReplaceExternalBuffer);
                    integer    backSlashPos= 0;
                    integer    lastLineLen=  0;

                    // Get maximum position of attribute assignment char '=' or ':' (if exists)
                    int maxAttributeAssignPos= 0;
                    bool allAttrHavePrecedingBlanks= true;
                    if (entry->FormatAttrAlignment.IsNotEmpty() )
                    {
                        for( AString& value : entry->Values )
                        {
                            int attributeAssignPos= getAssignmentPos( value, entry->FormatAttrAlignment );
                            if ( attributeAssignPos > 0 )
                            {
                                if ( maxAttributeAssignPos < attributeAssignPos )
                                    maxAttributeAssignPos= attributeAssignPos;
                                allAttrHavePrecedingBlanks&= value.CharAt( attributeAssignPos - 1 ) == ' ';
                            }
                        }
                        if ( !allAttrHavePrecedingBlanks )
                            maxAttributeAssignPos += 1;
                    }

                    // loop over values of entry
                    for( AString& value : entry->Values )
                    {

                        // write delim and backslash of previous line, newline and then spaces of actual line
                        if ( !isFirst )
                        {
                            ALIB_ASSERT_ERROR( entry->Delim != 0,
                                               ASTR("No delimiter given for multi-value variable \""),
                                               entry->Name.ToCString(), ASTR("\".") );
                            writer.Write( String8() << entry->Delim );
                            lastLineLen++;

                            if ( backSlashPos < lastLineLen + 1 )
                                 backSlashPos=  lastLineLen + 4;

                            Spaces::Write( outputFileStream, backSlashPos - lastLineLen );

                            outputFileStream << '\\' << std::endl;

                            Spaces::Write( outputFileStream, maxVarLength + 2 ); // 2 for "= "
                        }

                        // externalize value
                        Substring src( value );
                        externalizedValue._();
                        StringConverter->ExternalizeValue( src, externalizedValue, entry->Delim );

                        // if first character is a INI comment char, then escape it
                        character firstChar= externalizedValue.CharAt(0);
                        if( !isFirst && (firstChar == '#' || firstChar == ';' ) )
                            externalizedValue.InsertAt(ASTR("\\"), 0 );

                        // if assignment, insert spaces to align assignments
                        if (entry->FormatAttrAlignment.IsNotEmpty() )
                        {
                            int attributeAssignPos= getAssignmentPos( externalizedValue, entry->FormatAttrAlignment );
                            if ( attributeAssignPos > 0 && attributeAssignPos < maxAttributeAssignPos )
                                externalizedValue.InsertChars( ' ',
                                                               maxAttributeAssignPos-attributeAssignPos,
                                                               attributeAssignPos + (FormatIncludeDelimInAttrAlignment ?
                                                                                      0 : entry->FormatAttrAlignment.Length() )
                                                              );
                        }
                        writer.Write( externalizedValue );

                        lastLineLen=  maxVarLength + 2  + externalizedValue.Length();
                        isFirst= false;
                    }
                }
            }
            outputFileStream << std::endl;

            // add an empty line if we have comments
            if( (previousVarHasComments= entry->Comments.IsNotEmpty() ) == true )
                outputFileStream << std::endl;
        }
    }

    // close file
    outputFileStream.close();
}


}}}// namespace [aworx::lib::config]

