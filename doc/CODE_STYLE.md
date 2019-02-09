# Code Style Guide

Current policy is to update code to the standard style when changing a substantial portion of it.

Note that versions of astyle before 2.05 will handle many c++11 constructs incorrectly.

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

Here's a helpful workflow to astyle some low-hanging fruit:

````shell
# Astyle all the things.
make astyle-all
# List the changed files.
# |             Sort by number of lines changed.
# |             |              Truncate away the first part of the list.
# |             |              |           Truncate away the summary line.
# |             |              |           |           Trim off the fields except the filename.
# |             |              |           |           |                 Revert the listed files to the upstream version.
git diff --stat | sort -g -k 3 | tail -201 | head -200 | cut -d ' ' -f 2 | xargs git checkout
# Refresh astyle blacklist with all the files that still fail.
astyle --dry-run --options=.astylerc src/*.cpp src/*.h tests/*.cpp tests/*.h | grep Formatted | cut -d ' ' -f 3 > astyle_blacklist
# Add the changed files, please examine the changes to make sure they make sense, astyle occasionally messes up.
git add -p
# commit and push!
git commit -m "Updated astyling coverage."
git push
````
