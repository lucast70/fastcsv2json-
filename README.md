# fastcsv2jsonxx

Usage: fastcsv2json++ [OPTION]


Convert csv to json array

Options:

-d, --delimiter                    Delimiter as pipe, comma, semicolumn, column, space
                                            or tab. Default is comma
-h, --help                            This help screen
-i, --infile                             Input file path, default STDIN
-o, --outfile                         Output file path, default STDOUT
-r, --replace-with-space   Replace comma, semicolumn, column, tab, backslash,
                                            lf, cr, dquote, squote and slash characters of input
                                            with a space. Can be used multiple times
-e, --erase-char                 Remove comma, semicolumn, column, tab, backslash,
                                            lf, cr, dquote, squote, slash and space characters
                                            from input. Can be used multiple times
-v, --version                       Version information, license and copyright

example:  fastcsv2json++ -d pipe < myfile.csv > myfile.json
