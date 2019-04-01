# Code Style Guide

All of the C++ code in the project is styled, you should run any changes you make through astyle before pushing a pull request.

We are using astyle version 3.0.1. Version 3.1 should also work, though there are a few cases where they disagree and require annotation.

Blocks of code can be passed through astyle to ensure that their formatting is correct:

    astyle --style=1tbs --attach-inlines --indent=spaces=4 --align-pointer=name --max-code-length=100 --break-after-logical --indent-classes --indent-preprocessor --indent-switches --indent-col1-comments --min-conditional-indent=0 --pad-oper --unpad-paren --pad-paren-in --add-brackets --convert-tabs

These options are mirrored in `.astylerc`, `Cataclysm-DDA.sublime-project` and `doc/CODE_STYLE.txt`

For example, from `vi`, set marks a and b around the block, then:

    :'a,'b ! astyle  --style=1tbs --attach-inlines --indent=spaces=4 --align-pointer=name --max-code-length=100 --break-after-logical --indent-classes --indent-preprocessor --indent-switches --indent-col1-comments --min-conditional-indent=0 --pad-oper --unpad-paren --pad-paren-in --add-brackets --convert-tabs

## Code Example

Here's an example that illustrates the most common points of style:

````c++
int foo( int arg1, int *arg2 )
{
    if( arg1 < 5 ) {
        switch( *arg2 ) {
            case 0:
                return arg1 + 5;
                break;
            case 1:
                return arg1 + 7;
                break;
            default:
                return 0;
                break;
        }
    } else if( arg1 > 17 ) {
        int i = 0;
        while( i < arg1 ) {
            printf( _( "Really long message that's pointless except for the number %d and for its "
                       "length as it's illustrative of how to break strings properly.\n" ), i );
        }
    }
    return 0;
}
````
