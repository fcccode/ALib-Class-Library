﻿// #################################################################################################
//  aworx::lib::lox::loggers - ALox Logging Library
//
//  Copyright 2013-2018 A-Worx GmbH, Germany
//  Published under 'Boost Software License' (a free software license, see LICENSE.txt)
// #################################################################################################
/** @file */ // Hello Doxygen

// include ALox main header first...
#if !defined (HPP_ALIB_ALOX)
    #include "alib/alox/alox.hpp"
#endif

// then, set include guard
#ifndef HPP_ALOX_WINDOWS_CONSOLE_LOGGER
#define HPP_ALOX_WINDOWS_CONSOLE_LOGGER 1

#if defined( _WIN32 )

// #################################################################################################
// includes
// #################################################################################################
#if !defined (HPP_ALOX_CORE_TEXTLOGGER_TEXTLOGGER)
    #include "alib/alox/core/textlogger/textlogger.hpp"
#endif


namespace aworx { namespace lib { namespace lox { namespace loggers {

/** ************************************************************************************************
 * A logger that logs all messages to the standard output <em>cout</em>.
 * The name of the \e Logger defaults to "WINDOWS_CONSOLE".
 *
 * \alox text logger escape sequences (see class \ref aworx::lib::lox::ESC "ESC")
 * are translated to Windows API calls (<em>SetConsoleTextAttribute</em>) which manipulates
 * colors the text output.
 * There is not 100% match between windows console capabilities and the definitions in
 * \ref aworx::lib::lox::ESC "ESC". Especially, ESC style attributes are ignored.
 *
 * Foreground and background colors can be set to be either light/dark or dark/light. This improves
 * the readability of log output a lot and even allows to read if foreground and background colors
 * are the same (they then still differ). However, the right setting for this is dependent on
 * the color scheme of the final output device (window). To manipulate the right setting, see field
 * #UseLightColors and also configuration variable
 * [ALOX_CONSOLE_LIGHT_COLORS](../group__GrpALoxConfigVars.html).
 *
 * In the constructor, a default format string and some other definitions in member
 * \ref MetaInfo get set to include ESC sequences. Of-course, these publicly accessible
 * format attributes can be customized after creation.
 *
 * \note This class can not enable the output console (which receives \alox
 * log data) to support the windows API call <em>SetConsoleTextAttribute</em>.
 * The opposite is right: this class should be used only if the output console supports such calls.
 **************************************************************************************************/
class WindowsConsoleLogger : public aworx::lib::lox::core::textlogger::TextLogger
{
    // #############################################################################################
    // Internals
    // #############################################################################################
    protected:
        /// The attributes that were set on the windows console at the time this object was created.
        WORD        originalConsoleAttributes;

    // #############################################################################################
    // public fields
    // #############################################################################################
    public:

        /**
         * Foreground and background colors chosen by this class might differ in their intensity.
         * This increases the overall readability by increasing the contrast.
         * If the background color of a console window is dark, then the background colors of
         * colored log output should be darker colors than the foreground colors - and vice versa.
         *
         * Depending on the setting of this field, \alox
         * \ref aworx::lib::lox::ESC "escape codes" for colors are translated to normal ANSI colors or
         * lighter ones:
         * - If this field is \ref LightColorUsage "LightColorUsage::Never", light colors are
         *   never used.
         * - If this field is \ref LightColorUsage "LightColorUsage::ForegroundLight", foreground
         *   colors will be light colors and background colors dark ones. This is the default.
         * - If \ref LightColorUsage "LightColorUsage::ForegroundDark", background colors will be
         *   light colors and foreground colors dark ones.
         *
         * The configuration variable [ALOX_CONSOLE_LIGHT_COLORS](../group__GrpALoxConfigVars.html)
         * allows to externally modify this flag. It is read once within the constructor .
         */
        LightColorUsage    UseLightColors;

        /**
         * The code page that is used for the console log output. For possible values refer to
         * [Microsoft documentation] (https://msdn.microsoft.com/EN-US/library/windows/desktop/dd317756%28v=vs.85%29.aspx)
         *
         * This defaults to 65001, which is UTF-8 unicode encoding.
         *
         * This flag can be modified from outside any time. If the value is not equal to 0,
         * the code page of the output console is set prior to each log output.
         *
         * The configuration variable [ALOX_CODE_PAGE](../group__GrpALoxConfigVars.html)
         * is evaluated within the constructor of this class, to allow to modifying the codepage at
         * runtime.
         */

        UINT            CodePage                                                            =65001;

    // #############################################################################################
    // Constructor/destructor
    // #############################################################################################
    public:
        /** ****************************************************************************************
         *  Creates a WindowsConsoleLogger.
         * @param name  (Optional) The name of the \e Logger, defaults to "WINDOWS_CONSOLE".
         ******************************************************************************************/
        ALIB_API
        explicit            WindowsConsoleLogger( const NString& name= nullptr  );

        /** ****************************************************************************************
         * Destructs a WindowsConsoleLogger
         ******************************************************************************************/
        ALIB_API
        virtual            ~WindowsConsoleLogger();


    // #############################################################################################
    // Abstract method implementations
    // #############################################################################################
    protected:
        /** ****************************************************************************************
         *  The implementation of the abstract method of parent class TextLogger.
         *  Logs messages to the basic output stream 'cout' and sets windows console colors
         *  by invoking corresponding Windows API methods..
         *
         * @param domain      The <em>Log Domain</em>.
         * @param verbosity   The verbosity. This has been checked to be active already on this
         *                    stage and is provided to be able to be logged out only.
         * @param msg         The log message.
         * @param scope       Information about the scope of the <em>Log Statement</em>..
         * @param lineNumber  The line number of a multi-line message, starting with 0. For
         *                    single line messages this is -1.
         ******************************************************************************************/
        ALIB_API
        virtual void logText(  core::Domain&     domain,
                               Verbosity         verbosity,
                               AString&          msg,
                               core::ScopeInfo&  scope,
                               int               lineNumber);

        /** ****************************************************************************************
         *  Empty implementation, not needed for this class
         ******************************************************************************************/
        ALIB_API
        virtual void notifyMultiLineOp (lib::lang::Phase )    {  }

}; // class WindowsConsoleLogger

}}}// namespace aworx[::lib::lox::loggers]

/// Type alias in namespace #aworx.
using     WindowsConsoleLogger=     aworx::lib::lox::loggers::WindowsConsoleLogger;

}  // namespace aworx

#endif // defined( _WIN32 )

#endif // HPP_ALOX_WINDOWS_CONSOLE_LOGGER

