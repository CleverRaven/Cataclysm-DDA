# Contribute

**Opening new issue?** Please read [ISSUES.md](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/ISSUES.md) first.

Contributing to Cataclysm: Dark Days Ahead is easy — simply fork the repository here on GitHub, make your changes, and then send us a pull request.

Cataclysm:Dark Days Ahead is released under the Creative Commons Attribution ShareAlike 3.0 license. The code and content of the game is free to use, modify, and redistribute for any purpose whatsoever. See http://creativecommons.org/licenses/by-sa/3.0/ for details.
This means any contribution you make to the project will also be covered by the same license, and this license is irrevocable.

## Guidelines

There are a couple of guidelines we suggest sticking to:

* Add this repository as an `upstream` remote.
* Keep your `master` branch clean. This means you can easily pull changes made to this repository into yours.
* Create a new branch for each new feature or set of related bug fixes.
* Never merge from your local branches into your `master` branch. Only update that by pulling from `upstream/master`.

## Code Style

Current policy is to only update code to the standard style when changing a substantial portion of it, but **please** do this in a separate commit. See [CODE_STYLE](../doc/CODE_STYLE.md) for details.

## Translations

The translation of Cataclysm: DDA is done using Transifex.
Look at the [translation project](https://www.transifex.com/cataclysm-dda-translators/cataclysm-dda/) for an up-to-date list of supported languages.

See [TRANSLATING](../doc/TRANSLATING.md) for more information:

* [For translators](../doc/TRANSLATING.md#translators)
* [For developers](../doc/TRANSLATING.md#developers)
* [For maintainers](../doc/TRANSLATING.md#maintainers)

## Doxygen Comments

Extensive documentation of classes and class members will make the code more readable to new contributors. New doxygen comments for existing classes are a welcomed contribution.

Use the following template for commenting classes:
```c++
/**
 * Brief description
 *
 * Lengthy description with many words. (optional)
 */
class foo {
```

Use the following template for commenting functions:
```c++
/**
 * Brief description
 *
 * Lengthy description with many words. (optional)
 * @param param1 Description of param1 (optional)
 * @return Description of return (optional)
 */
int foo(int param1);
```

Use the following template for commenting member variables:
```c++
/** Brief description **/
int foo;
```

Helpful pages:
http://www.stack.nl/~dimitri/doxygen/manual/commands.html
http://www.stack.nl/~dimitri/doxygen/manual/markdown.html#markdown_std
http://www.stack.nl/~dimitri/doxygen/manual/faq.html

### Guidelines for adding documentation
* Doxygen comments should describe behavior towards the outside, not implementation, but since many classes in Cataclysm are intertwined, it's often necessary to describe implementation.
* Describe things that aren't obvious to newcomers just from the name.
* Don't describe redundantly: `/** Map **/; map* map;` is not a helpful comment.
* When documenting X, describe how X interacts with other components, not just what X itself does.

### Building the documentation for viewing it locally
* Install doxygen
* `doxygen doxygen_doc/doxygen_conf.txt `
* `firefox doxygen_doc/html/index.html` (replace firefox with your browser of choice)

## Example Workflow

#### Setup your environment

*(This only needs to be done once.)*

1. Fork this repository here on GitHub.

2. Clone your fork locally.

        $ git clone https://github.com/YOUR_USERNAME/Cataclysm-DDA.git
        # Clones your fork of the repository into the current directory in terminal

3. Set commit message template.

        $ git config --local commit.template .gitmessage

4. Add this repository as a remote.

        $ cd Cataclysm-DDA
        # Changes the active directory in the prompt to the newly cloned "Cataclysm-DDA" directory
        $ git remote add -f upstream https://github.com/CleverRaven/Cataclysm-DDA.git
        # Assigns the original repository to a remote called "upstream"

For further details about commit message guidelines please visit:
- [codeinthehole.com](https://codeinthehole.com/tips/a-useful-template-for-commit-messages/)
- [chris.beams.io](https://chris.beams.io/posts/git-commit/)
- [help.github.com](https://help.github.com/articles/closing-issues-using-keywords/)

#### Update your `master` branch

1. Make sure you have your `master` branch checked out.

        $ git checkout master

2. Pull the changes from the `upstream/master` branch.

        $ git pull --ff-only upstream master
        # gets changes from "master" branch on the "upstream" remote

 * Note: If this gives you an error, it means you have committed directly to your local `master` branch. [Click here for instructions on how to fix this issue](#why-does-git-pull---ff-only-result-in-an-error).

#### Make your changes

0. Update your `master` branch, if you haven't already.

1. For each new feature or bug fix, create a new branch.

        $ git branch new_feature
        # Creates a new branch called "new_feature"
        $ git checkout new_feature
        # Makes "new_feature" the active branch

2. Once you've committed some changes locally, you need to push them to your fork here on GitHub.

        $ git push origin new_feature
        # origin was automatically set to point to your fork when you cloned it


3. Once you're finished working on your branch, and have committed and pushed all your changes, submit a pull request from your `new_feature` branch to this repository's `master` branch.

 * Note: any new commits to the `new_feature` branch on GitHub will automatically be included in the pull request, so make sure to only commit related changes to the same branch.

## Pull Request Notes
If you file a PR but you're still working on it, please add a [WIP] before the title text. This will tell the reviewers that you still intend to add more to the PR and we don't need to review it yet. When it's ready to be reviewed by a merger just edit the title text to remove the [WIP].

If you are also looking for suggestions then mark it with [CR] — "comments requested". You can use both [WIP] and [CR] to indicated that you need opinion/code review/suggestions to continue working (e.g. "[WIP] [CR] Super awesome big feature"). Feel free to remove [CR] when you feel you got enough information to proceed.

This can help speed up our review process by allowing us to only review the things that are ready for it, and will prevent anything that isn't completely ready from being merged in.

### All PRs should have a "Summary" line
Summary is a one-line description of your change that will be extracted and added to the project changelog at https://github.com/CleverRaven/Cataclysm-DDA/blob/master/data/changelog.txt

The format is: ```SUMMARY: Category "description"```

The categories to choose from are: Features, Content, Interface, Mods, Balance, Bugfixes, Performance, Infrastructure, Build, I18N.

Example: ```SUMMARY: Content "Adds new mutation category 'Mouse'"```
Or, if you want it treated as a minor tweak that doesn't appear in the changelog:
```SUMMARY: None```

See the Changelog Guidelines at https://github.com/CleverRaven/Cataclysm-DDA/blob/master/doc/CHANGELOG_GUIDELINES.md for explanations of the categories.

### Closing issues using keywords

One more thing: when marking your PR as closing, fixing, or resolving issues, please include "XXXX #???" somewhere in the description, where XXX is on this list:
* close
* closes
* closed
* fix
* fixes
* fixed
* resolve
* resolves
* resolved

The "???" is the issue number. This automatically closes the issue when the PR is pulled in, and allows merges to work slightly faster. To close multiple issues format it as "XXXX #???, XXXX#???".

See https://help.github.com/articles/closing-issues-using-keywords/ for more.

## Tooling support

Various tools are available to help you keep your contributions conforming to the appropriate style.  See [the relevant docs](../doc/DEVELOPER_TOOLING.md) for more details.

## Advanced Techniques

These guidelines aren't essential, but they can make keeping things in order much easier.

#### Using remote tracking branches

Remote tracking branches allow you to easily stay in touch with this repository's `master` branch, as they automatically know which remote branch to get changes from.

    $ git branch -vv
    * master      xxxx [origin/master] ....
      new_feature xxxx ....

Here you can see we have two branches; `master` which is tracking `origin/master`, and `new_feature` which isn't tracking any branch. In practice, what this means is that git won't know where to get changes from.

    $ git checkout new_feature
    Switched to branch 'new_feature'
    $ git pull
    There is no tracking information for the current branch.
    Please specify which branch you want to merge with.

In order to easily pull changes from `upstream/master` into the `new_feature` branch, we can tell git which branch it should track. (You can even do this for your local master branch.)

    $ git branch -u upstream/master new_feature
    Branch new_feature set up to track remote branch master from upstream.
    $ git pull
    Updating xxxx..xxxx
    ....

You can also set the tracking information at the same time as creating the branch.

    $ git branch new_feature_2 --track upstream/master
    Branch new_feature_2 set up to track remote branch master from upstream.

 * Note: Although this makes it easier to pull from `upstream/master`, it doesn't change anything with regards to pushing. `git push` fails because you don't have permission to push to `upstream/master`.

        $ git push
        error: The requested URL returned error: 403 while accessing https://github.com/CleverRaven/Cataclysm-DDA.git
        fatal: HTTP request failed
        $ git push origin
        ....
        To https://github.com/YOUR_USERNAME/Cataclysm-DDA.git
        xxxx..xxxx  new_feature -> new_feature

## Unit tests

There is a suite of tests built into the source tree at tests/  
You should run the test suite after ANY change to the game source.  
An ordinary invocation of ``make`` will build the test executable at tests/cata_test, and it can be invoked like any ordinary executable.  
With no arguments it will run the entire test suite.  
With ``--help`` it will print a number of invocation options you can use to adjust its operation.

    $ make
    ... compilation details ...
    $ tests/cata_test
    Starting the actual test at Fri Nov  9 04:37:03 2018
    ===============================================================================
    All tests passed (1324684 assertions in 94 test cases)
    Ended test at Fri Nov  9 04:37:45 2018
    The test took 41.772 seconds

I recommend habitually invoking make like ``make YOUR BUILD OPTIONS && tests/cata_test``.

## In-game testing, test environment and the debug menu

Whether you are implementing a new feature or whether you are fixing a bug, it is always a good practice to test your changes in-game. It can be a hard task to create the exact conditions by playing a normal game to be able to test your changes, which is why there is a debug menu. There is no default key to bring up the menu so you will need to assign one first.

Bring up the keybindings menu (press `Escape` then `1`), scroll down almost to the bottom and press `+` to add a new key binding. Press the letter that corresponds to the *Debug menu* item, then press the key you want to use to bring up the debug menu. To test your changes, create a new world with a new character. Once you are in that world, press the key you just assigned for the debug menu and you should see something like this:

```
┌────────────────────────────────────────────┐
│ Debug Functions - Using these is CHEATING! │
├────────────────────────────────────────────┤
│ 1 Wish for an item                         │
│ 2 Teleport - Short Range                   │
│ 3 Teleport - Long Range                    │
│ 4 Reveal map                               │
│ 5 Spawn NPC                                │
│ 6 Spawn Monster                            │
│ 7 Check game state...                      │
│ 8 Kill NPCs                                │
│ 9 Mutate                                   │
│ 0 Spawn a vehicle                          │
│ a Change all skills                        │
│ b Learn all melee styles                   │
│ c Unlock all recipes                       │
│ d Edit player/NPC                          │
│ e Spawn Artifact                           │
│ f Spawn Clairvoyance Artifact              │
│ g Map editor                               │
│ h Change weather                           │
│ i Remove all monsters                      │
│ j Display hordes                           │
│ k Test Item Group                          │
│ l Damage Self                              │
│ m Show Sound Clustering                    │
│ n Lua Command                              │
│ o Display weather                          │
│ p Change time                              │
│ q Set automove route                       │
│ r Show mutation category levels            │
│ s Cancel                                   │
└────────────────────────────────────────────┘
```

With these commands, you should be able to recreate the proper conditions to test your changes. You can find some more information about the debug menu on [the official wiki](http://cddawiki.chezzo.com/cdda_wiki/index.php).

## Frequently Asked Questions

#### Why does `git pull --ff-only` result in an error?

If `git pull --ff-only` shows an error, it means that you've committed directly to your local `master` branch. To fix this, we create a new branch with these commits, find the point at which we diverged from `upstream/master`, and then reset `master` to that point.

    $ git pull --ff-only upstream master
    From https://github.com/CleverRaven/Cataclysm-DDA
     * branch            master     -> FETCH_HEAD
    fatal: Not possible to fast-forward, aborting.
    $ git branch new_branch master          # mark the current commit with a tmp branch
    $ git merge-base master upstream/master
    cc31d0... # the last commit before we committed directly to master
    $ git reset --hard cc31d0....
    HEAD is now at cc31d0... ...

Now that `master` has been cleaned up, we can easily pull from `upstream/master`, and then continue working on `new_branch`.

    $ git pull --ff-only upstream master
    # gets changes from the "upstream" remote for the matching branch, in this case "master"
    $ git checkout new_branch
