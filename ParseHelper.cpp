/**
Copyright (c) 2014 Alex Tsui

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#include "ParseHelper.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>
#include "ParseListener.h"

#ifndef NDEBUG
void print(const ParseHelper::Indent& indent)
{
    std::string str = indent.Token;
    for (int i = 0; i < str.size(); ++i)
    {
        switch (str.at(i))
        {
            case ' ':
                str[i] = 's';
                break;
            case '\t':
                str[i] = 't';
                break;
        }
    }
    std::cout << str << "\n";
}
#endif

ParseHelper::Indent::
Indent( )
{ }

ParseHelper::Indent::
Indent( const std::string& indent ):
    Token( indent )
{ }

ParseHelper::ParseState::
ParseState( ParseHelper& parent_ ): parent( parent_ )
{ }

ParseHelper::ParseState::
~ParseState( )
{ }

bool ParseHelper::PeekIndent( const std::string& str, Indent* indent )
{
    if ( !str.size() || ! isspace(str[0]) )
        return false;

    int nonwhitespaceIndex = -1;
    for (int i = 0; i < str.size(); ++i)
    {
        if (!isspace(str[i]))
        {
            nonwhitespaceIndex = i;
            break;
        }
    }
    if (nonwhitespaceIndex == -1)
    {
        return false;
    }
    std::string indentToken = str.substr(0, nonwhitespaceIndex);
    indent->Token = indentToken;
    return true;
}

ParseHelper::ParseHelper( ):
    inContinuation( false )
{ }

void ParseHelper::process( const std::string& str )
{
#ifndef NDEBUG
    std::cout << "processing: (" << str << ")\n";
#endif

    boost::shared_ptr<BlockParseState> blockStatePtr;
    if (stateStack.size()
        && (blockStatePtr = boost::dynamic_pointer_cast<BlockParseState>(
            stateStack.back())))
    {
        if (blockStatePtr->process(str))
            return;
    }

    // standard state
    if ( !str.size() )
        return;

    { // check for unexpected indent
        Indent ind;
        bool isIndented = PeekIndent( str, &ind );
        if ( isIndented &&
            ! inContinuation )
        {
            reset( );
            ParseMessage msg( 1, "IndentationError: unexpected indent");
            broadcast( msg );
            return;
        }
    }

    // enter indented block state
    if ( str[str.size()-1] == ':' )
    {
        commandBuffer.push_back( str );

        boost::shared_ptr<ParseState> parseState(
            new BlockParseState( *this ) );
        stateStack.push_back( parseState );
        return;
    }

    if ( str[str.size()-1] == '\\' )
    {
        commandBuffer.push_back( str );
        inContinuation = true;
        return;
    }

    // handle single-line statement
    commandBuffer.push_back( str );
    flush( );
}

bool ParseHelper::buffered( ) const
{
    return commandBuffer.size( );
}

void ParseHelper::flush( )
{
    std::stringstream ss;
    for (int i = 0; i < commandBuffer.size(); ++i )
    {
        ss << commandBuffer[i] << "\n";
    }
    commandBuffer.clear();

    broadcast( ss.str() );
    // TODO: feed string to interpreter
}

void ParseHelper::reset( )
{
    inContinuation = false;
    stateStack.clear( );
    commandBuffer.clear( );
}

void ParseHelper::subscribe( ParseListener* listener )
{
    listeners.push_back( listener );
}

void ParseHelper::unsubscribeAll( )
{
    listeners.clear( );
}

void ParseHelper::broadcast( const ParseMessage& msg )
{
    // broadcast signal
    for (int i = 0; i < listeners.size(); ++i)
    {
        if (listeners[i])
        {
            listeners[i]->parseEvent(msg);
        }
    }
}
