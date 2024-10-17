/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

//
// fastcsv2json++:
// command line utility for fast csv to json array 
// conversion of large files
//
// Copyright © 2024 Lucas Tsatiris. All rights reserved. 
// 

#if __cplusplus < 202002L
#error C++20 compiler required.
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <string_view>
#include <algorithm>


// version information
constexpr char programname[] = "fastcsv2json++";
constexpr unsigned VERSION_MAJOR = 0;	// major
constexpr unsigned VERSION_MINOR = 1;	// minor
constexpr unsigned VERSION_PATCH = 0;	// patch

constexpr int MAX_TOKEN_COUNT = 4096;		// max token count per csv line
constexpr int STRING_RESERVE_SIZE = 256;	// std::string reserve chars

// conversion data
struct CSV2JSONData
{
  std::istream *in = &std::cin;						// input stream
  std::ostream *out = &std::cout;					// output stream
  std::array<std::string, MAX_TOKEN_COUNT> tokens;	// csv line tokens
  std::vector<std::string> s_header;				// csv header std::string
  std::vector<std::string_view> header;				// csv header std::string_view
  std::vector<char> replacewithspace;				// character to replace with space from input
  std::vector<char> erasechars;		    			// character to erase from input
  std::string inputline;							// input line buffer
  std::string outputline;							// output buffer
  std::string_view delimiter = ",";					// delimiter, default comma
  std::string infilepath = "";						// input file path
  std::string outfilepath = "";						// output file path
  std::size_t validtokencount = 0;	    			// valid token count
  unsigned line_counter = 0;						// line counter
};

using CData = struct CSV2JSONData;

// tokenize inputline into tokens
// returns: the token count
static const std::size_t
Tokenize(CData & data)
{
  const std::string_view
  inputline (data.inputline);

  std::string::size_type
  start = 0,
  end = inputline.find (data.delimiter);

  std::size_t column = 0;

  while (end != std::string::npos)
  {
    data.tokens[column++] = inputline.substr (start, end - start);
    start = end + 1; // Move past the delimiter
    end = inputline.find (data.delimiter, start);
  }

  // add the last token
  data.tokens[column++] = inputline.substr (start);
  data.tokens[column - 1].pop_back ();

  return column;
}

// tokenize inputline into tokens using delimeter
// returns: the token count
static const std::size_t
TokenizeLine (CData & data)
{
  data.inputline.append (" "); // iostream discards last character
  auto ntokens = Tokenize (data);

  // when no tokens or too many tokens found, do nothing
  if (ntokens == 0 || ntokens > MAX_TOKEN_COUNT) [[unlikely]]
    return 0;

  // the first line is the header
  if (data.line_counter == 1) [[unlikely]]
  {
    // copy tokens to std::string header
    std::copy(data.tokens.begin(), data.tokens.begin() + ntokens,
              std::back_inserter(data.s_header));

    // copy std::string header to std::string_view for performance
    for (const auto & header : data.s_header)
      data.header.push_back (std::string_view (header));

    // valid token count equals the token count of the header
    data.validtokencount = ntokens;

    // reserve chars for tokens to avoid string resize costs
    for (unsigned counter = 0; counter < ntokens; counter++)
      data.tokens[counter].reserve (STRING_RESERVE_SIZE);
  }

  return ntokens;
}

// get a line from istream into inputline
// returns: size of inputline, 0 on eof or error
static const std::string::size_type
GetLine (CData & data)
{
  data.line_counter ++;

  return
    std::getline(*data.in, data.inputline) ?
    data.inputline.size () : 0;
}

// generate json
// returns 0 on success, 1 otherwise
static int
GenerateJson (CData & data)
{
  constexpr char
    dquote = '"',
    comma = ',',
    space = ' ',
    dquote_semi[] = "\":\"";

  // reserve std::string buffer to avoid often resize
  data.inputline.reserve (STRING_RESERVE_SIZE * 4);
  data.outputline.reserve (STRING_RESERVE_SIZE * 4);

  // set input path. -i command line argument
  std::ifstream is;
  if (data.infilepath != "")
  {
    is.open (data.infilepath);
    data.in = &is;
  }

  // set output path. -o command line argument
  std::ofstream os;
  if (data.outfilepath != "")
  {
    os.open (data.outfilepath);
    data.out = &os;
  }

  // decouple iostream from stdio
  std::ios::sync_with_stdio(false);
  data.in->tie(nullptr);
  data.out->tie(nullptr);

  // begin a json array
  *data.out << '[';

  // until eof or error
  while (GetLine (data))
  {
    // replace char with space. -r command line argument
    if (data.replacewithspace.size () != 0)
    {
      for (const auto & schar : data.replacewithspace)
        std::ranges::replace (data.inputline, schar, space);
    }

    // erase characters. -e command line argument
    if (data.erasechars.size () != 0)
    {
      for (const auto & echar : data.erasechars)
        std::erase (data.inputline, echar);
    }

    // comma after json record
    if (data.line_counter > 2) [[likely]]
      *data.out << comma << '\n';

    const auto ntokens = TokenizeLine (data);
    if (data.validtokencount == ntokens) [[likely]] // csv must be valid
    {
      // begin json record
      data.outputline = '{';
      for (unsigned counter = 0; counter < ntokens; counter++)
      {
        data.outputline += dquote;
        data.outputline += data.header[counter];
        data.outputline += dquote_semi;
        data.outputline += data.tokens[counter];
        data.outputline += dquote;
        if (ntokens - counter > 1)
          data.outputline += comma;
      }
      // end json record
      data.outputline += '}';

      // if inputline is not the header, send outline to output
      if (data.line_counter > 1) [[likely]]
        *data.out << data.outputline;
    }
  }

  // end json array and flush output buffer
  *data.out << ']';
  data.out->flush ();

  if (data.infilepath != "")
    is.close ();

  if (data.outfilepath != "")
    os.close ();

  return 0;
}

// help screen
int
Help (CData & data) noexcept
{
  std::cerr << "Usage: " << programname << " [OPTION]" << '\n';
  std::cerr << "Convert csv to json array" << '\n' << '\n';
  std::cerr << "Options:" << '\n' << '\n';
  std::cerr <<
            "-d, --delimiter            Delimiter as pipe, comma, semicolumn, column, space" << '\n' <<
            "                           or tab. Default is comma" << '\n' <<
            "-h, --help                 This help screen" << '\n' <<
            "-i, --infile               Input file path, default STDIN" << '\n' <<
            "-o, --outfile              Output file path, default STDOUT" << '\n' <<
            "-r, --replace-with-space   Replace comma, semicolumn, column, tab, backslash," << '\n' <<
            "                           lf, cr, dquote, squote and slash characters of input" << '\n' <<
            "                           with a space. Can be used multiple times" << '\n' <<
            "-e, --erase-char           Remove comma, semicolumn, column, tab, backslash," << '\n' <<
            "                           lf, cr, dquote, squote, slash and space characters" << '\n' <<
            "                           from input. Can be used multiple times" << '\n' <<
            "-v, --version              Version information, license and copyright" << '\n' << '\n' <<
            "example: " << programname << " -d pipe < myfile.csv > myfile.json" << '\n';

  std::cerr << '\n';
  return 1;
}

// hash function for std::string switch
constexpr unsigned int
hash(const char *s, int off = 0) noexcept
{
  return !s[off] ? 5381 : (hash(s, off+1)*33) ^ s[off];
}

// parse command line arguments
static int
ParseArguments (int argc, char *argv[], CData & data)
{
  std::vector<std::string> argument;

  for (int c = 0; c < argc; c++)
    argument.push_back (std::string (argv[c]));

  int result = 0, counter = 1;

  while (counter < argc)
  {
    if (argument.at (counter) == "-d" ||
             argument.at (counter) == "--delimiter") // delimiter
    {
      counter ++;
      if (counter < argc)
      {
        switch (hash (argv[counter]))
        {
        case hash ("pipe") :
          data.delimiter = "|";
          break;
        case hash ("comma") :
          data.delimiter = ",";
          break;
        case hash ("semicolumn") :
          data.delimiter = ";";
          break;
        case hash ("column") :
          data.delimiter = ":";
          break;
        case hash ("space") :
          data.delimiter = " ";
          break;
        case hash ("tab") :
          data.delimiter = "\t";
          break;
        default:
          std::cerr << "Unknown delimiter: " << argument.at (counter) << '\n';
          result = 1;
        }
      }
    }
    else if (argument.at (counter) == "-i" ||
             argument.at (counter) == "--infile")	// input file
    {
      counter ++;
      if (counter < argc)
        data.infilepath = argument.at (counter);
    }
    else if (argument.at (counter) == "-o" ||
             argument.at (counter) == "--outfile")	// output file
    {
      counter ++;
      if (counter < argc)
        data.outfilepath = argument.at (counter);
    }
    else if (argument.at (counter) == "-r" ||
             argument.at (counter) == "--replace-with-space")	// character from input
      // to replace with space
    {
      counter ++;
      if (counter < argc)
      {
        switch (hash (argv[counter]))
        {
        case hash ("pipe") :
          data.replacewithspace.push_back ('|');
          break;
        case hash ("comma") :
          data.replacewithspace.push_back (',');
          break;
        case hash ("semicolumn") :
          data.replacewithspace.push_back (';');
          break;
        case hash ("column") :
          data.replacewithspace.push_back (':');
          break;
        case hash ("tab") :
          data.replacewithspace.push_back (9);
          break;
        case hash ("backslash") :
          data.replacewithspace.push_back ('\\');
          break;
        case hash ("lf") :
          data.replacewithspace.push_back (10);
          break;
        case hash ("cr") :
          data.replacewithspace.push_back (13);
          break;
        case hash ("squote") :
          data.replacewithspace.push_back ('\'');
          break;
        case hash ("dquote") :
          data.replacewithspace.push_back ('\"');
          break;
        case hash ("slash") :
          data.replacewithspace.push_back ('/');
          break;
        default:
          std::cerr << "Unknown character: " << argument.at (counter) << '\n';
          result = 1;
        }
      }
    }
    else if (argument.at (counter) == "-e" ||
             argument.at (counter) == "--erase-char")	// character from input
      // to erase
    {
      counter ++;
      if (counter < argc)
      {
        switch (hash (argv[counter]))
        {
        case hash ("pipe") :
          data.erasechars.push_back ('|');
          break;
        case hash ("comma") :
          data.erasechars.push_back (',');
          break;
        case hash ("semicolumn") :
          data.erasechars.push_back (';');
          break;
        case hash ("column") :
          data.erasechars.push_back (':');
          break;
        case hash ("space") :
          data.erasechars.push_back (' ');
          break;
        case hash ("tab") :
          data.erasechars.push_back (9);
          break;
        case hash ("backslash") :
          data.erasechars.push_back ('\\');
          break;
        case hash ("lf") :
          data.erasechars.push_back (10);
          break;
        case hash ("cr") :
          data.erasechars.push_back (13);
          break;
        case hash ("squote") :
          data.erasechars.push_back ('\'');
          break;
        case hash ("dquote") :
          data.erasechars.push_back ('\"');
          break;
        case hash ("slash") :
          data.erasechars.push_back ('/');
          break;
        default:
          std::cerr << "Unknown character: " << argument.at (counter) << '\n';
          result = 1;
        }
      }
    }
    else if (argument.at (counter) == "-h" ||
             argument.at (counter) == "--help")	// help
    {
      result = Help (data);
    }
    else if (argument.at (counter) == "-v" ||
             argument.at (counter) == "--version")	// version
    {
      std::cerr
          << programname << ' '
          << VERSION_MAJOR << '.'
          << VERSION_MINOR << '.'
          << VERSION_PATCH << '\n'
          << "Copyright (C) 2024 Lucas Tsatiris." << '\n' << '\n'
          << "This is free software. You may redistribute copies of it under the terms of" << '\n'
          << "the GNU General Public License <https://www.gnu.org/licenses/gpl.html>." << '\n'
          << "There is NO WARRANTY, to the extent permitted by law" << '\n' << '\n'
          << "Written by Lucas Tsatiris." << '\n';

      result = 1;
    }
    else
    {
      std::cerr << "Unknown argument: " << argument.at (counter) << '\n';
      result = 1;
    }

    if (result != 0)
      return result;
    counter ++;
  }

  return result;
}

// returns 0 on success, a positive int otherwise
int
main (int argc, char *argv[])
{
  CData data;

  auto result = ParseArguments (argc, argv, data);
  return result != 0 ? result : GenerateJson (data);
}
